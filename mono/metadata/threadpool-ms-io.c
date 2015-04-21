/*
 * threadpool-ms-io.c: Microsoft IO threadpool runtime support
 *
 * Author:
 *	Ludovic Henry (ludovic.henry@xamarin.com)
 *
 * Copyright 2015 Xamarin, Inc (http://www.xamarin.com)
 */

#include <config.h>

#ifndef DISABLE_SOCKETS

#include <glib.h>

#if defined(HOST_WIN32)
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#endif

#include <mono/metadata/gc-internal.h>
#include <mono/metadata/mono-mlist.h>
#include <mono/metadata/threadpool-ms.h>
#include <mono/metadata/threadpool-ms-io.h>
#include <mono/utils/atomic.h>
#include <mono/utils/mono-threads.h>
#include <mono/utils/mono-lazy-init.h>
#include <mono/utils/mono-logger-internal.h>

typedef struct {
	gboolean (*init) (gint wakeup_pipe_fd);
	void     (*cleanup) (void);
	void     (*register_fd) (gint fd, gint operations, gboolean is_new);
	void     (*remove_fd) (gint fd);
	gint     (*event_wait) (void (*callback) (gint fd, gint events, gpointer user_data), gpointer user_data);
} ThreadPoolIOBackend;

/* Keep in sync with System.Threading.IOOperation */
enum {
	EVENT_IN   = 1 << 0,
	EVENT_OUT  = 1 << 1,
} ThreadPoolIOEvent;

#include "threadpool-ms-io-epoll.c"
#include "threadpool-ms-io-kqueue.c"
#include "threadpool-ms-io-poll.c"

#define UPDATES_CAPACITY 128

/* Keep in sync with System.Threading.IOAsyncResult */
struct _MonoIOAsyncResult {
	MonoObject obj;
	MonoObject *state;
	MonoObject *wait_handle;
	MonoBoolean completed_synchronously;
	MonoBoolean completed;
	gchar __dummy [SIZEOF_VOID_P - 2]; // align on SIZEOF_VOID_P bytes
	gpointer handle;
	MonoAsyncResult *async_result;
	MonoObject *async_callback;
	gint operation;
};

typedef enum {
	UPDATE_EMPTY = 0,
	UPDATE_ADD,
	UPDATE_REMOVE_SOCKET,
	UPDATE_REMOVE_DOMAIN,
} ThreadPoolIOUpdateType;

typedef struct {
	gint fd;
	MonoIOAsyncResult *ioares;
} ThreadPoolIOUpdate_Add;

typedef struct {
	gint fd;
} ThreadPoolIOUpdate_RemoveSocket;

typedef struct {
	MonoDomain *domain;
} ThreadPoolIOUpdate_RemoveDomain;

typedef struct {
	ThreadPoolIOUpdateType type;
	union {
		ThreadPoolIOUpdate_Add add;
		ThreadPoolIOUpdate_RemoveSocket remove_socket;
		ThreadPoolIOUpdate_RemoveDomain remove_domain;
	} data;
} ThreadPoolIOUpdate;

typedef struct {
	ThreadPoolIOBackend backend;

	ThreadPoolIOUpdate updates [UPDATES_CAPACITY];
	gint updates_size;
	mono_mutex_t updates_lock;
	mono_cond_t updates_cond;

#if !defined(HOST_WIN32)
	gint wakeup_pipes [2];
#else
	SOCKET wakeup_pipes [2];
#endif
} ThreadPoolIO;

static mono_lazy_init_t io_status = MONO_LAZY_INIT_STATUS_NOT_INITIALIZED;

static gboolean io_selector_running = FALSE;

static ThreadPoolIO* threadpool_io;

static MonoIOAsyncResult*
get_ioares_for_operation (MonoMList **list, gint operation)
{
	MonoMList *current;

	g_assert (list);

	for (current = *list; current; current = mono_mlist_next (current)) {
		MonoIOAsyncResult *ioares = (MonoIOAsyncResult*) mono_mlist_get_data (current);
		if ((ioares->operation & operation) != 0) {
			*list = mono_mlist_remove_item (*list, current);
			return ioares;
		}
	}

	return NULL;
}

static gint
get_operations (MonoMList *list)
{
	gint operations = 0;

	for (; list; list = mono_mlist_next (list))
		operations |= ((MonoIOAsyncResult*) mono_mlist_get_data (list))->operation;

	return operations;
}

static void
selector_thread_wakeup (void)
{
	gchar msg = 'c';
	gint written;

	for (;;) {
#if !defined(HOST_WIN32)
		written = write (threadpool_io->wakeup_pipes [1], &msg, 1);
		if (written == 1)
			break;
		if (written == -1) {
			g_warning ("selector_thread_wakeup: write () failed, error (%d) %s\n", errno, g_strerror (errno));
			break;
		}
#else
		written = send (threadpool_io->wakeup_pipes [1], &msg, 1, 0);
		if (written == 1)
			break;
		if (written == SOCKET_ERROR) {
			g_warning ("selector_thread_wakeup: write () failed, error (%d)\n", WSAGetLastError ());
			break;
		}
#endif
	}
}

static void
selector_thread_wakeup_drain_pipes (void)
{
	gchar buffer [128];
	gint received;

	for (;;) {
#if !defined(HOST_WIN32)
		received = read (threadpool_io->wakeup_pipes [0], buffer, sizeof (buffer));
		if (received == 0)
			break;
		if (received == -1) {
			if (errno != EINTR && errno != EAGAIN)
				g_warning ("selector_thread_wakeup_drain_pipes: read () failed, error (%d) %s\n", errno, g_strerror (errno));
			break;
		}
#else
		received = recv (threadpool_io->wakeup_pipes [0], buffer, sizeof (buffer), 0);
		if (received == 0)
			break;
		if (received == SOCKET_ERROR) {
			if (WSAGetLastError () != WSAEINTR && WSAGetLastError () != WSAEWOULDBLOCK)
				g_warning ("selector_thread_wakeup_drain_pipes: recv () failed, error (%d) %s\n", WSAGetLastError ());
			break;
		}
#endif
	}
}

typedef struct {
	MonoDomain *domain;
	MonoGHashTable *states;
} FilterIOAresForDomainData;

static void
filter_ioares_for_domain (gpointer key, gpointer value, gpointer user_data)
{
	FilterIOAresForDomainData *data;
	MonoMList *list = value, *element;
	MonoDomain *domain;
	MonoGHashTable *states;

	g_assert (user_data);
	data = user_data;
	domain = data->domain;
	states = data->states;

	for (element = list; element; element = mono_mlist_next (element)) {
		MonoIOAsyncResult *ioares = (MonoIOAsyncResult*) mono_mlist_get_data (element);
		if (mono_object_domain (ioares) == domain)
			mono_mlist_set_data (element, NULL);
	}

	/* we skip all the first elements which are NULL */
	for (; list; list = mono_mlist_next (list)) {
		if (mono_mlist_get_data (list))
			break;
	}

	if (list) {
		g_assert (mono_mlist_get_data (list));

		/* we delete all the NULL elements after the first one */
		for (element = list; element;) {
			MonoMList *next;
			if (!(next = mono_mlist_next (element)))
				break;
			if (mono_mlist_get_data (next))
				element = next;
			else
				mono_mlist_set_next (element, mono_mlist_next (next));
		}
	}

	mono_g_hash_table_replace (states, key, list);
}

static void
wait_callback (gint fd, gint events, gpointer user_data)
{
	if (mono_runtime_is_shutting_down ())
		return;

	if (fd == threadpool_io->wakeup_pipes [0]) {
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_THREADPOOL, "io threadpool: wke");
		selector_thread_wakeup_drain_pipes ();
	} else {
		MonoGHashTable *states;
		MonoMList *list = NULL;
		gpointer k;
		gint operations;

		g_assert (user_data);
		states = user_data;

		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_THREADPOOL, "io threadpool: cal fd %3d, events = %2s | %2s",
			fd, (events & EVENT_IN) ? "RD" : "..", (events & EVENT_OUT) ? "WR" : "..");

		if (!mono_g_hash_table_lookup_extended (states, GINT_TO_POINTER (fd), &k, (gpointer*) &list))
			g_error ("wait_callback: fd %d not found in states table", fd);

		if (list && (events & EVENT_IN) != 0) {
			MonoIOAsyncResult *ioares = get_ioares_for_operation (&list, EVENT_IN);
			if (ioares)
				mono_threadpool_ms_enqueue_work_item (((MonoObject*) ioares)->vtable->domain, (MonoObject*) ioares);
		}
		if (list && (events & EVENT_OUT) != 0) {
			MonoIOAsyncResult *ioares = get_ioares_for_operation (&list, EVENT_OUT);
			if (ioares)
				mono_threadpool_ms_enqueue_work_item (((MonoObject*) ioares)->vtable->domain, (MonoObject*) ioares);
		}

		mono_g_hash_table_replace (states, GINT_TO_POINTER (fd), list);

		operations = get_operations (list);

		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_THREADPOOL, "io threadpool: res fd %3d, operations = %2s | %2s",
			fd, (operations & EVENT_IN) ? "RD" : "..", (operations & EVENT_OUT) ? "WR" : "..");

		threadpool_io->backend.register_fd (fd, operations, FALSE);
	}
}

static void
selector_thread (gpointer data)
{
	MonoGHashTable *states;

	io_selector_running = TRUE;

	if (mono_runtime_is_shutting_down ()) {
		io_selector_running = FALSE;
		return;
	}

	states = mono_g_hash_table_new_type (g_direct_hash, g_direct_equal, MONO_HASH_VALUE_GC);

	for (;;) {
		gint i, j;
		gint res;

		mono_mutex_lock (&threadpool_io->updates_lock);

		for (i = 0; i < threadpool_io->updates_size; ++i) {
			ThreadPoolIOUpdate *update = &threadpool_io->updates [i];

			switch (update->type) {
			case UPDATE_EMPTY:
				break;
			case UPDATE_ADD: {
				gint fd;
				gint operations;
				gpointer k;
				gboolean exists;
				MonoMList *list = NULL;
				MonoIOAsyncResult *ioares;

				fd = update->data.add.fd;
				g_assert (fd >= 0);

				ioares = update->data.add.ioares;
				g_assert (ioares);

				exists = mono_g_hash_table_lookup_extended (states, GINT_TO_POINTER (fd), &k, (gpointer*) &list);
				list = mono_mlist_append (list, (MonoObject*) ioares);
				mono_g_hash_table_replace (states, ioares->handle, list);

				operations = get_operations (list);

				mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_THREADPOOL, "io threadpool: %3s fd %3d, operations = %2s | %2s",
					exists ? "mod" : "add", fd, (operations & EVENT_IN) ? "RD" : "..", (operations & EVENT_OUT) ? "WR" : "..");

				threadpool_io->backend.register_fd (fd, operations, !exists);

				break;
			}
			case UPDATE_REMOVE_SOCKET: {
				gint fd;
				gpointer k;
				MonoMList *list = NULL;

				fd = update->data.remove_socket.fd;
				g_assert (fd >= 0);

				if (mono_g_hash_table_lookup_extended (states, GINT_TO_POINTER (fd), &k, (gpointer*) &list)) {
					mono_g_hash_table_remove (states, GINT_TO_POINTER (fd));

					for (j = i + 1; j < threadpool_io->updates_size; ++j) {
						ThreadPoolIOUpdate *update = &threadpool_io->updates [j];
						if (update->type == UPDATE_ADD && update->data.add.fd == fd)
							memset (update, 0, sizeof (ThreadPoolIOUpdate));
					}

					for (; list; list = mono_mlist_remove_item (list, list)) {
						MonoSocketAsyncResult *sockares = (MonoSocketAsyncResult*) mono_mlist_get_data (list);

						switch (sockares->operation) {
						case AIO_OP_RECEIVE:
							sockares->operation = AIO_OP_RECV_JUST_CALLBACK;
							break;
						case AIO_OP_SEND:
							sockares->operation = AIO_OP_SEND_JUST_CALLBACK;
							break;
						}

						mono_threadpool_ms_enqueue_work_item (mono_object_domain (sockares), (MonoObject*) sockares);
					}

					mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_THREADPOOL, "io threadpool: del fd %3d", fd);
					threadpool_io->backend.remove_fd (fd);
				}

				break;
			}
			case UPDATE_REMOVE_DOMAIN: {
				MonoDomain *domain;

				domain = update->data.remove_domain.domain;
				g_assert (domain);

				FilterIOAresForDomainData user_data = { .domain = domain, .states = states };
				mono_g_hash_table_foreach (states, filter_ioares_for_domain, &user_data);

				for (j = i + 1; j < threadpool_io->updates_size; ++j) {
					ThreadPoolIOUpdate *update = &threadpool_io->updates [j];
					if (update->type == UPDATE_ADD && mono_object_domain (update->data.add.ioares) == domain)
						memset (update, 0, sizeof (ThreadPoolIOUpdate));
				}

				break;
			}
			default:
				g_assert_not_reached ();
			}
		}

		mono_cond_broadcast (&threadpool_io->updates_cond);

		if (threadpool_io->updates_size > 0) {
			threadpool_io->updates_size = 0;
			memset (&threadpool_io->updates, 0, UPDATES_CAPACITY * sizeof (ThreadPoolIOUpdate));
		}

		mono_mutex_unlock (&threadpool_io->updates_lock);

		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_THREADPOOL, "io threadpool: wai");

		res = threadpool_io->backend.event_wait (wait_callback, states);

		if (res == -1 || mono_runtime_is_shutting_down ())
			break;
	}

	mono_g_hash_table_destroy (states);

	io_selector_running = FALSE;
}

static ThreadPoolIOUpdate*
update_get_new (void)
{
	ThreadPoolIOUpdate *update = NULL;

	mono_mutex_lock (&threadpool_io->updates_lock);

	g_assert (threadpool_io->updates_size <= UPDATES_CAPACITY);

	while (threadpool_io->updates_size == UPDATES_CAPACITY) {
		/* we wait for updates to be applied in the selector_thread and we loop
		 * as long as none are available. if it happends too much, then we need
		 * to increase UPDATES_CAPACITY */
		mono_cond_wait (&threadpool_io->updates_cond, &threadpool_io->updates_lock);
	}

	g_assert (threadpool_io->updates_size < UPDATES_CAPACITY);

	update = &threadpool_io->updates [threadpool_io->updates_size ++];

	mono_mutex_unlock (&threadpool_io->updates_lock);

	return update;
}

static void
wakeup_pipes_init (void)
{
#if !defined(HOST_WIN32)
	if (pipe (threadpool_io->wakeup_pipes) == -1)
		g_error ("wakeup_pipes_init: pipe () failed, error (%d) %s\n", errno, g_strerror (errno));
	if (fcntl (threadpool_io->wakeup_pipes [0], F_SETFL, O_NONBLOCK) == -1)
		g_error ("wakeup_pipes_init: fcntl () failed, error (%d) %s\n", errno, g_strerror (errno));
#else
	struct sockaddr_in client;
	struct sockaddr_in server;
	SOCKET server_sock;
	gulong arg;
	gint size;

	server_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	g_assert (server_sock != INVALID_SOCKET);
	threadpool_io->wakeup_pipes [1] = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	g_assert (threadpool_io->wakeup_pipes [1] != INVALID_SOCKET);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr ("127.0.0.1");
	server.sin_port = 0;
	if (bind (server_sock, (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("wakeup_pipes_init: bind () failed, error (%d)\n", WSAGetLastError ());
	}

	size = sizeof (server);
	if (getsockname (server_sock, (SOCKADDR*) &server, &size) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("wakeup_pipes_init: getsockname () failed, error (%d)\n", WSAGetLastError ());
	}
	if (listen (server_sock, 1024) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("wakeup_pipes_init: listen () failed, error (%d)\n", WSAGetLastError ());
	}
	if (connect ((SOCKET) threadpool_io->wakeup_pipes [1], (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("wakeup_pipes_init: connect () failed, error (%d)\n", WSAGetLastError ());
	}

	size = sizeof (client);
	threadpool_io->wakeup_pipes [0] = accept (server_sock, (SOCKADDR *) &client, &size);
	g_assert (threadpool_io->wakeup_pipes [0] != INVALID_SOCKET);

	arg = 1;
	if (ioctlsocket (threadpool_io->wakeup_pipes [0], FIONBIO, &arg) == SOCKET_ERROR) {
		closesocket (threadpool_io->wakeup_pipes [0]);
		closesocket (server_sock);
		g_error ("wakeup_pipes_init: ioctlsocket () failed, error (%d)\n", WSAGetLastError ());
	}

	closesocket (server_sock);
#endif
}

static void
initialize (void)
{
	g_assert (!threadpool_io);
	threadpool_io = g_new0 (ThreadPoolIO, 1);
	g_assert (threadpool_io);

	mono_mutex_init_recursive (&threadpool_io->updates_lock);
	mono_cond_init (&threadpool_io->updates_cond, NULL);

	threadpool_io->updates_size = 0;

	threadpool_io->backend = backend_poll;
	if (g_getenv ("MONO_ENABLE_AIO") != NULL) {
#if defined(HAVE_EPOLL)
		threadpool_io->backend = backend_epoll;
#elif defined(HAVE_KQUEUE)
		threadpool_io->backend = backend_kqueue;
#endif
	}

	wakeup_pipes_init ();

	if (!threadpool_io->backend.init (threadpool_io->wakeup_pipes [0]))
		g_error ("initialize: backend->init () failed");

	if (!mono_thread_create_internal (mono_get_root_domain (), selector_thread, NULL, TRUE, SMALL_STACK))
		g_error ("initialize: mono_thread_create_internal () failed");
}

static void
cleanup (void)
{
	/* we make the assumption along the code that we are
	 * cleaning up only if the runtime is shutting down */
	g_assert (mono_runtime_is_shutting_down ());

	selector_thread_wakeup ();
	while (io_selector_running)
		g_usleep (1000);

	mono_mutex_destroy (&threadpool_io->updates_lock);
	mono_cond_destroy (&threadpool_io->updates_cond);

	threadpool_io->backend.cleanup ();

#if !defined(HOST_WIN32)
	close (threadpool_io->wakeup_pipes [0]);
	close (threadpool_io->wakeup_pipes [1]);
#else
	closesocket (threadpool_io->wakeup_pipes [0]);
	closesocket (threadpool_io->wakeup_pipes [1]);
#endif

	g_assert (threadpool_io);
	g_free (threadpool_io);
	threadpool_io = NULL;
	g_assert (!threadpool_io);
}

gboolean
mono_threadpool_ms_is_io (MonoObject *target, MonoObject *state)
{
	static MonoClass *io_async_result_class = NULL;

	if (!io_async_result_class)
		io_async_result_class = mono_class_from_name (mono_defaults.corlib, "System.Threading", "IOAsyncResult");
	g_assert (io_async_result_class);

	return mono_class_has_parent (target->vtable->klass, io_async_result_class);
}

void
mono_threadpool_ms_io_cleanup (void)
{
	mono_lazy_cleanup (&io_status, cleanup);
}

MonoAsyncResult *
mono_threadpool_ms_io_add (MonoAsyncResult *ares, MonoIOAsyncResult *ioares)
{
	ThreadPoolIOUpdate *update;

	g_assert (ares);
	g_assert (ioares);

	if (mono_runtime_is_shutting_down ())
		return NULL;
	if (mono_domain_is_unloading (mono_object_domain (ioares)))
		return NULL;

	mono_lazy_initialize (&io_status, initialize);

	MONO_OBJECT_SETREF (ioares, async_result, ares);

	mono_mutex_lock (&threadpool_io->updates_lock);

	update = update_get_new ();
	update->type = UPDATE_ADD;
	update->data.add.fd = GPOINTER_TO_INT (ioares->handle);
	update->data.add.ioares = ioares;

	selector_thread_wakeup ();

	mono_mutex_unlock (&threadpool_io->updates_lock);

	return ares;
}

void
mono_threadpool_ms_io_remove_socket (int fd)
{
	ThreadPoolIOUpdate *update;

	if (!mono_lazy_is_initialized (&io_status))
		return;

	mono_mutex_lock (&threadpool_io->updates_lock);

	update = update_get_new ();
	update->type = UPDATE_REMOVE_SOCKET;
	update->data.add.fd = fd;

	selector_thread_wakeup ();

	mono_cond_wait (&threadpool_io->updates_cond, &threadpool_io->updates_lock);

	mono_mutex_unlock (&threadpool_io->updates_lock);
}

void
mono_threadpool_ms_io_remove_domain_jobs (MonoDomain *domain)
{
	ThreadPoolIOUpdate *update;

	if (!mono_lazy_is_initialized (&io_status))
		return;

	mono_mutex_lock (&threadpool_io->updates_lock);

	update = update_get_new ();
	update->type = UPDATE_REMOVE_DOMAIN;
	update->data.remove_domain.domain = domain;

	selector_thread_wakeup ();

	mono_cond_wait (&threadpool_io->updates_cond, &threadpool_io->updates_lock);

	mono_mutex_unlock (&threadpool_io->updates_lock);
}

void
icall_append_io_job (MonoObject *target, MonoIOAsyncResult *state)
{
	MonoAsyncResult *ares;

	/* Don't call mono_async_result_new() to avoid capturing the context */
	ares = (MonoAsyncResult *) mono_object_new (mono_domain_get (), mono_defaults.asyncresult_class);
	MONO_OBJECT_SETREF (ares, async_delegate, target);
	MONO_OBJECT_SETREF (ares, async_state, state);

	mono_threadpool_ms_io_add (ares, state);
	return;
}

#else

gboolean
mono_threadpool_ms_is_io (MonoObject *target, MonoObject *state)
{
	return FALSE;
}

void
mono_threadpool_ms_io_cleanup (void)
{
	g_assert_not_reached ();
}

MonoAsyncResult *
mono_threadpool_ms_io_add (MonoAsyncResult *ares, MonoIOAsyncResult *ioares)
{
	g_assert_not_reached ();
}

void
mono_threadpool_ms_io_remove_socket (int fd)
{
	g_assert_not_reached ();
}

void
mono_threadpool_ms_io_remove_domain_jobs (MonoDomain *domain)
{
	g_assert_not_reached ();
}

void
icall_append_io_job (MonoObject *target, MonoIOAsyncResult *state)
{
	g_assert_not_reached ();
}

#endif

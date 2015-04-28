/*
 * threadpool-ms-io.c: Microsoft IO threadpool runtime support
 *
 * Author:
 *	Ludovic Henry (ludovic.henry@xamarin.com)
 *
 * Copyright 2015 Xamarin, Inc (http://www.xamarin.com)
 */

#include <config.h>
#include <glib.h>

#ifndef DISABLE_SOCKETS

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
#include <mono/utils/mono-poll.h>
#include <mono/utils/mono-threads.h>

/* Keep in sync with System.Threading.IOOperation */
typedef enum {
	IO_OP_UNDEFINED = 0,
	IO_OP_IN        = 1,
	IO_OP_OUT       = 2,
} MonoIOOperation;

typedef struct {
	gint fd;
	MonoIOOperation operations;
	gboolean is_new;
} ThreadPoolIOUpdate;

typedef struct {
	gboolean (*init) (gint wakeup_pipe_fd);
	void     (*cleanup) (void);
	void     (*update_add) (ThreadPoolIOUpdate *update);
	gint     (*event_wait) (void);
	gint     (*event_get_fd_max) (void);
	gint     (*event_get_fd_at) (gint i, MonoIOOperation *operations);
	void     (*event_reset_fd_at) (gint i, MonoIOOperation operations);
} ThreadPoolIOBackend;

#include "threadpool-ms-io-epoll.c"
#include "threadpool-ms-io-kqueue.c"
#include "threadpool-ms-io-poll.c"

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
	MonoIOOperation operation;
};

typedef struct {
	MonoGHashTable *states;
	mono_mutex_t states_lock;

	ThreadPoolIOBackend backend;

	ThreadPoolIOUpdate *updates;
	guint updates_size;
	mono_mutex_t updates_lock;

	gint wakeup_pipes [2];
} ThreadPoolIO;

static gint32 io_status = STATUS_NOT_INITIALIZED;
static gint32 io_thread_status = STATUS_NOT_INITIALIZED;

static ThreadPoolIO* threadpool_io;

static MonoIOAsyncResult*
get_ioares_for_operation (MonoMList **list, MonoIOOperation operation)
{
	MonoIOAsyncResult *ioares = NULL;
	MonoMList *current;

	g_assert (list);

	for (current = *list; current; current = mono_mlist_next (current)) {
		ioares = (MonoIOAsyncResult*) mono_mlist_get_data (current);
		if ((ioares->operation & operation) != 0)
			break;
		ioares = NULL;
	}

	if (current)
		*list = mono_mlist_remove_item (*list, current);

	return ioares;
}

static MonoIOOperation
get_operations (MonoMList *list)
{
	MonoIOAsyncResult *ioares;
	MonoIOOperation operations = 0;

	for (; list; list = mono_mlist_next (list))
		if ((ioares = (MonoIOAsyncResult*) mono_mlist_get_data (list)))
			operations |= ioares->operation;

	return operations;
}

static void
selector_thread_wakeup (gint wrhandle)
{
	gchar c = 'c';
	gint written;

	for (;;) {
#if !defined(HOST_WIN32)
		written = write (wrhandle, &c, sizeof (c));
		if (written == sizeof (c))
			break;
		if (written == -1) {
			g_warning ("selector_thread_wakeup: write () failed, error (%d) %s\n", errno, g_strerror (errno));
			break;
		}
#else
		written = send (wrhandle, &c, sizeof (c), 0);
		if (written == sizeof (c))
			break;
		if (written == SOCKET_ERROR) {
			g_warning ("selector_thread_wakeup: send () failed, error (%d)\n", WSAGetLastError ());
			break;
		}
#endif
	}
}

static void
selector_thread_wakeup_drain_pipes (gint rdhandle)
{
	gchar buffer [128];
	gint received;

	for (;;) {
#if !defined(HOST_WIN32)
		received = read (rdhandle, buffer, sizeof (buffer));
		if (received == 0)
			break;
		if (received == -1) {
			if (errno != EINTR && errno != EAGAIN) {
				g_warning ("selector_thread_wakeup_drain_pipes: read () failed, error (%d) %s\n", errno, g_strerror (errno));
			}
			break;
		}
#else
		received = recv (rdhandle, buffer, sizeof (buffer), 0);
		if (received == 0)
			break;
		if (received == SOCKET_ERROR) {
			if (WSAGetLastError () != WSAEINTR && WSAGetLastError () != WSAEWOULDBLOCK) {
				g_warning ("selector_thread_wakeup_drain_pipes: recv () failed, error (%d)\n", WSAGetLastError ());
			}
			break;
		}
#endif
	}
}

static void
selector_thread_wakeup_create_pipes (gint *rdhandle, gint *wrhandle)
{
	gint wakeup_pipes [2];

	g_assert (rdhandle);
	g_assert (wrhandle);

	*rdhandle = 0;
	*wrhandle = 0;

#if !defined(HOST_WIN32)
	if (pipe (wakeup_pipes) == -1) {
		g_error ("selector_thread_wakeup_create_pipes: pipe () failed, error (%d) %s\n", errno, g_strerror (errno));
		return;
	}
	if (fcntl (wakeup_pipes [0], F_SETFL, O_NONBLOCK) == -1) {
		g_error ("selector_thread_wakeup_create_pipes: fcntl () failed, error (%d) %s\n", errno, g_strerror (errno));
		return;
	}
#else
	SOCKET wakeup_pipes [2];
	struct sockaddr_in client;
	struct sockaddr_in server;
	SOCKET server_sock;
	gulong arg;
	gint size;

	g_assert (rdhandle);
	g_assert (wrhandle);

	*rdhandle = 0;
	*wrhandle = 0;

	server_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	g_assert (server_sock != INVALID_SOCKET);
	wakeup_pipes [1] = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	g_assert (wakeup_pipes [1] != INVALID_SOCKET);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr ("127.0.0.1");
	server.sin_port = 0;
	if (bind (server_sock, (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("selector_thread_wakeup_create_pipes: bind () failed, error (%d)\n", WSAGetLastError ());
		return;
	}

	size = sizeof (server);
	if (getsockname (server_sock, (SOCKADDR*) &server, &size) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("selector_thread_wakeup_create_pipes: getsockname () failed, error (%d)\n", WSAGetLastError ());
		return;
	}
	if (listen (server_sock, 1024) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("selector_thread_wakeup_create_pipes: listen () failed, error (%d)\n", WSAGetLastError ());
		return;
	}
	if (connect ((SOCKET) wakeup_pipes [1], (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_error ("selector_thread_wakeup_create_pipes: connect () failed, error (%d)\n", WSAGetLastError ());
		return;
	}

	size = sizeof (client);
	wakeup_pipes [0] = accept (server_sock, (SOCKADDR *) &client, &size);
	g_assert (wakeup_pipes [0] != INVALID_SOCKET);

	arg = 1;
	if (ioctlsocket (wakeup_pipes [0], FIONBIO, &arg) == SOCKET_ERROR) {
		closesocket (wakeup_pipes [0]);
		closesocket (server_sock);
		g_error ("selector_thread_wakeup_create_pipes: ioctlsocket () failed, error (%d)\n", WSAGetLastError ());
		return;
	}

	closesocket (server_sock);
#endif

	*rdhandle = wakeup_pipes [0];
	*wrhandle = wakeup_pipes [1];
}

static void
selector_thread_cleanup (gint rdhandle, gint wrhandle)
{
#if !defined(HOST_WIN32)
	close (rdhandle);
	close (wrhandle);
#else
	closesocket (rdhandle);
	closesocket (wrhandle);
#endif
}

static void
selector_thread (gpointer data)
{
	io_thread_status = STATUS_INITIALIZED;

	for (;;) {
		guint i;
		guint max;
		gint ready = 0;

		mono_gc_set_skip_thread (TRUE);

		mono_mutex_lock (&threadpool_io->updates_lock);
		for (i = 0; i < threadpool_io->updates_size; ++i) {
			threadpool_io->backend.update_add (&threadpool_io->updates [i]);
		}
		if (threadpool_io->updates_size > 0) {
			threadpool_io->updates_size = 0;
			threadpool_io->updates = g_renew (ThreadPoolIOUpdate, threadpool_io->updates, threadpool_io->updates_size);
		}
		mono_mutex_unlock (&threadpool_io->updates_lock);

		ready = threadpool_io->backend.event_wait ();

		mono_gc_set_skip_thread (FALSE);

		if (ready == -1 || mono_runtime_is_shutting_down ())
			break;

		max = threadpool_io->backend.event_get_fd_max ();

		mono_mutex_lock (&threadpool_io->states_lock);
		for (i = 0; ready > 0 && i < max; ++i) {
			MonoIOOperation operations;
			gint fd = threadpool_io->backend.event_get_fd_at (i, &operations);

			if (fd == -1)
				continue;

			if (fd == GPOINTER_TO_INT (threadpool_io->wakeup_pipes [0])) {
				selector_thread_wakeup_drain_pipes (threadpool_io->wakeup_pipes [0]);
			} else {
				MonoMList *list = mono_g_hash_table_lookup (threadpool_io->states, GINT_TO_POINTER (fd));

				if (list && (operations & IO_OP_IN) != 0) {
					MonoIOAsyncResult *ioares = get_ioares_for_operation (&list, IO_OP_IN);
					if (ioares)
						mono_threadpool_ms_enqueue_work_item (((MonoObject*) ioares)->vtable->domain, (MonoObject*) ioares);
				}
				if (list && (operations & IO_OP_OUT) != 0) {
					MonoIOAsyncResult *ioares = get_ioares_for_operation (&list, IO_OP_OUT);
					if (ioares)
						mono_threadpool_ms_enqueue_work_item (((MonoObject*) ioares)->vtable->domain, (MonoObject*) ioares);
				}

				if (!list) {
					mono_g_hash_table_remove (threadpool_io->states, GINT_TO_POINTER (fd));
				} else {
					mono_g_hash_table_replace (threadpool_io->states, GINT_TO_POINTER (fd), list);
					threadpool_io->backend.event_reset_fd_at (i, get_operations (list));
				}
			}

			ready -= 1;
		}
		mono_mutex_unlock (&threadpool_io->states_lock);
	}

	io_thread_status = STATUS_CLEANED_UP;
}

static gboolean
backend_init (gint wakeup_rdhandle)
{
#if defined(HAVE_EPOLL)
	threadpool_io->backend = backend_epoll;
#elif defined(HAVE_KQUEUE)
	threadpool_io->backend = backend_kqueue;
#else
	threadpool_io->backend = backend_poll;
#endif
	if (g_getenv ("MONO_DISABLE_AIO") != NULL)
		threadpool_io->backend = backend_poll;

	return threadpool_io->backend.init (wakeup_rdhandle);
}

static void
backend_cleanup (void)
{
	threadpool_io->backend.cleanup ();
}

static void
ensure_initialized (void)
{
	if (io_status >= STATUS_INITIALIZED)
		return;
	if (io_status == STATUS_INITIALIZING || InterlockedCompareExchange (&io_status, STATUS_INITIALIZING, STATUS_NOT_INITIALIZED) != STATUS_NOT_INITIALIZED) {
		while (io_status == STATUS_INITIALIZING)
			mono_thread_info_yield ();
		g_assert (io_status >= STATUS_INITIALIZED);
		return;
	}

	g_assert (!threadpool_io);
	threadpool_io = g_new0 (ThreadPoolIO, 1);
	g_assert (threadpool_io);

	threadpool_io->states = mono_g_hash_table_new_type (g_direct_hash, g_direct_equal, MONO_HASH_VALUE_GC);
	MONO_GC_REGISTER_ROOT_FIXED (threadpool_io->states);
	mono_mutex_init (&threadpool_io->states_lock);

	threadpool_io->updates = NULL;
	threadpool_io->updates_size = 0;
	mono_mutex_init (&threadpool_io->updates_lock);

	selector_thread_wakeup_create_pipes (&threadpool_io->wakeup_pipes [0], &threadpool_io->wakeup_pipes [1]);

	if (!backend_init (threadpool_io->wakeup_pipes [0])) {
		g_error ("ensure_initialized: backend_init () failed");
		return;
	}

	if (!mono_thread_create_internal (mono_get_root_domain (), selector_thread, NULL, TRUE, SMALL_STACK)) {
		g_error ("ensure_initialized: mono_thread_create_internal () failed");
		return;
	}

	io_thread_status = STATUS_INITIALIZING;
	mono_memory_write_barrier ();

	io_status = STATUS_INITIALIZED;
}

static void
ensure_cleanedup (void)
{
	if (io_status == STATUS_NOT_INITIALIZED && InterlockedCompareExchange (&io_status, STATUS_CLEANED_UP, STATUS_NOT_INITIALIZED) == STATUS_NOT_INITIALIZED)
		return;
	if (io_status == STATUS_INITIALIZING) {
		while (io_status == STATUS_INITIALIZING)
			mono_thread_info_yield ();
	}
	if (io_status == STATUS_CLEANED_UP)
		return;
	if (io_status == STATUS_CLEANING_UP || InterlockedCompareExchange (&io_status, STATUS_CLEANING_UP, STATUS_INITIALIZED) != STATUS_INITIALIZED) {
		while (io_status == STATUS_CLEANING_UP)
			mono_thread_info_yield ();
		g_assert (io_status == STATUS_CLEANED_UP);
		return;
	}

	/* we make the assumption along the code that we are
	 * cleaning up only if the runtime is shutting down */
	g_assert (mono_runtime_is_shutting_down ());

	selector_thread_wakeup (threadpool_io->wakeup_pipes [1]);
	while (io_thread_status != STATUS_CLEANED_UP)
		usleep (1000);

	MONO_GC_UNREGISTER_ROOT (threadpool_io->states);
	mono_g_hash_table_destroy (threadpool_io->states);
	mono_mutex_destroy (&threadpool_io->states_lock);

	g_free (threadpool_io->updates);
	mono_mutex_destroy (&threadpool_io->updates_lock);

	backend_cleanup ();

	selector_thread_cleanup (threadpool_io->wakeup_pipes [0], threadpool_io->wakeup_pipes [1]);

	g_assert (threadpool_io);
	g_free (threadpool_io);
	threadpool_io = NULL;
	g_assert (!threadpool_io);

	io_status = STATUS_CLEANED_UP;
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
	ensure_cleanedup ();
}

MonoAsyncResult *
mono_threadpool_ms_io_add (MonoAsyncResult *ares, MonoIOAsyncResult *ioares)
{
	ThreadPoolIOUpdate *update;
	MonoMList *list;
	gboolean is_new;

	g_assert (ares);
	g_assert (ioares);

	if (mono_runtime_is_shutting_down ())
		return NULL;

	ensure_initialized ();

	MONO_OBJECT_SETREF (ioares, async_result, ares);

	mono_mutex_lock (&threadpool_io->states_lock);
	g_assert (threadpool_io->states);

	list = mono_g_hash_table_lookup (threadpool_io->states, ioares->handle);
	is_new = list == NULL;
	list = mono_mlist_append (list, (MonoObject*) ioares);
	mono_g_hash_table_replace (threadpool_io->states, ioares->handle, list);

	mono_mutex_lock (&threadpool_io->updates_lock);
	threadpool_io->updates_size += 1;
	threadpool_io->updates = g_renew (ThreadPoolIOUpdate, threadpool_io->updates, threadpool_io->updates_size);

	update = &threadpool_io->updates [threadpool_io->updates_size - 1];
	update->fd = GPOINTER_TO_INT (ioares->handle);
	update->operations = ioares->operation;
	update->is_new = is_new;
	mono_mutex_unlock (&threadpool_io->updates_lock);

	mono_mutex_unlock (&threadpool_io->states_lock);

	selector_thread_wakeup (threadpool_io->wakeup_pipes [1]);

	return ares;
}

void
mono_threadpool_ms_io_remove_socket (int fd)
{
	MonoMList *list;

	if (io_status != STATUS_INITIALIZED)
		return;

	mono_mutex_lock (&threadpool_io->states_lock);
	g_assert (threadpool_io->states);
	list = mono_g_hash_table_lookup (threadpool_io->states, GINT_TO_POINTER (fd));
	if (list)
		mono_g_hash_table_remove (threadpool_io->states, GINT_TO_POINTER (fd));
	mono_mutex_unlock (&threadpool_io->states_lock);

	for (; list; list = mono_mlist_remove_item (list, list)) {
		MonoIOAsyncResult *ioares = (MonoIOAsyncResult*) mono_mlist_get_data (list);
		if (ioares)
			mono_threadpool_ms_enqueue_work_item (((MonoObject*) ioares)->vtable->domain, (MonoObject*) ioares);
	}
}

static gboolean
remove_sockstate_for_domain (gpointer key, gpointer value, gpointer user_data)
{
	MonoMList *list;
	gboolean remove = FALSE;

	for (list = value; list; list = mono_mlist_next (list)) {
		MonoObject *data = mono_mlist_get_data (list);
		if (mono_object_domain (data) == user_data) {
			remove = TRUE;
			mono_mlist_set_data (list, NULL);
		}
	}

	//FIXME is there some sort of additional unregistration we need to perform here?
	return remove;
}

void
mono_threadpool_ms_io_remove_domain_jobs (MonoDomain *domain)
{
	if (io_status == STATUS_INITIALIZED) {
		mono_mutex_lock (&threadpool_io->states_lock);
		mono_g_hash_table_foreach_remove (threadpool_io->states, remove_sockstate_for_domain, domain);
		mono_mutex_unlock (&threadpool_io->states_lock);
	}
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
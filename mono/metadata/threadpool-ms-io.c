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
#include <mono/metadata/threadpool-ms.h>
#include <mono/metadata/threadpool-ms-io.h>
#include <mono/utils/atomic.h>
#include <mono/utils/mono-threads.h>

/* Keep in sync with System.Threading.IOOperation */
enum {
	IO_OP_UNDEFINED = 0,
	IO_OP_IN        = 1,
	IO_OP_OUT       = 2,
};

/* Keep in sync with System.Threading.NativeThreadPoolIOBackend */
struct _MonoThreadPoolIOUpdate {
	gpointer handle;
	gint32 operations;
	gboolean is_new;
};

typedef struct {
	gint (*init) (gpointer *backend_data, gint wakeup_pipe_fd, gchar *error, gint error_size);
	void (*cleanup) (gpointer backend_data);
	gint (*update_add) (gpointer backend_data, MonoThreadPoolIOUpdate *update, gchar *error, gint error_size);
	gint (*event_wait) (gpointer backend_data, gchar *error, gint error_size);
	gint (*event_get_fd_max) (gpointer backend_data);
	gint (*event_get_fd_at) (gpointer backend_data, gint i, gint32 *operations);
	gint (*event_reset_fd_at) (gpointer backend_data, gint i, gint32 operations, gchar *error, gint error_size);
} ThreadPoolIOBackend;

#include "threadpool-ms-io-epoll.c"
#include "threadpool-ms-io-kqueue.c"
#include "threadpool-ms-io-poll.c"

/* Keep in sync with System.Threading.NativeThreadPoolIOBackend */
struct _MonoThreadPoolIO {
	MonoObject obj;
	ThreadPoolIOBackend *backend;
	gpointer backend_data;
};

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadCreateWakeupPipes_internal (gpointer *rdhandle, gpointer *wrhandle)
{
	gchar error [256];
	gint wakeup_pipes [2];

	g_assert (rdhandle);
	g_assert (wrhandle);

	*rdhandle = 0;
	*wrhandle = 0;

#if !defined(HOST_WIN32)
	if (pipe (wakeup_pipes) == -1) {
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: pipe () failed, error (%d) %s\n", errno, g_strerror (errno));
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}
	if (fcntl (wakeup_pipes [0], F_SETFL, O_NONBLOCK) == -1) {
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: fcntl () failed, error (%d) %s\n", errno, g_strerror (errno));
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}
#else
	SOCKET wakeup_pipes [2];
	struct sockaddr_in client;
	struct sockaddr_in server;
	SOCKET server_sock;
	gulong arg;
	gint size;

	if ((server_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: socket (0) failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}

	if ((wakeup_pipes [1] = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		closesocket (server_sock);
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: socket (1) failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr ("127.0.0.1");
	server.sin_port = 0;
	if (bind (server_sock, (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: bind () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}

	size = sizeof (server);
	if (getsockname (server_sock, (SOCKADDR*) &server, &size) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: getsockname () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}
	if (listen (server_sock, 1024) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: listen () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}
	if (connect ((SOCKET) wakeup_pipes [1], (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: connect () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}

	size = sizeof (client);
	if ((wakeup_pipes [0] = accept (server_sock, (SOCKADDR *) &client, &size)) == INVALID_SOCKET) {
		closesocket (server_sock);
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: accept () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}

	arg = 1;
	if (ioctlsocket (wakeup_pipes [0], FIONBIO, &arg) == SOCKET_ERROR) {
		closesocket (wakeup_pipes [0]);
		closesocket (server_sock);
		g_snprintf (error, sizeof (error), "selector_thread_create_wakeup_pipes: ioctlsocket () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (error));
		return;
	}

	closesocket (server_sock);
#endif

	*rdhandle = GINT_TO_POINTER (wakeup_pipes [0]);
	*wrhandle = GINT_TO_POINTER (wakeup_pipes [1]);
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadWakeup_internal (gpointer wrhandle)
{
	gchar error [256];
	gchar c = 'c';
	gint written;

	for (;;) {
#if !defined(HOST_WIN32)
		written = write (GPOINTER_TO_INT (wrhandle), &c, sizeof (c));
		if (written == sizeof (c))
			break;
		if (written == -1) {
			g_snprintf (error, sizeof (error), "selector_thread_wakeup: write () failed, error (%d) %s\n", errno, g_strerror (errno));
			mono_raise_exception (mono_get_exception_io (error));
			break;
		}
#else
		written = send (GPOINTER_TO_INT (wrhandle), &c, sizeof (c), 0);
		if (written == sizeof (c))
			break;
		if (written == SOCKET_ERROR) {
			g_snprintf (error, sizeof (error), "selector_thread_wakeup: send () failed, error (%d)\n", WSAGetLastError ());
			mono_raise_exception (mono_get_exception_io (error));
			break;
		}
#endif
	}
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadDrainWakeupPipes_internal (gpointer rdhandle)
{
	gchar error [256];
	gchar buffer [128];
	gint received;

	for (;;) {
#if !defined(HOST_WIN32)
		received = read (GPOINTER_TO_INT (rdhandle), buffer, sizeof (buffer));
		if (received == 0)
			break;
		if (received == -1) {
			if (errno != EINTR && errno != EAGAIN) {
				g_snprintf (error, sizeof (error), "selector_thread_wakeup_drain_pipes: read () failed, error (%d) %s\n", errno, g_strerror (errno));
				mono_raise_exception (mono_get_exception_io (error));
				break;
			}
			break;
		}
#else
		received = recv (GPOINTER_TO_INT (rdhandle), buffer, sizeof (buffer), 0);
		if (received == 0)
			break;
		if (received == SOCKET_ERROR) {
			if (WSAGetLastError () != WSAEINTR && WSAGetLastError () != WSAEWOULDBLOCK) {
				g_snprintf (error, sizeof (error), "selector_thread_wakeup_drain_pipes: recv () failed, error (%d)\n", WSAGetLastError ());
				mono_raise_exception (mono_get_exception_io (error));
				break;
			}
			break;
		}
#endif
	}
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_Init_internal (MonoThreadPoolIO *threadpool_io, gpointer wakeup_rdhandle)
{
	gchar error [256];

#if defined(HAVE_EPOLL)
	threadpool_io->backend = &backend_epoll;
#elif defined(HAVE_KQUEUE)
	threadpool_io->backend = &backend_kqueue;
#else
	threadpool_io->backend = &backend_poll;
#endif
	if (g_getenv ("MONO_DISABLE_AIO") != NULL)
		threadpool_io->backend = &backend_poll;

	if (threadpool_io->backend->init (&threadpool_io->backend_data, GPOINTER_TO_INT (wakeup_rdhandle), error, sizeof (error)) == -1) {
		mono_raise_exception (mono_get_exception_io (error));
	}
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_Cleanup_internal (MonoThreadPoolIO *threadpool_io)
{
	threadpool_io->backend->cleanup (threadpool_io->backend_data);
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_UpdateAdd_internal (MonoThreadPoolIO *threadpool_io, MonoThreadPoolIOUpdate update)
{
	gchar error [256];

	if (threadpool_io->backend->update_add (threadpool_io->backend_data, &update, error, sizeof (error)) == -1) {
		mono_raise_exception (mono_get_exception_io (error));
	}
}

gint
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventWait_internal (MonoThreadPoolIO *threadpool_io)
{
	gchar error [256];
	gint ready = threadpool_io->backend->event_wait (threadpool_io->backend_data, error, sizeof (error));
	if (ready == -1) {
		mono_raise_exception (mono_get_exception_io (error));
	}

	return ready;
}

gint
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventGetHandleMax_internal (MonoThreadPoolIO *threadpool_io)
{
	return threadpool_io->backend->event_get_fd_max (threadpool_io->backend_data);
}

gpointer
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventGetHandleAt_internal (MonoThreadPoolIO *threadpool_io, gint i, gint *operations)
{
	return GINT_TO_POINTER (threadpool_io->backend->event_get_fd_at (threadpool_io->backend_data, i, operations));
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventResetHandleAt_internal (MonoThreadPoolIO *threadpool_io, gint i, gint operations)
{
	gchar error [256];

	if (threadpool_io->backend->event_reset_fd_at (threadpool_io->backend_data, i, operations, error, sizeof (error)) == -1) {
		mono_raise_exception (mono_get_exception_io (error));
	}
}

#else

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadCreateWakeupPipes_internal (gpointer *rdhandle, gpointer *wrhandle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadWakeup_internal (gpointer wrhandle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadDrainWakeupPipes_internal (gpointer rdhandle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_Init_internal (MonoThreadPoolIO *threadpool_io, gpointer wakeup_handle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_Cleanup_internal (MonoThreadPoolIO *threadpool_io)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_UpdateAdd_internal (MonoThreadPoolIO *threadpool_io, MonoThreadPoolIOUpdate update)
{
	g_assert_not_reached ();
}

gint
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventWait_internal (MonoThreadPoolIO *threadpool_io)
{
	g_assert_not_reached ();
}

gint
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventGetHandleMax_internal (MonoThreadPoolIO *threadpool_io)
{
	g_assert_not_reached ();
}

gpointer
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventGetHandleAt_internal (MonoThreadPoolIO *threadpool_io, gint i, gint *operations)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventResetHandleAt_internal (MonoThreadPoolIO *threadpool_io, gint i, gint operations)
{
	g_assert_not_reached ();
}

#endif
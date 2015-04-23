/*
 * io-selector.c: Microsoft IO threadpool runtime support
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

#include <mono/metadata/exception.h>
#include <mono/metadata/io-selector.h>

/* Keep in sync with System.Threading.IOOperation */
enum {
	IO_OP_UNDEFINED = 0,
	IO_OP_IN        = 1,
	IO_OP_OUT       = 2,
};

/* Keep in sync with System.Threading.IOSelector.Update */
struct _MonoIOSelectorUpdate {
	gpointer fd;
	gint operations;
	MonoBoolean is_new;
};

typedef struct {
	gint (*init) (gpointer *backend_data, gint wakeup_pipe_fd, gchar *error, gint error_size);
	void (*cleanup) (gpointer backend_data);
	gint (*update_add) (gpointer backend_data, MonoIOSelectorUpdate *update, gchar *error, gint error_size);
	gint (*event_wait) (gpointer backend_data, gchar *error, gint error_size);
	gint (*event_get_fd_max) (gpointer backend_data);
	gint (*event_get_fd_at) (gpointer backend_data, gint i, gint *operations);
	gint (*event_reset_fd_at) (gpointer backend_data, gint i, gint operations, gchar *error, gint error_size);
} IOSelectorBackend;

#include "io-selector-epoll.c"
#include "io-selector-kqueue.c"
#include "io-selector-poll.c"

/* Keep in sync with System.Threading.IOSelector */
struct _MonoIOSelector {
	MonoObject obj;
	IOSelectorBackend *backend;
	gpointer backend_data;
};

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadCreateWakeupPipes_internal (gpointer *rdhandle, gpointer *wrhandle)
{
	gchar msg [256];

	g_assert (rdhandle);
	g_assert (wrhandle);

#if !defined(HOST_WIN32)
	gint wakeup_pipes [2];

	if (pipe (wakeup_pipes) == -1) {
		g_snprintf (msg, sizeof (msg), "selector_thread_create_wakeup_pipes: pipe () failed, error (%d) %s\n", errno, g_strerror (errno));
		mono_raise_exception (mono_get_exception_io (msg));
	}
	if (fcntl (wakeup_pipes [0], F_SETFL, O_NONBLOCK) == -1) {
		g_snprintf (msg, sizeof (msg), "selector_thread_create_wakeup_pipes: fcntl () failed, error (%d) %s\n", errno, g_strerror (errno));
		mono_raise_exception (mono_get_exception_io (msg));
	}
#else
	SOCKET wakeup_pipes [2];
	struct sockaddr_in client;
	struct sockaddr_in server;
	SOCKET server_sock;
	gulong arg;
	gint size;

	server_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	g_assert (server_sock != INVALID_SOCKET);
	wakeup_pipes [1] = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	g_assert (wakeup_pipes [1] != INVALID_SOCKET);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr ("127.0.0.1");
	server.sin_port = 0;
	if (bind (server_sock, (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (msg, sizeof (msg), "selector_thread_create_wakeup_pipes: bind () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (msg));
	}

	size = sizeof (server);
	if (getsockname (server_sock, (SOCKADDR*) &server, &size) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (msg, sizeof (msg), "selector_thread_create_wakeup_pipes: getsockname () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (msg));
	}
	if (listen (server_sock, 1024) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (msg, sizeof (msg), "selector_thread_create_wakeup_pipes: listen () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (msg));
	}
	if (connect ((SOCKET) wakeup_pipes [1], (SOCKADDR*) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket (server_sock);
		g_snprintf (msg, sizeof (msg), "selector_thread_create_wakeup_pipes: connect () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (msg));
	}

	size = sizeof (client);
	wakeup_pipes [0] = accept (server_sock, (SOCKADDR *) &client, &size);
	g_assert (wakeup_pipes [0] != INVALID_SOCKET);

	arg = 1;
	if (ioctlsocket (wakeup_pipes [0], FIONBIO, &arg) == SOCKET_ERROR) {
		closesocket (wakeup_pipes [0]);
		closesocket (server_sock);
		g_snprintf (msg, sizeof (msg), "selector_thread_create_wakeup_pipes: ioctlsocket () failed, error (%d)\n", WSAGetLastError ());
		mono_raise_exception (mono_get_exception_io (msg));
	}

	closesocket (server_sock);
#endif

	*rdhandle = GINT_TO_POINTER (wakeup_pipes [0]);
	*wrhandle = GINT_TO_POINTER (wakeup_pipes [1]);
}

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadWakeup_internal (gpointer wrhandle)
{
	gchar c = 'c';
	gchar msg [256];
	gint written;

	for (;;) {
#if !defined(HOST_WIN32)
		written = write (GPOINTER_TO_INT (wrhandle), &c, sizeof (c));
		if (written == sizeof (c))
			break;
		if (written == -1) {
			g_snprintf (msg, sizeof (msg), "selector_thread_wakeup: write () failed, error (%d) %s\n", errno, g_strerror (errno));
			mono_raise_exception (mono_get_exception_io (msg));
			break;
		}
#else
		written = send (GPOINTER_TO_INT (wrhandle), &c, sizeof (c), 0);
		if (written == sizeof (c))
			break;
		if (written == SOCKET_ERROR) {
			g_snprintf (msg, sizeof (msg), "selector_thread_wakeup: send () failed, error (%d)\n", WSAGetLastError ());
			mono_raise_exception (mono_get_exception_io (msg));
			break;
		}
#endif
	}
}

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadDrainWakeupPipes_internal (gpointer rdhandle)
{
	gchar buffer [128], msg [256];
	gint received;

	for (;;) {
#if !defined(HOST_WIN32)
		received = read (GPOINTER_TO_INT (rdhandle), buffer, sizeof (buffer));
		if (received == 0)
			break;
		if (received == -1) {
			if (errno != EINTR && errno != EAGAIN) {
				g_snprintf (msg, sizeof (msg), "selector_thread_wakeup_drain_pipes: read () failed, error (%d) %s\n", errno, g_strerror (errno));
				mono_raise_exception (mono_get_exception_io (msg));
			}
			break;
		}
#else
		received = recv (GPOINTER_TO_INT (rdhandle), buffer, sizeof (buffer), 0);
		if (received == 0)
			break;
		if (received == SOCKET_ERROR) {
			if (WSAGetLastError () != WSAEINTR && WSAGetLastError () != WSAEWOULDBLOCK) {
				g_snprintf (msg, sizeof (msg), "selector_thread_wakeup_drain_pipes: recv () failed, error (%d)\n", WSAGetLastError ());
				mono_raise_exception (mono_get_exception_io (msg));
			}
			break;
		}
#endif
	}
}

void
ves_icall_System_Threading_NativePollingIOSelector_Init_internal (MonoIOSelector *ioselector, gpointer wakeup_handle)
{
	gchar error [256];

#if defined(HAVE_EPOLL)
	ioselector->backend = &backend_epoll;
#elif defined(HAVE_KQUEUE)
	ioselector->backend = &backend_kqueue;
#else
	ioselector->backend = &backend_poll;
#endif
	if (g_getenv ("MONO_DISABLE_AIO") != NULL)
		ioselector->backend = &backend_poll;

	if (ioselector->backend->init (&ioselector->backend_data, GPOINTER_TO_INT (wakeup_handle), error, sizeof (error)) == -1) {
		gchar msg [sizeof (error) + 64];
		g_snprintf (msg, sizeof (msg), "ioselector->backend->init () failed, error : '%s'", error);
		mono_raise_exception (mono_get_exception_io (msg));
		return;
	}
}

void
ves_icall_System_Threading_NativePollingIOSelector_Cleanup_internal (MonoIOSelector *ioselector)
{
	ioselector->backend->cleanup (ioselector->backend_data);
}

void
ves_icall_System_Threading_NativePollingIOSelector_UpdateAdd_internal (MonoIOSelector *ioselector, MonoIOSelectorUpdate update)
{
	gchar error [256];

	if (ioselector->backend->update_add (ioselector->backend_data, &update, error, sizeof (error)) == -1) {
		gchar msg [sizeof (error) + 64];
		g_snprintf (msg, sizeof (msg), "ioselector->backend->update_add () failed, error : '%s'", error);
		mono_raise_exception (mono_get_exception_io (msg));
		return;
	}
}

gint
ves_icall_System_Threading_NativePollingIOSelector_EventWait_internal (MonoIOSelector *ioselector)
{
	gchar error [256];
	gint ready;

	ready = ioselector->backend->event_wait (ioselector->backend_data, error, sizeof (error));
	if (ready == -1) {
		gchar msg [sizeof (error) + 64];
		g_snprintf (msg, sizeof (msg), "ioselector->backend->event_wait () failed, error : '%s'", error);
		mono_raise_exception (mono_get_exception_io (msg));
		return -1;
	}

	return ready;
}

gint
ves_icall_System_Threading_NativePollingIOSelector_EventGetHandleMax_internal (MonoIOSelector *ioselector)
{
	return ioselector->backend->event_get_fd_max (ioselector->backend_data);
}

gpointer
ves_icall_System_Threading_NativePollingIOSelector_EventGetHandleAt_internal (MonoIOSelector *ioselector, gint i, gint *operations)
{
	return GINT_TO_POINTER (ioselector->backend->event_get_fd_at (ioselector->backend_data, i, operations));
}

void
ves_icall_System_Threading_NativePollingIOSelector_EventResetHandleAt_internal (MonoIOSelector *ioselector, gint i, gint operations)
{
	char error [256];

	if (ioselector->backend->event_reset_fd_at (ioselector->backend_data, i, operations, error, sizeof (error)) == -1) {
		gchar msg [sizeof (error) + 64];
		g_snprintf (msg, sizeof (msg), "ioselector->backend->event_reset_fd_at () failed, error : '%s'", error);
		mono_raise_exception (mono_get_exception_io (msg));
		return;
	}
}

#else

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadCreateWakeupPipes_internal (gpointer *rdhandle, gpointer *wrhandle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadWakeup_internal (gpointer wrhandle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadDrainWakeupPipes_internal (gpointer rdhandle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativePollingIOSelector_Init_internal (MonoIOSelector *ioselector, gpointer wakeup_handle)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativePollingIOSelector_Cleanup_internal (MonoIOSelector *ioselector)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativePollingIOSelector_UpdateAdd_internal (MonoIOSelector *ioselector, MonoIOSelectorUpdate update)
{
	g_assert_not_reached ();
}

gint
ves_icall_System_Threading_NativePollingIOSelector_EventWait_internal (MonoIOSelector *ioselector)
{
	g_assert_not_reached ();
}

gint
ves_icall_System_Threading_NativePollingIOSelector_EventGetHandleMax_internal (MonoIOSelector *ioselector)
{
	g_assert_not_reached ();
}

gpointer
ves_icall_System_Threading_NativePollingIOSelector_EventGetHandleAt_internal (MonoIOSelector *ioselector, gint i, gint *operations)
{
	g_assert_not_reached ();
}

void
ves_icall_System_Threading_NativePollingIOSelector_EventResetHandleAt_internal (MonoIOSelector *ioselector, gint i, gint operations)
{
	g_assert_not_reached ();
}

#endif
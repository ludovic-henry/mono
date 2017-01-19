/*
 * w32socket-win32.c: Windows specific socket code.
 *
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <glib.h>

#include <string.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include <sys/types.h>

#include "w32socket.h"
#include "w32socket-internals.h"

#include "utils/w32api.h"

#define LOGDEBUG(...)  

void
mono_w32socket_initialize (void)
{
}

void
mono_w32socket_cleanup (void)
{
}

static gboolean set_blocking (MonoSocket sock, gboolean block)
{
	u_long non_block = block ? 0 : 1;
	return ioctlsocket (sock, FIONBIO, &non_block) != SOCKET_ERROR;
}

static DWORD get_socket_timeout (MonoSocket sock, int optname)
{
	DWORD timeout = 0;
	int optlen = sizeof (DWORD);
	if (getsockopt (sock, SOL_SOCKET, optname, (char *)&timeout, &optlen) == SOCKET_ERROR) {
		WSASetLastError (0);
		return WSA_INFINITE;
	}
	if (timeout == 0)
		timeout = WSA_INFINITE; // 0 means infinite
	return timeout;
}

/*
* Performs an alertable wait for the specified event (FD_ACCEPT_BIT,
* FD_CONNECT_BIT, FD_READ_BIT, FD_WRITE_BIT) on the specified socket.
* Returns TRUE if the event is fired without errors. Calls WSASetLastError()
* with WSAEINTR and returns FALSE if the thread is alerted. If the event is
* fired but with an error WSASetLastError() is called to set the error and the
* function returns FALSE.
*/
static gboolean alertable_socket_wait (MonoSocket sock, int event_bit)
{
	static char *EVENT_NAMES[] = { "FD_READ", "FD_WRITE", NULL /*FD_OOB*/, "FD_ACCEPT", "FD_CONNECT", "FD_CLOSE" };
	gboolean success = FALSE;
	int error = -1;
	DWORD timeout = WSA_INFINITE;
	if (event_bit == FD_READ_BIT || event_bit == FD_WRITE_BIT) {
		timeout = get_socket_timeout (sock, event_bit == FD_READ_BIT ? SO_RCVTIMEO : SO_SNDTIMEO);
	}
	WSASetLastError (0);
	WSAEVENT event = WSACreateEvent ();
	if (event != WSA_INVALID_EVENT) {
		if (WSAEventSelect (sock, event, (1 << event_bit) | FD_CLOSE) != SOCKET_ERROR) {
			LOGDEBUG (g_message ("%06d - Calling WSAWaitForMultipleEvents () on socket %d", GetCurrentThreadId (), sock));
			DWORD ret = WSAWaitForMultipleEvents (1, &event, TRUE, timeout, TRUE);
			if (ret == WSA_WAIT_IO_COMPLETION) {
				LOGDEBUG (g_message ("%06d - WSAWaitForMultipleEvents () returned WSA_WAIT_IO_COMPLETION for socket %d", GetCurrentThreadId (), sock));
				error = WSAEINTR;
			} else if (ret == WSA_WAIT_TIMEOUT) {
				error = WSAETIMEDOUT;
			} else {
				g_assert (ret == WSA_WAIT_EVENT_0);
				WSANETWORKEVENTS ne = { 0 };
				if (WSAEnumNetworkEvents (sock, event, &ne) != SOCKET_ERROR) {
					if (ne.lNetworkEvents & (1 << event_bit) && ne.iErrorCode[event_bit]) {
						LOGDEBUG (g_message ("%06d - %s error %d on socket %d", GetCurrentThreadId (), EVENT_NAMES[event_bit], ne.iErrorCode[event_bit], sock));
						error = ne.iErrorCode[event_bit];
					} else if (ne.lNetworkEvents & FD_CLOSE_BIT && ne.iErrorCode[FD_CLOSE_BIT]) {
						LOGDEBUG (g_message ("%06d - FD_CLOSE error %d on socket %d", GetCurrentThreadId (), ne.iErrorCode[FD_CLOSE_BIT], sock));
						error = ne.iErrorCode[FD_CLOSE_BIT];
					} else {
						LOGDEBUG (g_message ("%06d - WSAEnumNetworkEvents () finished successfully on socket %d", GetCurrentThreadId (), sock));
						success = TRUE;
						error = 0;
					}
				}
			}
			WSAEventSelect (sock, NULL, 0);
		}
		WSACloseEvent (event);
	}
	if (error != -1) {
		WSASetLastError (error);
	}
	return success;
}

#define ALERTABLE_SOCKET_CALL(event_bit, blocking, repeat, ret, op, sock, ...) \
	LOGDEBUG (g_message ("%06d - Performing %s " #op " () on socket %d", GetCurrentThreadId (), blocking ? "blocking" : "non-blocking", sock)); \
	if (blocking) { \
		if (ret = set_blocking(sock, FALSE)) { \
			while (-1 == (int) (ret = op (sock, __VA_ARGS__))) { \
				int _error = WSAGetLastError ();\
				if (_error != WSAEWOULDBLOCK && _error != WSA_IO_PENDING) \
					break; \
				if (!alertable_socket_wait (sock, event_bit) || !repeat) \
					break; \
			} \
			int _saved_error = WSAGetLastError (); \
			set_blocking (sock, TRUE); \
			WSASetLastError (_saved_error); \
		} \
	} else { \
		ret = op (sock, __VA_ARGS__); \
	} \
	int _saved_error = WSAGetLastError (); \
	LOGDEBUG (g_message ("%06d - Finished %s " #op " () on socket %d (ret = %d, WSAGetLastError() = %d)", GetCurrentThreadId (), \
		blocking ? "blocking" : "non-blocking", sock, ret, _saved_error)); \
	WSASetLastError (_saved_error);

gint
mono_w32socket_disconnect (MonoSocket sock, gboolean reuse)
{
	LPFN_DISCONNECTEX disconnect;
	LPFN_TRANSMITFILE transmit_file;
	DWORD output_bytes;
	gint ret;

	/* Use the SIO_GET_EXTENSION_FUNCTION_POINTER to determine
	 * the address of the disconnect method without taking
	 * a hard dependency on a single provider
	 *
	 * For an explanation of why this is done, you can read the
	 * article at http://www.codeproject.com/internet/jbsocketserver3.asp
	 *
	 * I _think_ the extension function pointers need to be looked
	 * up for each socket.
	 *
	 * FIXME: check the best way to store pointers to functions in
	 * managed objects that still works on 64bit platforms. */

	GUID disconnect_guid = WSAID_DISCONNECTEX;
	ret = WSAIoctl (sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &disconnect_guid, sizeof (GUID), &disconnect, sizeof (LPFN_DISCONNECTEX), &output_bytes, NULL, NULL);
	if (ret == 0) {
		if (!disconnect (sock, NULL, reuse ? TF_REUSE_SOCKET : 0, 0))
			return WSAGetLastError ();

		return 0;
	}

	GUID transmit_file_guid = WSAID_TRANSMITFILE;
	ret = WSAIoctl (sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &transmit_file_guid, sizeof (GUID), &transmit_file, sizeof (LPFN_TRANSMITFILE), &output_bytes, NULL, NULL);
	if (ret == 0) {
		if (!transmit_file (sock, NULL, 0, 0, NULL, NULL, TF_DISCONNECT | (reuse ? TF_REUSE_SOCKET : 0)))
			return WSAGetLastError ();

		return 0;
	}

	return ERROR_NOT_SUPPORTED;
}

void
mono_w32socket_set_last_error (gint32 error)
{
	WSASetLastError (error);
}

gint32
mono_w32socket_get_last_error (void)
{
	return WSAGetLastError ();
}

gint32
mono_w32socket_convert_error (gint error)
{
	return (error > 0 && error < WSABASEERR) ? error + WSABASEERR : error;
}

gboolean
ves_icall_System_Net_Sockets_Socket_SupportPortReuse (MonoProtocolType proto)
{
	return TRUE;
}

gsize
ves_icall_Mono_PAL_Sockets_Win32_Accept (gsize sock, MonoBoolean blocking, gint32 *error)
{
	MonoInternalThread *internal;
	gsize ret;

	*werror = 0;

	internal = mono_thread_internal_current ();
	internal->interrupt_on_stop = (gpointer)TRUE;

	MONO_ENTER_GC_SAFE;

	ALERTABLE_SOCKET_CALL (FD_ACCEPT_BIT, blocking, TRUE, ret, accept, (MonoSocket) sock, NULL, 0);
	if (ret == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;

	internal->interrupt_on_stop = (gpointer)FALSE;

	return ret;
}

void
ves_icall_Mono_PAL_Sockets_Win32_Connect (gsize sock, gpointer addr, gint addr_size, gint32 *werror, MonoBoolean blocking)
{
	gint res;

	*werror = 0;

	MONO_ENTER_GC_SAFE;

	ALERTABLE_SOCKET_CALL (FD_CONNECT_BIT, blocking, FALSE, res, connect, (MonoSocket) sock, (struct sockaddr*) addr, (socklen_t) addr_size);
	if (res == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;
}

gint32
ves_icall_Mono_PAL_Sockets_Win32_Receive (gsize sock, gpointer buffer, gint32 len, gint32 flags, MonoBoolean blocking, gint32 *werror)
{
	MonoInternalThread *internal;
	gint32 ret;

	*werror = 0;

	internal = mono_thread_internal_current ();
	internal->interrupt_on_stop = (gpointer)TRUE;

	MONO_ENTER_GC_SAFE;

	ALERTABLE_SOCKET_CALL (FD_READ_BIT, blocking, TRUE, ret, recv, (MonoSocket) sock, (gchar*) buffer, len, flags);
	if (ret == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;

	internal->interrupt_on_stop = (gpointer)FALSE;

	return ret;
}

gint32
ves_icall_Mono_PAL_Sockets_Win32_ReceiveBuffers (gsize sock, gpointer buffers, gint32 count, gint32 flags, MonoBoolean blocking, gint32 *werror)
{
	guint32 recv;
	gint32 ret;

	*werror = 0;

	MONO_ENTER_GC_SAFE;

	ret = ALERTABLE_SOCKET_CALL (FD_READ_BIT, blocking, TRUE, ret, WSARecv, (MonoSocket) sock, (WSABUF*) buffers, count, &recv, &flags, NULL, NULL);
	if (ret == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;

	return recv;
}

gint32
ves_icall_Mono_PAL_Sockets_Win32_ReceiveFrom (gsize sock, gpointer buffer, gint32 len, gint32 flags,
		gpointer sa, gint32 sa_size, MonoBoolean blocking, gint32 *werror)
{
	gint32 ret;

	*werror = 0;

	MONO_ENTER_GC_SAFE;

	ALERTABLE_SOCKET_CALL (FD_READ_BIT, blocking, TRUE, ret, recvfrom, (MonoSocket) sock, (gchar*) buffer, len, flags, (struct sockaddr*) sa, (socklen_t) sa_size);
	if (ret == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;

	return ret;
}

gint32
ves_icall_Mono_PAL_Sockets_Win32_Send (gsize sock, gpointer buffer, gint32 len, gint32 flags, MonoBoolean blocking, gint32 *werror)
{
	gint32 ret;

	*werror = 0;

	MONO_ENTER_GC_SAFE;

	ALERTABLE_SOCKET_CALL (FD_WRITE_BIT, blocking, FALSE, ret, send, sock, (gchar*) buffer, count, flags);
	if (ret == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;

	return ret;
}

gint32
ves_icall_Mono_PAL_Sockets_Win32_SendBuffers (gsize sock, gpointer buffers, gint32 count, gint32 flags, MonoBoolean blocking, gint32 *werror)
{
	guint32 sent;
	gint32 ret;

	*werror = 0;

	MONO_ENTER_GC_SAFE;

	ALERTABLE_SOCKET_CALL (FD_WRITE_BIT, blocking, FALSE, ret, WSASend, sock, (WSABUF*) buffers, count, &sent, flags, NULL, NULL);
	if (ret == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;

	return sent;
}

gint32
ves_icall_Mono_PAL_Sockets_Win32_SendTo (gsize sock, gpointer buffer, gint32 len, gint32 flags,
		gpointer sa, gint32 sa_size, MonoBoolean blocking, gint32 *werror)
{
	gint32 ret;

	*werror = 0;

	MONO_ENTER_GC_SAFE;

	ALERTABLE_SOCKET_CALL (FD_READ_BIT, blocking, TRUE, ret, sendto, (MonoSocket) sock, (gchar*) buffer, len, flags, (struct sockaddr*) sa, (socklen_t) sa_size);
	if (ret == SOCKET_ERROR)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;

	return ret;
}

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT | HAVE_UWP_WINAPI_SUPPORT)

static gboolean
mono_w32socket_transmit_file (MonoSocket hSocket, gpointer hFile, TRANSMIT_FILE_BUFFERS *lpTransmitBuffers, guint32 dwReserved)
{
	LOGDEBUG (g_message ("%06d - Performing %s TransmitFile () on socket %d", GetCurrentThreadId (), blocking ? "blocking" : "non-blocking", hSocket));

	int error = 0;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = WSACreateEvent ();
	if (overlapped.hEvent == WSA_INVALID_EVENT)
		return FALSE;
	if (!TransmitFile (hSocket, hFile, 0, 0, &overlapped, lpTransmitBuffers, dwReserved)) {
		error = WSAGetLastError ();
		if (error == WSA_IO_PENDING) {
			error = 0;
			// NOTE: .NET's Socket.SendFile() doesn't honor the Socket's SendTimeout so we shouldn't either
			DWORD ret = WaitForSingleObjectEx (overlapped.hEvent, INFINITE, TRUE);
			if (ret == WAIT_IO_COMPLETION) {
				LOGDEBUG (g_message ("%06d - WaitForSingleObjectEx () returned WSA_WAIT_IO_COMPLETION for socket %d", GetCurrentThreadId (), hSocket));
				error = WSAEINTR;
			} else if (ret == WAIT_TIMEOUT) {
				error = WSAETIMEDOUT;
			} else if (ret != WAIT_OBJECT_0) {
				error = GetLastError ();
			}
		}
	}
	WSACloseEvent (overlapped.hEvent);

	LOGDEBUG (g_message ("%06d - Finished %s TransmitFile () on socket %d (ret = %d, WSAGetLastError() = %d)", GetCurrentThreadId (), \
		blocking ? "blocking" : "non-blocking", hSocket, error == 0, error));
	WSASetLastError (error);

	return error == 0;
}

void
ves_icall_Mono_PAL_Sockets_Win32_SendFile (gsize sock, gpointer file, gpointer buffers, gint flags, gint32 *werror)
{
	gboolean res;

	*werror = 0;

	MONO_ENTER_GC_SAFE;

	res = mono_w32socket_transmit_file ((MonoSocket) sock, file, (TRANSMIT_FILE_BUFFERS*) buffers, flags);
	if (!res)
		*werror = WSAGetLastError ();

	MONO_EXIT_GC_SAFE;
}

#endif /* G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT | HAVE_UWP_WINAPI_SUPPORT) */

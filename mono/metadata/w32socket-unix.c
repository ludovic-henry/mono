/*
 * w32socket-unix.c: Unix specific socket code.
 *
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <glib.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <arpa/inet.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>     /* defines FIONBIO and FIONREAD */
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>    /* defines SIOCATMARK */
#endif
#ifndef HAVE_MSG_NOSIGNAL
#include <signal.h>
#endif
#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include "w32socket.h"
#include "w32socket-internals.h"
#include "w32error.h"
#include "w32handle.h"
#include "assembly.h"
#include "exception.h"
#include "utils/mono-logger-internals.h"
#include "utils/mono-poll.h"
#include "utils/mono-memory-model.h"
#include "utils/networking.h"

#define LOGDEBUG(...)
/* define LOGDEBUG(...) g_message(__VA_ARGS__)  */

typedef struct {
	int domain;
	int type;
	int protocol;
	int saved_error;
	int still_readable;
} MonoW32HandleSocket;

typedef struct {
	guint32 len;
	gpointer buf;
} WSABUF;

typedef struct {
	gpointer Head;
	guint32 HeadLength;
	gpointer Tail;
	guint32 TailLength;
} TRANSMIT_FILE_BUFFERS;

static guint32 in_cleanup = 0;

static void
socket_close (gpointer handle, gpointer data)
{
	int ret;
	MonoW32HandleSocket *socket_handle = (MonoW32HandleSocket *)data;
	MonoThreadInfo *info = mono_thread_info_current ();

	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: closing socket handle %p", __func__, handle);

	/* Shutdown the socket for reading, to interrupt any potential
	 * receives that may be blocking for data.  See bug 75705. */
	shutdown (GPOINTER_TO_UINT (handle), SHUT_RD);

	do {
		ret = close (GPOINTER_TO_UINT(handle));
	} while (ret == -1 && errno == EINTR &&
		 !mono_thread_info_is_interrupt_state (info));

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: close error: %s", __func__, g_strerror (errno));
		if (!in_cleanup)
			mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
	}

	if (!in_cleanup)
		socket_handle->saved_error = 0;
}

static void
socket_details (gpointer data)
{
	/* FIXME: do something */
}

static const gchar*
socket_typename (void)
{
	return "Socket";
}

static gsize
socket_typesize (void)
{
	return sizeof (MonoW32HandleSocket);
}

static MonoW32HandleOps ops = {
	socket_close,    /* close */
	NULL,            /* signal */
	NULL,            /* own */
	NULL,            /* is_owned */
	NULL,            /* special_wait */
	NULL,            /* prewait */
	socket_details,  /* details */
	socket_typename, /* typename */
	socket_typesize, /* typesize */
};

void
mono_w32socket_initialize (void)
{
	mono_w32handle_register_ops (MONO_W32HANDLE_SOCKET, &ops);
}

static gboolean
cleanup_close (gpointer handle, gpointer data, gpointer user_data)
{
	if (mono_w32handle_get_type (handle) == MONO_W32HANDLE_SOCKET)
		mono_w32handle_force_close (handle, data);

	return FALSE;
}

void
mono_w32socket_cleanup (void)
{
	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: cleaning up", __func__);

	in_cleanup = 1;
	mono_w32handle_foreach (cleanup_close, NULL);
	in_cleanup = 0;
}

static MonoSocket
mono_w32socket_accept (MonoSocket sock)
{
	gpointer new_handle;
	MonoW32HandleSocket *socket_handle;
	MonoW32HandleSocket new_socket_handle;
	MonoSocket new_fd;
	MonoThreadInfo *info;

	if (!mono_w32handle_lookup (GUINT_TO_POINTER (sock), MONO_W32HANDLE_SOCKET, (gpointer *)&socket_handle)) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return INVALID_SOCKET;
	}

	info = mono_thread_info_current ();

	MONO_ENTER_GC_SAFE;

	do {
		new_fd = accept (sock, NULL, 0);
	} while (new_fd == -1 && errno == EINTR && !mono_thread_info_is_interrupt_state (info));

	MONO_EXIT_GC_SAFE;

	if (new_fd == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: accept error: %s", __func__, g_strerror(errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return INVALID_SOCKET;
	}

	if (new_fd >= mono_w32handle_fd_reserve) {
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: File descriptor is too big", __func__);

		mono_w32socket_set_last_error (WSASYSCALLFAILURE);

		close (new_fd);

		return INVALID_SOCKET;
	}

	new_socket_handle.domain = socket_handle->domain;
	new_socket_handle.type = socket_handle->type;
	new_socket_handle.protocol = socket_handle->protocol;
	new_socket_handle.still_readable = 1;

	new_handle = mono_w32handle_new_fd (MONO_W32HANDLE_SOCKET, new_fd, &new_socket_handle);
	if (new_handle == INVALID_HANDLE_VALUE) {
		g_warning ("%s: error creating socket handle", __func__);
		mono_w32socket_set_last_error (ERROR_GEN_FAILURE);
		return INVALID_SOCKET;
	}

	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: returning newly accepted socket handle %p with", __func__, new_handle);

	return new_fd;
}

static int
mono_w32socket_connect (MonoSocket sock, const struct sockaddr *addr, int addrlen)
{
	gpointer handle;
	MonoW32HandleSocket *socket_handle;
	gint res;

	handle = GUINT_TO_POINTER (sock);
	if (!mono_w32handle_lookup (handle, MONO_W32HANDLE_SOCKET, (gpointer *)&socket_handle)) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	MONO_ENTER_GC_SAFE;
	res = connect (sock, addr, addrlen);
	MONO_EXIT_GC_SAFE;

	if (res == -1) {
		MonoThreadInfo *info;
		mono_pollfd fds;
		gint errnum, so_error;
		socklen_t len;

		errnum = errno;

		if (errno != EINTR) {
			mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: connect error: %s", __func__, g_strerror (errnum));

			errnum = mono_w32socket_convert_error (errnum);
			if (errnum == WSAEINPROGRESS)
				errnum = WSAEWOULDBLOCK; /* see bug #73053 */

			mono_w32socket_set_last_error (errnum);

			/*
			 * On solaris x86 getsockopt (SO_ERROR) is not set after
			 * connect () fails so we need to save this error.
			 *
			 * But don't do this for EWOULDBLOCK (bug 317315)
			 */
			if (errnum != WSAEWOULDBLOCK) {
				/* ECONNRESET means the socket was closed by another thread */
				/* Async close on mac raises ECONNABORTED. */
				socket_handle->saved_error = errnum;
			}
			return SOCKET_ERROR;
		}

		info = mono_thread_info_current ();

		fds.fd = sock;
		fds.events = MONO_POLLOUT;

		for (;;) {
			MONO_ENTER_GC_SAFE;
			res = mono_poll (&fds, 1, -1);
			MONO_EXIT_GC_SAFE;

			if (res != -1 || mono_thread_info_is_interrupt_state (info))
				break;

			if (errno != EINTR) {
				gint errnum = errno;
				mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: connect poll error: %s", __func__, g_strerror (errno));
				mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
				return SOCKET_ERROR;
			}
		}

		len = sizeof(so_error);
		if (getsockopt (sock, SOL_SOCKET, SO_ERROR, &so_error, &len) == -1) {
			gint errnum = errno;
			mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: connect getsockopt error: %s", __func__, g_strerror (errno));
			mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
			return SOCKET_ERROR;
		}

		if (so_error != 0) {
			gint errnum = mono_w32socket_convert_error (so_error);

			/* Need to save this socket error */
			socket_handle->saved_error = errnum;

			mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: connect getsockopt returned error: %s", __func__, g_strerror (so_error));

			mono_w32socket_set_last_error (errnum);
			return SOCKET_ERROR;
		}
	}

	return 0;
}

static int
mono_w32socket_recvfrom (MonoSocket sock, char *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	gpointer handle;
	MonoW32HandleSocket *socket_handle;
	int ret;
	MonoThreadInfo *info;

	handle = GUINT_TO_POINTER (sock);
	if (!mono_w32handle_lookup (handle, MONO_W32HANDLE_SOCKET, (gpointer *)&socket_handle)) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	info = mono_thread_info_current ();

	MONO_ENTER_GC_SAFE;

	do {
		ret = recvfrom (sock, buf, len, flags, from, fromlen);
	} while (ret == -1 && errno == EINTR && !mono_thread_info_is_interrupt_state (info));

	MONO_EXIT_GC_SAFE;

	if (ret == 0 && len > 0) {
		/* According to the Linux man page, recvfrom only
		 * returns 0 when the socket has been shut down
		 * cleanly.  Turn this into an EINTR to simulate win32
		 * behaviour of returning EINTR when a socket is
		 * closed while the recvfrom is blocking (we use a
		 * shutdown() in socket_close() to trigger this.) See
		 * bug 75705.
		 */
		/* Distinguish between the socket being shut down at
		 * the local or remote ends, and reads that request 0
		 * bytes to be read
		 */

		/* If this returns FALSE, it means the socket has been
		 * closed locally.  If it returns TRUE, but
		 * still_readable != 1 then shutdown
		 * (SHUT_RD|SHUT_RDWR) has been called locally.
		 */
		if (socket_handle->still_readable != 1) {
			ret = -1;
			errno = EINTR;
		}
	}

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: recv error: %s", __func__, g_strerror(errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}
	return ret;
}

static void
wsabuf_to_msghdr (WSABUF *buffers, guint32 count, struct msghdr *hdr)
{
	guint32 i;

	memset (hdr, 0, sizeof (struct msghdr));
	hdr->msg_iovlen = count;
	hdr->msg_iov = g_new0 (struct iovec, count);
	for (i = 0; i < count; i++) {
		hdr->msg_iov [i].iov_base = buffers [i].buf;
		hdr->msg_iov [i].iov_len  = buffers [i].len;
	}
}

static void
msghdr_iov_free (struct msghdr *hdr)
{
	g_free (hdr->msg_iov);
}

static int
mono_w32socket_recvbuffers (MonoSocket sock, WSABUF *buffers, guint32 count, guint32 *received, guint32 *flags)
{
	MonoW32HandleSocket *socket_handle;
	MonoThreadInfo *info;
	gpointer handle;
	gint ret;
	struct msghdr hdr;

	handle = GUINT_TO_POINTER (sock);
	if (!mono_w32handle_lookup (handle, MONO_W32HANDLE_SOCKET, (gpointer *)&socket_handle)) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	info = mono_thread_info_current ();

	wsabuf_to_msghdr (buffers, count, &hdr);

	MONO_ENTER_GC_SAFE;

	do {
		ret = recvmsg (sock, &hdr, *flags);
	} while (ret == -1 && errno == EINTR && !mono_thread_info_is_interrupt_state (info));

	MONO_EXIT_GC_SAFE;

	msghdr_iov_free (&hdr);

	if (ret == 0) {
		/* see mono_w32socket_recvfrom */
		if (socket_handle->still_readable != 1) {
			ret = -1;
			errno = EINTR;
		}
	}

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: recvmsg error: %s", __func__, g_strerror(errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	*received = ret;
	*flags = hdr.msg_flags;

	return 0;
}

static int
mono_w32socket_send (MonoSocket sock, char *buf, int len, int flags)
{
	gpointer handle;
	int ret;
	MonoThreadInfo *info;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	info = mono_thread_info_current ();

	MONO_ENTER_GC_SAFE;

	do {
		ret = send (sock, buf, len, flags);
	} while (ret == -1 && errno == EINTR && !mono_thread_info_is_interrupt_state (info));

	MONO_EXIT_GC_SAFE;

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: send error: %s", __func__, g_strerror (errno));

#ifdef O_NONBLOCK
		/* At least linux returns EAGAIN/EWOULDBLOCK when the timeout has been set on
		 * a blocking socket. See bug #599488 */
		if (errnum == EAGAIN) {
			ret = fcntl (sock, F_GETFL, 0);
			if (ret != -1 && (ret & O_NONBLOCK) == 0)
				errnum = ETIMEDOUT;
		}
#endif /* O_NONBLOCK */
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}
	return ret;
}

static int
mono_w32socket_sendto (MonoSocket sock, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
	gpointer handle;
	int ret;
	MonoThreadInfo *info;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	info = mono_thread_info_current ();

	MONO_ENTER_GC_SAFE;

	do {
		ret = sendto (sock, buf, len, flags, to, tolen);
	} while (ret == -1 && errno == EINTR &&  !mono_thread_info_is_interrupt_state (info));

	MONO_EXIT_GC_SAFE;

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: send error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}
	return ret;
}

static int
mono_w32socket_sendbuffers (MonoSocket sock, WSABUF *buffers, guint32 count, guint32 *sent, guint32 flags)
{
	struct msghdr hdr;
	MonoThreadInfo *info;
	gpointer handle;
	gint ret;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	info = mono_thread_info_current ();

	wsabuf_to_msghdr (buffers, count, &hdr);

	MONO_ENTER_GC_SAFE;

	do {
		ret = sendmsg (sock, &hdr, flags);
	} while (ret == -1 && errno == EINTR && !mono_thread_info_is_interrupt_state (info));

	MONO_EXIT_GC_SAFE;

	msghdr_iov_free (&hdr);

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: sendmsg error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	*sent = ret;
	return 0;
}

#define SF_BUFFER_SIZE	16384

static gboolean
mono_w32socket_transmit_file (MonoSocket sock, gpointer file_handle, TRANSMIT_FILE_BUFFERS *buffers, guint32 flags)
{
	MonoThreadInfo *info;
	gint file;
	gssize ret;

	if (mono_w32handle_get_type (GUINT_TO_POINTER (sock)) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return FALSE;
	}

	info = mono_thread_info_current ();

	/* Write the header */
	if (buffers && buffers->Head && buffers->HeadLength > 0) {
		gssize sent, len;
		gchar *buffer;
		for (sent = 0, len = buffers->HeadLength, buffer = (gchar*) buffers->Head; sent < len;) {
			MONO_ENTER_GC_SAFE;
			ret = send (sock, buffer + sent, len - sent, 0);
			MONO_EXIT_GC_SAFE;

			if (ret == -1) {
				gint errnum = errno;
				if (errnum == EINTR && !mono_thread_info_is_interrupt_state (info))
					continue;
				mono_w32error_set_last (mono_w32socket_convert_error (errnum));
				return FALSE;
			}
			if (ret == 0) {
				mono_w32error_set_last (WSAESHUTDOWN);
				return FALSE;
			}

			sent += ret;
		}
	}

	file = GPOINTER_TO_INT (file_handle);

	{
#if defined(HAVE_SENDFILE) && (defined(__linux__) || defined(DARWIN))
		struct stat statbuf;

		ret = fstat (file, &statbuf);
		if (ret == -1) {
			mono_w32socket_set_last_error (mono_w32socket_convert_error (errno));
			return FALSE;
		}

#if defined(DARWIN)
		/* sendfile returns -1 on error, and 0 otherwise */

		MONO_ENTER_GC_SAFE;
		/* TODO: header/tail could be sent in the 5th argument */
		ret = sendfile (file, sock, 0, &statbuf.st_size, NULL, 0);
		MONO_EXIT_GC_SAFE;

		if (ret == -1) {
			gint errnum = errno;
			if (errnum == EINTR && !mono_thread_info_is_interrupt_state (info))
				goto retry_sendfile;
			mono_w32error_set_last (mono_w32socket_convert_error (errnum));
			return FALSE;
		}
#elif defined(__linux__)
		/* sendfile returns the number of byte sent, and does
		 * not guarantee that the whole file has been sent */

		gssize sent;
		for (sent = 0, len = statbuf.st_size; sent < len;) {
			MONO_ENTER_GC_SAFE;
			off_t offset = sent;
			ret = sendfile (sock, file, &offset, len - sent);
			MONO_EXIT_GC_SAFE;

			if (ret == -1) {
				gint errnum = errno;
				if (errnum == EINTR && !mono_thread_info_is_interrupt_state (info))
					goto retry_sendfile;
				mono_w32error_set_last (mono_w32socket_convert_error (errnum));
				return FALSE;
			}
			if (ret == 0) {
				mono_w32error_set_last (WSAESHUTDOWN);
				return FALSE;
			}
			sent += ret;
		}
#endif

#else
		gchar *buffer;

		buffer = g_malloc (SF_BUFFER_SIZE);

		for (;;) {
			gssize sent, len;

			MONO_ENTER_GC_SAFE;
			len = read (file, buffer, SF_BUFFER_SIZE);
			MONO_EXIT_GC_SAFE;

			if (len == -1) {
				gint errnum = errno;
				if (errnum == EINTR && !mono_thread_info_is_interrupt_state (info))
					continue;
				mono_w32error_set_last (mono_w32socket_convert_error (errnum));
				return FALSE;
			}
			if (len == 0)
				break;

			for (sent = 0; sent < len;) {
				MONO_ENTER_GC_SAFE;
				ret = send (sock, buffer + sent, len - sent, 0);
				MONO_EXIT_GC_SAFE;

				if (ret == -1) {
					gint errnum = errno;
					if (errnum == EINTR && !mono_thread_info_is_interrupt_state (info))
						continue;
					mono_w32error_set_last (mono_w32socket_convert_error (errnum));
					return FALSE;
				}
				if (ret == 0) {
					mono_w32error_set_last (WSAESHUTDOWN);
					return FALSE;
				}

				sent += ret;
			}
		}

		g_free (buffer);
#endif
	}

	/* Write the tail */
	if (buffers && buffers->Tail && buffers->TailLength > 0) {
		gssize sent, len;
		gchar *buffer;
		for (sent = 0, len = buffers->TailLength, buffer = (gchar*) buffers->Tail; sent < len;) {
			MONO_ENTER_GC_SAFE;
			ret = send (sock, buffer + sent, len - sent, 0);
			MONO_EXIT_GC_SAFE;
			if (ret == -1) {
				gint errnum = errno;
				if (errnum == EINTR && !mono_thread_info_is_interrupt_state (info))
					continue;
				mono_w32error_set_last (mono_w32socket_convert_error (errnum));
				return FALSE;
			}
			if (ret == 0) {
				mono_w32error_set_last (WSAESHUTDOWN);
				return FALSE;
			}
			sent += ret;
		}
	}

	if ((flags & TF_DISCONNECT) == TF_DISCONNECT)
		mono_w32handle_close (GUINT_TO_POINTER (sock));

	return TRUE;
}

static MonoSocket
mono_w32socket_socket (int domain, int type, int protocol)
{
	MonoW32HandleSocket socket_handle = {0};
	gpointer handle;
	MonoSocket sock;

	socket_handle.domain = domain;
	socket_handle.type = type;
	socket_handle.protocol = protocol;
	socket_handle.still_readable = 1;

	sock = socket (domain, type, protocol);
	if (sock == -1 && domain == AF_INET && type == SOCK_RAW &&
	    protocol == 0) {
		/* Retry with protocol == 4 (see bug #54565) */
		// https://bugzilla.novell.com/show_bug.cgi?id=MONO54565
		socket_handle.protocol = 4;
		sock = socket (AF_INET, SOCK_RAW, 4);
	}

	if (sock == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: socket error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return INVALID_SOCKET;
	}

	if (sock >= mono_w32handle_fd_reserve) {
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: File descriptor is too big (%d >= %d)",
			   __func__, sock, mono_w32handle_fd_reserve);

		mono_w32socket_set_last_error (WSASYSCALLFAILURE);
		close (sock);

		return INVALID_SOCKET;
	}

	/* .net seems to set this by default for SOCK_STREAM, not for
	 * SOCK_DGRAM (see bug #36322)
	 * https://bugzilla.novell.com/show_bug.cgi?id=MONO36322
	 *
	 * It seems winsock has a rather different idea of what
	 * SO_REUSEADDR means.  If it's set, then a new socket can be
	 * bound over an existing listening socket.  There's a new
	 * windows-specific option called SO_EXCLUSIVEADDRUSE but
	 * using that means the socket MUST be closed properly, or a
	 * denial of service can occur.  Luckily for us, winsock
	 * behaves as though any other system would when SO_REUSEADDR
	 * is true, so we don't need to do anything else here.  See
	 * bug 53992.
	 * https://bugzilla.novell.com/show_bug.cgi?id=MONO53992
	 */
	{
		int ret, true_ = 1;

		ret = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &true_, sizeof (true_));
		if (ret == -1) {
			close (sock);
			gint errnum = errno;
			mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: Error setting SO_REUSEADDR", __func__);
			mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
			return INVALID_SOCKET;
		}
	}


	handle = mono_w32handle_new_fd (MONO_W32HANDLE_SOCKET, sock, &socket_handle);
	if (handle == INVALID_HANDLE_VALUE) {
		g_warning ("%s: error creating socket handle", __func__);
		mono_w32socket_set_last_error (WSASYSCALLFAILURE);
		close (sock);
		return INVALID_SOCKET;
	}

	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: returning socket handle %p", __func__, handle);

	return sock;
}

static gint
mono_w32socket_bind (MonoSocket sock, struct sockaddr *addr, socklen_t addrlen)
{
	gpointer handle;
	int ret;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	ret = bind (sock, addr, addrlen);
	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: bind error: %s", __func__, g_strerror(errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	return 0;
}

static gint
mono_w32socket_getpeername (MonoSocket sock, struct sockaddr *name, socklen_t *namelen)
{
	gpointer handle;
	gint ret;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	MONO_ENTER_GC_SAFE;
	ret = getpeername (sock, name, namelen);
	MONO_EXIT_GC_SAFE;

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: getpeername error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	return 0;
}

static gint
mono_w32socket_getsockname (MonoSocket sock, struct sockaddr *name, socklen_t *namelen)
{
	gpointer handle;
	gint ret;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	MONO_ENTER_GC_SAFE;
	ret = getsockname (sock, name, namelen);
	MONO_EXIT_GC_SAFE;

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: getsockname error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	return 0;
}

gint
mono_w32socket_getsockopt (MonoSocket sock, gint level, gint optname, gpointer optval, socklen_t *optlen)
{
	gpointer handle;
	gint ret;
	struct timeval tv;
	gpointer tmp_val;
	MonoW32HandleSocket *socket_handle;

	handle = GUINT_TO_POINTER (sock);
	if (!mono_w32handle_lookup (handle, MONO_W32HANDLE_SOCKET, (gpointer *)&socket_handle)) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	tmp_val = optval;
	if (level == SOL_SOCKET &&
	    (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)) {
		tmp_val = &tv;
		*optlen = sizeof (tv);
	}

	ret = getsockopt (sock, level, optname, tmp_val, optlen);
	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: getsockopt error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	if (level == SOL_SOCKET && (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)) {
		*((int *) optval) = tv.tv_sec * 1000 + (tv.tv_usec / 1000);	// milli from micro
		*optlen = sizeof (int);
	}

	if (optname == SO_ERROR) {
		if (*((int *)optval) != 0) {
			*((int *) optval) = mono_w32socket_convert_error (*((int *)optval));
			socket_handle->saved_error = *((int *)optval);
		} else {
			*((int *)optval) = socket_handle->saved_error;
		}
	}

	return 0;
}

gint
mono_w32socket_setsockopt (MonoSocket sock, gint level, gint optname, const gpointer optval, socklen_t optlen)
{
	gpointer handle;
	gint ret;
	gpointer tmp_val;
#if defined (__linux__)
	/* This has its address taken so it cannot be moved to the if block which uses it */
	gint bufsize = 0;
#endif
	struct timeval tv;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	tmp_val = optval;
	if (level == SOL_SOCKET &&
	    (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)) {
		int ms = *((int *) optval);
		tv.tv_sec = ms / 1000;
		tv.tv_usec = (ms % 1000) * 1000;	// micro from milli
		tmp_val = &tv;
		optlen = sizeof (tv);
	}
#if defined (__linux__)
	else if (level == SOL_SOCKET &&
		   (optname == SO_SNDBUF || optname == SO_RCVBUF)) {
		/* According to socket(7) the Linux kernel doubles the
		 * buffer sizes "to allow space for bookkeeping
		 * overhead."
		 */
		bufsize = *((int *) optval);

		bufsize /= 2;
		tmp_val = &bufsize;
	}
#endif

	ret = setsockopt (sock, level, optname, tmp_val, optlen);
	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: setsockopt error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

#if defined (SO_REUSEPORT)
	/* BSD's and MacOS X multicast sockets also need SO_REUSEPORT when SO_REUSEADDR is requested.  */
	if (level == SOL_SOCKET && optname == SO_REUSEADDR) {
		int type;
		socklen_t type_len = sizeof (type);

		if (!getsockopt (sock, level, SO_TYPE, &type, &type_len)) {
			if (type == SOCK_DGRAM || type == SOCK_STREAM)
				setsockopt (sock, level, SO_REUSEPORT, tmp_val, optlen);
		}
	}
#endif

	return ret;
}

gint
mono_w32socket_listen (MonoSocket sock, gint backlog)
{
	gpointer handle;
	gint ret;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	ret = listen (sock, backlog);
	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: listen error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	return 0;
}

gint
mono_w32socket_shutdown (MonoSocket sock, gint how)
{
	MonoW32HandleSocket *socket_handle;
	gpointer handle;
	gint ret;

	handle = GUINT_TO_POINTER (sock);
	if (!mono_w32handle_lookup (handle, MONO_W32HANDLE_SOCKET, (gpointer *)&socket_handle)) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	if (how == SHUT_RD || how == SHUT_RDWR)
		socket_handle->still_readable = 0;

	ret = shutdown (sock, how);
	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: shutdown error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	return ret;
}

gint
mono_w32socket_disconnect (MonoSocket sock, gboolean reuse)
{
	MonoW32HandleSocket *socket_handle;
	gpointer handle;
	MonoSocket newsock;
	gint ret;

	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: called on socket %d!", __func__, sock);

	/* We could check the socket type here and fail unless its
	 * SOCK_STREAM, SOCK_SEQPACKET or SOCK_RDM (according to msdn)
	 * if we really wanted to */

	handle = GUINT_TO_POINTER (sock);
	if (!mono_w32handle_lookup (handle, MONO_W32HANDLE_SOCKET, (gpointer *)&socket_handle)) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	newsock = socket (socket_handle->domain, socket_handle->type, socket_handle->protocol);
	if (newsock == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: socket error: %s", __func__, g_strerror (errnum));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	/* According to Stevens "Advanced Programming in the UNIX
	 * Environment: UNIX File I/O" dup2() is atomic so there
	 * should not be a race condition between the old fd being
	 * closed and the new socket fd being copied over */
	do {
		ret = dup2 (newsock, sock);
	} while (ret == -1 && errno == EAGAIN);

	if (ret == -1) {
		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: dup2 error: %s", __func__, g_strerror (errnum));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	close (newsock);

	return 0;
}

static gboolean
extension_disconnect (MonoSocket sock, gpointer overlapped, guint32 flags, guint32 reserved)
{
	gboolean ret;
	MONO_ENTER_GC_UNSAFE;
	ret = mono_w32socket_disconnect (sock, flags & TF_REUSE_SOCKET) == 0;
	MONO_EXIT_GC_UNSAFE;
	return ret;
}

static gboolean
extension_transmit_file (MonoSocket sock, gpointer file_handle, guint32 bytes_to_write, guint32 bytes_per_send,
	gpointer overlapped, TRANSMIT_FILE_BUFFERS *buffers, guint32 flags)
{
	gboolean ret;
	MONO_ENTER_GC_UNSAFE;
	ret = mono_w32socket_transmit_file (sock, file_handle, buffers, flags);
	MONO_EXIT_GC_UNSAFE;
	return ret;
}

typedef struct {
	guint32 Data1;
	guint16 Data2;
	guint16 Data3;
	guint8 Data4[8];
} GUID;

static struct {
	GUID guid;
	gpointer func;
} extension_functions[] = {
	{ {0x7fda2e11,0x8630,0x436f,{0xa0,0x31,0xf5,0x36,0xa6,0xee,0xc1,0x57}} /* WSAID_DISCONNECTEX */, extension_disconnect },
	{ {0xb5367df0,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}} /* WSAID_TRANSMITFILE */, extension_transmit_file },
	{ {0} , NULL },
};

gint
mono_w32socket_ioctl (MonoSocket sock, gint32 command, gchar *input, gint inputlen, gchar *output, gint outputlen, glong *written)
{
	gpointer handle;
	gint ret;
	gchar *buffer;

	handle = GUINT_TO_POINTER (sock);
	if (mono_w32handle_get_type (handle) != MONO_W32HANDLE_SOCKET) {
		mono_w32socket_set_last_error (WSAENOTSOCK);
		return SOCKET_ERROR;
	}

	if (command == 0xC8000006 /* SIO_GET_EXTENSION_FUNCTION_POINTER */) {
		gint i;
		GUID *guid;

		if (inputlen < sizeof(GUID)) {
			/* As far as I can tell, windows doesn't
			 * actually set an error here...
			 */
			mono_w32socket_set_last_error (WSAEINVAL);
			return SOCKET_ERROR;
		}

		if (outputlen < sizeof(gpointer)) {
			/* Or here... */
			mono_w32socket_set_last_error (WSAEINVAL);
			return SOCKET_ERROR;
		}

		if (output == NULL) {
			/* Or here */
			mono_w32socket_set_last_error (WSAEINVAL);
			return SOCKET_ERROR;
		}

		guid = (GUID*) input;
		for (i = 0; extension_functions[i].func; i++) {
			if (memcmp (guid, &extension_functions[i].guid, sizeof(GUID)) == 0) {
				memcpy (output, &extension_functions[i].func, sizeof(gpointer));
				*written = sizeof(gpointer);
				return 0;
			}
		}

		mono_w32socket_set_last_error (WSAEINVAL);
		return SOCKET_ERROR;
	}

	if (command == 0x98000004 /* SIO_KEEPALIVE_VALS */) {
		guint32 onoff;

		if (inputlen < 3 * sizeof (guint32)) {
			mono_w32socket_set_last_error (WSAEINVAL);
			return SOCKET_ERROR;
		}

		onoff = *((guint32*) input);

		ret = setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, &onoff, sizeof (guint32));
		if (ret < 0) {
			mono_w32socket_set_last_error (mono_w32socket_convert_error (errno));
			return SOCKET_ERROR;
		}

#if defined(TCP_KEEPIDLE) && defined(TCP_KEEPINTVL)
		if (onoff != 0) {
			/* Values are in ms, but we need s */
			guint32 keepalivetime, keepaliveinterval, rem;

			keepalivetime = *(((guint32*) input) + 1);
			keepaliveinterval = *(((guint32*) input) + 2);

			/* keepalivetime and keepaliveinterval are > 0 (checked in managed code) */
			rem = keepalivetime % 1000;
			keepalivetime /= 1000;
			if (keepalivetime == 0 || rem >= 500)
				keepalivetime++;
			ret = setsockopt (sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepalivetime, sizeof (guint32));
			if (ret == 0) {
				rem = keepaliveinterval % 1000;
				keepaliveinterval /= 1000;
				if (keepaliveinterval == 0 || rem >= 500)
					keepaliveinterval++;
				ret = setsockopt (sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepaliveinterval, sizeof (guint32));
			}
			if (ret != 0) {
				mono_w32socket_set_last_error (mono_w32socket_convert_error (errno));
				return SOCKET_ERROR;
			}

			return 0;
		}
#endif

		return 0;
	}

	buffer = inputlen > 0 ? (gchar*) g_memdup (input, inputlen) : NULL;

	ret = ioctl (sock, command, buffer);
	if (ret == -1) {
		g_free (buffer);

		gint errnum = errno;
		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_IO_LAYER, "%s: WSAIoctl error: %s", __func__, g_strerror (errno));
		mono_w32socket_set_last_error (mono_w32socket_convert_error (errnum));
		return SOCKET_ERROR;
	}

	if (!buffer) {
		*written = 0;
		return 0;
	}

	/* We just copy the buffer to the output. Some ioctls
	 * don't even output any data, but, well...
	 *
	 * NB windows returns WSAEFAULT if outputlen is too small */
	inputlen = (inputlen > outputlen) ? outputlen : inputlen;

	if (inputlen > 0 && output != NULL)
		memcpy (output, buffer, inputlen);

	g_free (buffer);
	*written = inputlen;

	return 0;
}

gboolean
mono_w32socket_close (MonoSocket sock)
{
	return mono_w32handle_close (GINT_TO_POINTER (sock));
}

void
mono_w32socket_set_last_error (gint32 error)
{
	mono_w32error_set_last (error);
}

gint32
mono_w32socket_get_last_error (void)
{
	return mono_w32error_get_last ();
}

gint32
mono_w32socket_convert_error (gint error)
{
	switch (error) {
	case 0: return ERROR_SUCCESS;
	case EACCES: return WSAEACCES;
#ifdef EADDRINUSE
	case EADDRINUSE: return WSAEADDRINUSE;
#endif
#ifdef EAFNOSUPPORT
	case EAFNOSUPPORT: return WSAEAFNOSUPPORT;
#endif
#if EAGAIN != EWOULDBLOCK
	case EAGAIN: return WSAEWOULDBLOCK;
#endif
#ifdef EALREADY
	case EALREADY: return WSAEALREADY;
#endif
	case EBADF: return WSAENOTSOCK;
#ifdef ECONNABORTED
	case ECONNABORTED: return WSAENETDOWN;
#endif
#ifdef ECONNREFUSED
	case ECONNREFUSED: return WSAECONNREFUSED;
#endif
#ifdef ECONNRESET
	case ECONNRESET: return WSAECONNRESET;
#endif
	case EFAULT: return WSAEFAULT;
#ifdef EHOSTUNREACH
	case EHOSTUNREACH: return WSAEHOSTUNREACH;
#endif
#ifdef EINPROGRESS
	case EINPROGRESS: return WSAEINPROGRESS;
#endif
	case EINTR: return WSAEINTR;
	case EINVAL: return WSAEINVAL;
	/*FIXME: case EIO: return WSAE????; */
#ifdef EISCONN
	case EISCONN: return WSAEISCONN;
#endif
	/* FIXME: case ELOOP: return WSA????; */
	case EMFILE: return WSAEMFILE;
#ifdef EMSGSIZE
	case EMSGSIZE: return WSAEMSGSIZE;
#endif
	/* FIXME: case ENAMETOOLONG: return WSAEACCES; */
#ifdef ENETUNREACH
	case ENETUNREACH: return WSAENETUNREACH;
#endif
#ifdef ENOBUFS
	case ENOBUFS: return WSAENOBUFS; /* not documented */
#endif
	/* case ENOENT: return WSAE????; */
	case ENOMEM: return WSAENOBUFS;
#ifdef ENOPROTOOPT
	case ENOPROTOOPT: return WSAENOPROTOOPT;
#endif
#ifdef ENOSR
	case ENOSR: return WSAENETDOWN;
#endif
#ifdef ENOTCONN
	case ENOTCONN: return WSAENOTCONN;
#endif
	/*FIXME: case ENOTDIR: return WSAE????; */
#ifdef ENOTSOCK
	case ENOTSOCK: return WSAENOTSOCK;
#endif
	case ENOTTY: return WSAENOTSOCK;
#ifdef EOPNOTSUPP
	case EOPNOTSUPP: return WSAEOPNOTSUPP;
#endif
	case EPERM: return WSAEACCES;
	case EPIPE: return WSAESHUTDOWN;
#ifdef EPROTONOSUPPORT
	case EPROTONOSUPPORT: return WSAEPROTONOSUPPORT;
#endif
#if ERESTARTSYS
	case ERESTARTSYS: return WSAENETDOWN;
#endif
	/*FIXME: case EROFS: return WSAE????; */
#ifdef ESOCKTNOSUPPORT
	case ESOCKTNOSUPPORT: return WSAESOCKTNOSUPPORT;
#endif
#ifdef ETIMEDOUT
	case ETIMEDOUT: return WSAETIMEDOUT;
#endif
#ifdef EWOULDBLOCK
	case EWOULDBLOCK: return WSAEWOULDBLOCK;
#endif
#ifdef EADDRNOTAVAIL
	case EADDRNOTAVAIL: return WSAEADDRNOTAVAIL;
#endif
	/* This might happen with unix sockets */
	case ENOENT: return WSAECONNREFUSED;
#ifdef EDESTADDRREQ
	case EDESTADDRREQ: return WSAEDESTADDRREQ;
#endif
#ifdef EHOSTDOWN
	case EHOSTDOWN: return WSAEHOSTDOWN;
#endif
#ifdef ENETDOWN
	case ENETDOWN: return WSAENETDOWN;
#endif
	case ENODEV: return WSAENETDOWN;
	default:
		g_error ("%s: no translation into winsock error for (%d) \"%s\"", __func__, error, g_strerror (error));
	}
}

gboolean
ves_icall_System_Net_Sockets_Socket_SupportPortReuse (MonoProtocolType proto)
{
#if defined (SO_REUSEPORT)
	return TRUE;
#else
#ifdef __linux__
	/* Linux always supports double binding for UDP, even on older kernels. */
	if (proto == ProtocolType_Udp)
		return TRUE;
#endif
	return FALSE;
#endif
}

static gint32
convert_family (MonoAddressFamily mono_family)
{
	switch (mono_family) {
	case AddressFamily_Unknown:
	case AddressFamily_ImpLink:
	case AddressFamily_Pup:
	case AddressFamily_Chaos:
	case AddressFamily_Iso:
	case AddressFamily_Ecma:
	case AddressFamily_DataKit:
	case AddressFamily_Ccitt:
	case AddressFamily_DataLink:
	case AddressFamily_Lat:
	case AddressFamily_HyperChannel:
	case AddressFamily_NetBios:
	case AddressFamily_VoiceView:
	case AddressFamily_FireFox:
	case AddressFamily_Banyan:
	case AddressFamily_Atm:
	case AddressFamily_Cluster:
	case AddressFamily_Ieee12844:
	case AddressFamily_NetworkDesigners:
		g_warning ("System.Net.Sockets.AddressFamily has unsupported value 0x%x", mono_family);
		return -1;
	case AddressFamily_Unspecified:
		return AF_UNSPEC;
	case AddressFamily_Unix:
		return AF_UNIX;
	case AddressFamily_InterNetwork:
		return AF_INET;
	case AddressFamily_AppleTalk:
		return AF_APPLETALK;
	case AddressFamily_InterNetworkV6:
		return AF_INET6;
	case AddressFamily_DecNet:
#ifdef AF_DECnet
		return AF_DECnet;
#else
		return -1;
#endif
	case AddressFamily_Ipx:
#ifdef AF_IPX
		return AF_IPX;
#else
		return -1;
#endif
	case AddressFamily_Sna:
#ifdef AF_SNA
		return AF_SNA;
#else
		return -1;
#endif
	case AddressFamily_Irda:
#ifdef AF_IRDA
		return AF_IRDA;
#else
		return -1;
#endif
	default:
		g_warning ("System.Net.Sockets.AddressFamily has unknown value 0x%x", mono_family);
		return -1;
	}
}

static MonoAddressFamily
convert_to_mono_family (guint16 af_family)
{
	switch (af_family) {
	case AF_UNSPEC:
		return AddressFamily_Unspecified;
	case AF_UNIX:
		return AddressFamily_Unix;
	case AF_INET:
		return AddressFamily_InterNetwork;
#ifdef AF_IPX
	case AF_IPX:
		return AddressFamily_Ipx;
#endif
#ifdef AF_SNA
	case AF_SNA:
		return AddressFamily_Sna;
#endif
#ifdef AF_DECnet
	case AF_DECnet:
		return AddressFamily_DecNet;
#endif
	case AF_APPLETALK:
		return AddressFamily_AppleTalk;
	case AF_INET6:
		return AddressFamily_InterNetworkV6;
#ifdef AF_IRDA
	case AF_IRDA:
		return AddressFamily_Irda;
#endif
	default:
		g_warning ("unknown address family 0x%x", af_family);
		return AddressFamily_Unknown;
	}
}

static gint32
convert_type (MonoSocketType mono_type)
{
	switch (mono_type) {
	case SocketType_Stream:
		return SOCK_STREAM;
	case SocketType_Dgram:
		return SOCK_DGRAM;
	case SocketType_Raw:
		return SOCK_RAW;
	case SocketType_Rdm:
#ifdef SOCK_RDM
		return SOCK_RDM;
#else
		return -1;
#endif
	case SocketType_Seqpacket:
		return SOCK_SEQPACKET;
	case SocketType_Unknown:
		g_warning ("System.Net.Sockets.SocketType has unsupported value 0x%x", mono_type);
		return -1;
	default:
		g_warning ("System.Net.Sockets.SocketType has unknown value 0x%x", mono_type);
		return -1;
	}
}

static gint32
convert_proto (MonoProtocolType mono_proto)
{
	switch (mono_proto) {
	case ProtocolType_IP:
	case ProtocolType_IPv6:
	case ProtocolType_Icmp:
	case ProtocolType_Igmp:
	case ProtocolType_Ggp:
	case ProtocolType_Tcp:
	case ProtocolType_Pup:
	case ProtocolType_Udp:
	case ProtocolType_Idp:
		/* These protocols are known (on my system at least) */
		return mono_proto;
	case ProtocolType_ND:
	case ProtocolType_Raw:
	case ProtocolType_Ipx:
	case ProtocolType_Spx:
	case ProtocolType_SpxII:
	case ProtocolType_Unknown:
		/* These protocols arent */
		g_warning ("System.Net.Sockets.ProtocolType has unsupported value 0x%x", mono_proto);
		return -1;
	default:
		return -1;
	}
}

/* Convert MonoSocketFlags */
static gint32
convert_socketflags (gint32 sflags)
{
	gint32 flags = 0;

	if (!sflags)
		/* SocketFlags.None */
		return 0;

	if (sflags & ~(SocketFlags_OutOfBand | SocketFlags_MaxIOVectorLength | SocketFlags_Peek |
			SocketFlags_DontRoute | SocketFlags_Partial))
		/* Contains invalid flag values */
		return -1;

	if (sflags & SocketFlags_OutOfBand)
		flags |= MSG_OOB;
	if (sflags & SocketFlags_Peek)
		flags |= MSG_PEEK;
	if (sflags & SocketFlags_DontRoute)
		flags |= MSG_DONTROUTE;

	/* Ignore Partial - see bug 349688.  Don't return -1, because
	 * according to the comment in that bug ms runtime doesn't for
	 * UDP sockets (this means we will silently ignore it for TCP
	 * too)
	 */
#ifdef MSG_MORE
	if (sflags & SocketFlags_Partial)
		flags |= MSG_MORE;
#endif
#if 0
	/* Don't do anything for MaxIOVectorLength */
	if (sflags & SocketFlags_MaxIOVectorLength)
		return -1;
#endif
	return flags;
}

/*
 * Returns:
 *    0 on success (mapped mono_level and mono_name to system_level and system_name
 *   -1 on error
 *   -2 on non-fatal error (ie, must ignore)
 */
static gint32
convert_sockopt_level_and_name (MonoSocketOptionLevel mono_level, MonoSocketOptionName mono_name, int *system_level, int *system_name)
{
	switch (mono_level) {
	case SocketOptionLevel_Socket:
		*system_level = SOL_SOCKET;

		switch (mono_name) {
		case SocketOptionName_DontLinger:
			/* This is SO_LINGER, because the setsockopt
			 * internal call maps DontLinger to SO_LINGER
			 * with l_onoff=0
			 */
			*system_name = SO_LINGER;
			break;
		case SocketOptionName_Debug:
			*system_name = SO_DEBUG;
			break;
#ifdef SO_ACCEPTCONN
		case SocketOptionName_AcceptConnection:
			*system_name = SO_ACCEPTCONN;
			break;
#endif
		case SocketOptionName_ReuseAddress:
			*system_name = SO_REUSEADDR;
			break;
		case SocketOptionName_KeepAlive:
			*system_name = SO_KEEPALIVE;
			break;
		case SocketOptionName_DontRoute:
			*system_name = SO_DONTROUTE;
			break;
		case SocketOptionName_Broadcast:
			*system_name = SO_BROADCAST;
			break;
		case SocketOptionName_Linger:
			*system_name = SO_LINGER;
			break;
		case SocketOptionName_OutOfBandInline:
			*system_name = SO_OOBINLINE;
			break;
		case SocketOptionName_SendBuffer:
			*system_name = SO_SNDBUF;
			break;
		case SocketOptionName_ReceiveBuffer:
			*system_name = SO_RCVBUF;
			break;
		case SocketOptionName_SendLowWater:
			*system_name = SO_SNDLOWAT;
			break;
		case SocketOptionName_ReceiveLowWater:
			*system_name = SO_RCVLOWAT;
			break;
		case SocketOptionName_SendTimeout:
			*system_name = SO_SNDTIMEO;
			break;
		case SocketOptionName_ReceiveTimeout:
			*system_name = SO_RCVTIMEO;
			break;
		case SocketOptionName_Error:
			*system_name = SO_ERROR;
			break;
		case SocketOptionName_Type:
			*system_name = SO_TYPE;
			break;
#ifdef SO_PEERCRED
		case SocketOptionName_PeerCred:
			*system_name = SO_PEERCRED;
			break;
#endif
		case SocketOptionName_ExclusiveAddressUse:
#ifdef SO_EXCLUSIVEADDRUSE
			*system_name = SO_EXCLUSIVEADDRUSE;
			break;
#endif
		case SocketOptionName_UseLoopback:
#ifdef SO_USELOOPBACK
			*system_name = SO_USELOOPBACK;
			break;
#endif
		case SocketOptionName_MaxConnections:
#ifdef SO_MAXCONN
			*system_name = SO_MAXCONN;
			break;
#elif defined(SOMAXCONN)
			*system_name = SOMAXCONN;
			break;
#endif
		default:
			g_warning ("System.Net.Sockets.SocketOptionName 0x%x is not supported at Socket level", mono_name);
			return -1;
		}
		break;

	case SocketOptionLevel_IP:
		*system_level = mono_networking_get_ip_protocol ();

		switch (mono_name) {
		case SocketOptionName_IPOptions:
			*system_name = IP_OPTIONS;
			break;
#ifdef IP_HDRINCL
		case SocketOptionName_HeaderIncluded:
			*system_name = IP_HDRINCL;
			break;
#endif
#ifdef IP_TOS
		case SocketOptionName_TypeOfService:
			*system_name = IP_TOS;
			break;
#endif
#ifdef IP_TTL
		case SocketOptionName_IpTimeToLive:
			*system_name = IP_TTL;
			break;
#endif
		case SocketOptionName_MulticastInterface:
			*system_name = IP_MULTICAST_IF;
			break;
		case SocketOptionName_MulticastTimeToLive:
			*system_name = IP_MULTICAST_TTL;
			break;
		case SocketOptionName_MulticastLoopback:
			*system_name = IP_MULTICAST_LOOP;
			break;
		case SocketOptionName_AddMembership:
			*system_name = IP_ADD_MEMBERSHIP;
			break;
		case SocketOptionName_DropMembership:
			*system_name = IP_DROP_MEMBERSHIP;
			break;
#ifdef HAVE_IP_PKTINFO
		case SocketOptionName_PacketInformation:
			*system_name = IP_PKTINFO;
			break;
#endif /* HAVE_IP_PKTINFO */

		case SocketOptionName_DontFragment:
#ifdef HAVE_IP_DONTFRAGMENT
			*system_name = IP_DONTFRAGMENT;
			break;
#elif defined HAVE_IP_MTU_DISCOVER
			/* Not quite the same */
			*system_name = IP_MTU_DISCOVER;
			break;
#else
			/* If the flag is not available on this system, we can ignore this error */
			return -2;
#endif /* HAVE_IP_DONTFRAGMENT */
		case SocketOptionName_AddSourceMembership:
		case SocketOptionName_DropSourceMembership:
		case SocketOptionName_BlockSource:
		case SocketOptionName_UnblockSource:
			/* Can't figure out how to map these, so fall
			 * through
			 */
		default:
			g_warning ("System.Net.Sockets.SocketOptionName 0x%x is not supported at IP level", mono_name);
			return -1;
		}
		break;

	case SocketOptionLevel_IPv6:
		*system_level = mono_networking_get_ipv6_protocol ();

		switch (mono_name) {
		case SocketOptionName_IpTimeToLive:
		case SocketOptionName_HopLimit:
			*system_name = IPV6_UNICAST_HOPS;
			break;
		case SocketOptionName_MulticastInterface:
			*system_name = IPV6_MULTICAST_IF;
			break;
		case SocketOptionName_MulticastTimeToLive:
			*system_name = IPV6_MULTICAST_HOPS;
			break;
		case SocketOptionName_MulticastLoopback:
			*system_name = IPV6_MULTICAST_LOOP;
			break;
		case SocketOptionName_AddMembership:
			*system_name = IPV6_JOIN_GROUP;
			break;
		case SocketOptionName_DropMembership:
			*system_name = IPV6_LEAVE_GROUP;
			break;
		case SocketOptionName_IPv6Only:
#ifdef IPV6_V6ONLY
			*system_name = IPV6_V6ONLY;
#else
			return -1;
#endif
			break;
		case SocketOptionName_PacketInformation:
#ifdef HAVE_IPV6_PKTINFO
			*system_name = IPV6_PKTINFO;
#endif
			break;
		case SocketOptionName_HeaderIncluded:
		case SocketOptionName_IPOptions:
		case SocketOptionName_TypeOfService:
		case SocketOptionName_DontFragment:
		case SocketOptionName_AddSourceMembership:
		case SocketOptionName_DropSourceMembership:
		case SocketOptionName_BlockSource:
		case SocketOptionName_UnblockSource:
			/* Can't figure out how to map these, so fall
			 * through
			 */
		default:
			g_warning ("System.Net.Sockets.SocketOptionName 0x%x is not supported at IPv6 level", mono_name);
			return -1;
		}
		break;	/* SocketOptionLevel_IPv6 */

	case SocketOptionLevel_Tcp:
		*system_level = mono_networking_get_tcp_protocol ();

		switch (mono_name) {
		case SocketOptionName_NoDelay:
			*system_name = TCP_NODELAY;
			break;
#if 0
			/* The documentation is talking complete
			 * bollocks here: rfc-1222 is titled
			 * 'Advancing the NSFNET Routing Architecture'
			 * and doesn't mention either of the words
			 * "expedite" or "urgent".
			 */
		case SocketOptionName_BsdUrgent:
		case SocketOptionName_Expedited:
#endif
		default:
			g_warning ("System.Net.Sockets.SocketOptionName 0x%x is not supported at TCP level", mono_name);
			return -1;
		}
		break;

	case SocketOptionLevel_Udp:
		g_warning ("System.Net.Sockets.SocketOptionLevel has unsupported value 0x%x", mono_level);

		switch(mono_name) {
		case SocketOptionName_NoChecksum:
		case SocketOptionName_ChecksumCoverage:
		default:
			g_warning ("System.Net.Sockets.SocketOptionName 0x%x is not supported at UDP level", mono_name);
			return -1;
		}
		return -1;
		break;

	default:
		g_warning ("System.Net.Sockets.SocketOptionLevel has unknown value 0x%x", mono_level);
		return -1;
	}

	return 0;
}

static MonoImage*
get_socket_assembly (void)
{
	MonoDomain *domain = mono_domain_get ();

	if (domain->socket_assembly == NULL) {
		MonoImage *socket_assembly;

		socket_assembly = mono_image_loaded ("System");
		if (!socket_assembly) {
			MonoAssembly *sa = mono_assembly_open ("System.dll", NULL);

			if (!sa) {
				g_assert_not_reached ();
			} else {
				socket_assembly = mono_assembly_get_image (sa);
			}
		}
		mono_atomic_store_release (&domain->socket_assembly, socket_assembly);
	}

	return domain->socket_assembly;
}

// Check whether it's ::ffff::0:0.
static gboolean
is_ipv4_mapped_any (const struct in6_addr *addr)
{
	int i;

	for (i = 0; i < 10; i++) {
		if (addr->s6_addr [i])
			return FALSE;
	}
	if ((addr->s6_addr [10] != 0xff) || (addr->s6_addr [11] != 0xff))
		return FALSE;
	for (i = 12; i < 16; i++) {
		if (addr->s6_addr [i])
			return FALSE;
	}
	return TRUE;
}

static MonoObject*
create_object_from_sockaddr (struct sockaddr *saddr, int sa_size, gint32 *werror, MonoError *error)
{
	MonoDomain *domain = mono_domain_get ();
	MonoObject *sockaddr_obj;
	MonoArray *data;
	MonoAddressFamily family;

	mono_error_init (error);

	/* Build a System.Net.SocketAddress object instance */
	if (!domain->sockaddr_class)
		domain->sockaddr_class = mono_class_load_from_name (get_socket_assembly (), "System.Net", "SocketAddress");
	sockaddr_obj = mono_object_new_checked (domain, domain->sockaddr_class, error);
	return_val_if_nok (error, NULL);

	/* Locate the SocketAddress data buffer in the object */
	if (!domain->sockaddr_data_field) {
		domain->sockaddr_data_field = mono_class_get_field_from_name (domain->sockaddr_class, "m_Buffer");
		g_assert (domain->sockaddr_data_field);
	}

	/* Locate the SocketAddress data buffer length in the object */
	if (!domain->sockaddr_data_length_field) {
		domain->sockaddr_data_length_field = mono_class_get_field_from_name (domain->sockaddr_class, "m_Size");
		g_assert (domain->sockaddr_data_length_field);
	}

	/* May be the +2 here is too conservative, as sa_len returns
	 * the length of the entire sockaddr_in/in6, including
	 * sizeof (unsigned short) of the family */
	/* We can't really avoid the +2 as all code below depends on this size - INCLUDING unix domain sockets.*/
	data = mono_array_new_cached (domain, mono_get_byte_class (), sa_size + 2, error);
	return_val_if_nok (error, NULL);

	/* The data buffer is laid out as follows:
	 * bytes 0 and 1 are the address family
	 * bytes 2 and 3 are the port info
	 * the rest is the address info
	 */

	family = convert_to_mono_family (saddr->sa_family);
	if (family == AddressFamily_Unknown) {
		*werror = WSAEAFNOSUPPORT;
		return NULL;
	}

	mono_array_set (data, guint8, 0, family & 0x0FF);
	mono_array_set (data, guint8, 1, (family >> 8) & 0x0FF);

	if (saddr->sa_family == AF_INET) {
		struct sockaddr_in *sa_in = (struct sockaddr_in *)saddr;
		guint16 port = ntohs (sa_in->sin_port);
		guint32 address = ntohl (sa_in->sin_addr.s_addr);
		int buffer_size = 8;

		if (sa_size < buffer_size) {
			mono_error_set_exception_instance (error, mono_exception_from_name (mono_get_corlib (), "System", "SystemException"));
			return NULL;
		}

		mono_array_set (data, guint8, 2, (port>>8) & 0xff);
		mono_array_set (data, guint8, 3, (port) & 0xff);
		mono_array_set (data, guint8, 4, (address>>24) & 0xff);
		mono_array_set (data, guint8, 5, (address>>16) & 0xff);
		mono_array_set (data, guint8, 6, (address>>8) & 0xff);
		mono_array_set (data, guint8, 7, (address) & 0xff);

		mono_field_set_value (sockaddr_obj, domain->sockaddr_data_field, data);
		mono_field_set_value (sockaddr_obj, domain->sockaddr_data_length_field, &buffer_size);

		return sockaddr_obj;
	} else if (saddr->sa_family == AF_INET6) {
		struct sockaddr_in6 *sa_in = (struct sockaddr_in6 *)saddr;
		int i;
		int buffer_size = 28;

		guint16 port = ntohs (sa_in->sin6_port);

		if (sa_size < buffer_size) {
			mono_error_set_exception_instance (error, mono_exception_from_name (mono_get_corlib (), "System", "SystemException"));
			return NULL;
		}

		mono_array_set (data, guint8, 2, (port>>8) & 0xff);
		mono_array_set (data, guint8, 3, (port) & 0xff);

		if (is_ipv4_mapped_any (&sa_in->sin6_addr)) {
			// Map ::ffff:0:0 to :: (bug #5502)
			for (i = 0; i < 16; i++)
				mono_array_set (data, guint8, 8 + i, 0);
		} else {
			for (i = 0; i < 16; i++) {
				mono_array_set (data, guint8, 8 + i,
								sa_in->sin6_addr.s6_addr [i]);
			}
		}

		mono_array_set (data, guint8, 24, sa_in->sin6_scope_id & 0xff);
		mono_array_set (data, guint8, 25,
						(sa_in->sin6_scope_id >> 8) & 0xff);
		mono_array_set (data, guint8, 26,
						(sa_in->sin6_scope_id >> 16) & 0xff);
		mono_array_set (data, guint8, 27,
						(sa_in->sin6_scope_id >> 24) & 0xff);

		mono_field_set_value (sockaddr_obj, domain->sockaddr_data_field, data);
		mono_field_set_value (sockaddr_obj, domain->sockaddr_data_length_field, &buffer_size);

		return sockaddr_obj;
	}
#ifdef HAVE_SYS_UN_H
	else if (saddr->sa_family == AF_UNIX) {
		int i;
		int buffer_size = sa_size + 2;

		for (i = 0; i < sa_size; i++)
			mono_array_set (data, guint8, i + 2, saddr->sa_data [i]);

		mono_field_set_value (sockaddr_obj, domain->sockaddr_data_field, data);
		mono_field_set_value (sockaddr_obj, domain->sockaddr_data_length_field, &buffer_size);

		return sockaddr_obj;
	}
#endif
	else {
		*werror = WSAEAFNOSUPPORT;
		return NULL;
	}
}

static int
get_sockaddr_size (int family)
{
	int size;

	size = 0;
	if (family == AF_INET) {
		size = sizeof (struct sockaddr_in);
	} else if (family == AF_INET6) {
		size = sizeof (struct sockaddr_in6);
	}
#ifdef HAVE_SYS_UN_H
	else if (family == AF_UNIX) {
		size = sizeof (struct sockaddr_un);
	}
#endif
	return size;
}

static struct sockaddr*
create_sockaddr_from_object (MonoObject *saddr_obj, socklen_t *sa_size, gint32 *werror, MonoError *error)
{
	MonoDomain *domain = mono_domain_get ();
	MonoArray *data;
	gint32 family;
	int len;

	mono_error_init (error);

	if (!domain->sockaddr_class)
		domain->sockaddr_class = mono_class_load_from_name (get_socket_assembly (), "System.Net", "SocketAddress");

	/* Locate the SocketAddress data buffer in the object */
	if (!domain->sockaddr_data_field) {
		domain->sockaddr_data_field = mono_class_get_field_from_name (domain->sockaddr_class, "m_Buffer");
		g_assert (domain->sockaddr_data_field);
	}

	/* Locate the SocketAddress data buffer length in the object */
	if (!domain->sockaddr_data_length_field) {
		domain->sockaddr_data_length_field = mono_class_get_field_from_name (domain->sockaddr_class, "m_Size");
		g_assert (domain->sockaddr_data_length_field);
	}

	data = *(MonoArray **)(((char *)saddr_obj) + domain->sockaddr_data_field->offset);

	/* The data buffer is laid out as follows:
	 * byte 0 is the address family low byte
	 * byte 1 is the address family high byte
	 * INET:
	 * 	bytes 2 and 3 are the port info
	 * 	the rest is the address info
	 * UNIX:
	 * 	the rest is the file name
	 */
	len = *(int *)(((char *)saddr_obj) + domain->sockaddr_data_length_field->offset);
	g_assert (len >= 2);

	family = convert_family ((MonoAddressFamily)(mono_array_get (data, guint8, 0) + (mono_array_get (data, guint8, 1) << 8)));
	if (family == AF_INET) {
		struct sockaddr_in *sa;
		guint16 port;
		guint32 address;

		if (len < 8) {
			mono_error_set_exception_instance (error, mono_exception_from_name (mono_get_corlib (), "System", "SystemException"));
			return NULL;
		}

		sa = g_new0 (struct sockaddr_in, 1);
		port = (mono_array_get (data, guint8, 2) << 8) +
			mono_array_get (data, guint8, 3);
		address = (mono_array_get (data, guint8, 4) << 24) +
			(mono_array_get (data, guint8, 5) << 16 ) +
			(mono_array_get (data, guint8, 6) << 8) +
			mono_array_get (data, guint8, 7);

		sa->sin_family = family;
		sa->sin_addr.s_addr = htonl (address);
		sa->sin_port = htons (port);

		*sa_size = sizeof (struct sockaddr_in);
		return (struct sockaddr *)sa;
	} else if (family == AF_INET6) {
		struct sockaddr_in6 *sa;
		int i;
		guint16 port;
		guint32 scopeid;

		if (len < 28) {
			mono_error_set_exception_instance (error, mono_exception_from_name (mono_get_corlib (), "System", "SystemException"));
			return NULL;
		}

		sa = g_new0 (struct sockaddr_in6, 1);
		port = mono_array_get (data, guint8, 3) +
			(mono_array_get (data, guint8, 2) << 8);
		scopeid = mono_array_get (data, guint8, 24) +
			(mono_array_get (data, guint8, 25) << 8) +
			(mono_array_get (data, guint8, 26) << 16) +
			(mono_array_get (data, guint8, 27) << 24);

		sa->sin6_family = family;
		sa->sin6_port = htons (port);
		sa->sin6_scope_id = scopeid;

		for (i = 0; i < 16; i++)
			sa->sin6_addr.s6_addr [i] = mono_array_get (data, guint8, 8 + i);

		*sa_size = sizeof (struct sockaddr_in6);
		return (struct sockaddr *)sa;
	}
#ifdef HAVE_SYS_UN_H
	else if (family == AF_UNIX) {
		struct sockaddr_un *sock_un;
		int i;

		/* Need a byte for the '\0' terminator/prefix, and the first
		 * two bytes hold the SocketAddress family
		 */
		if (len - 2 >= sizeof (sock_un->sun_path)) {
			mono_error_set_exception_instance (error, mono_get_exception_index_out_of_range ());
			return NULL;
		}

		sock_un = g_new0 (struct sockaddr_un, 1);

		sock_un->sun_family = family;
		for (i = 0; i < len - 2; i++)
			sock_un->sun_path [i] = mono_array_get (data, guint8, i + 2);

		*sa_size = len;
		return (struct sockaddr *)sock_un;
	}
#endif
	else {
		*werror = WSAEAFNOSUPPORT;
		return 0;
	}
}

gpointer
ves_icall_Mono_PAL_Sockets_Unix_Socket (gint32 family, gint32 type, gint32 proto, gint32 *werror)
{
	MonoSocket sock;
	gint32 sock_family;
	gint32 sock_proto;
	gint32 sock_type;

	*werror = 0;

	sock_family = convert_family ((MonoAddressFamily)family);
	if (sock_family == -1) {
		*werror = WSAEAFNOSUPPORT;
		return NULL;
	}

	sock_proto = convert_proto ((MonoProtocolType)proto);
	if (sock_proto == -1) {
		*werror = WSAEPROTONOSUPPORT;
		return NULL;
	}

	sock_type = convert_type ((MonoSocketType)type);
	if (sock_type == -1) {
		*werror = WSAESOCKTNOSUPPORT;
		return NULL;
	}

	sock = mono_w32socket_socket (sock_family, sock_type, sock_proto);
	if (sock == INVALID_SOCKET) {
		*werror = mono_w32socket_get_last_error ();
		return NULL;
	}

	return GUINT_TO_POINTER (sock);
}

gsize
ves_icall_Mono_PAL_Sockets_Unix_Accept (gsize sock, gint32 *werror)
{
	MonoSocket ret;

	*werror = 0;

	ret = mono_w32socket_accept (sock);
	if (ret == INVALID_SOCKET) {
		*werror = mono_w32socket_get_last_error ();
		return 0;
	}

	return ret;
}

MonoObject*
ves_icall_Mono_PAL_Sockets_Unix_GetLocalEndPoint (gsize sock, gint32 af, gint32 *werror)
{
	gchar *sa;
	socklen_t salen;
	int ret;
	MonoObject *result;
	MonoError error;

	*werror = 0;

	salen = get_sockaddr_size (convert_family ((MonoAddressFamily)af));
	if (salen == 0) {
		*werror = WSAEAFNOSUPPORT;
		return NULL;
	}
	sa = (salen <= 128) ? (gchar *)alloca (salen) : (gchar *)g_malloc0 (salen);

	ret = mono_w32socket_getsockname ((MonoSocket) sock, (struct sockaddr *)sa, &salen);
	if (ret == SOCKET_ERROR) {
		*werror = mono_w32socket_get_last_error ();
		if (salen > 128)
			g_free (sa);
		return NULL;
	}

	LOGDEBUG (g_message("%s: bound to %s port %d", __func__, inet_ntoa (((struct sockaddr_in *)&sa)->sin_addr), ntohs (((struct sockaddr_in *)&sa)->sin_port)));

	result = create_object_from_sockaddr ((struct sockaddr *)sa, salen, werror, &error);
	if (salen > 128)
		g_free (sa);
	if (!mono_error_ok (&error))
		mono_error_set_pending_exception (&error);
	return result;
}

MonoObject*
ves_icall_Mono_PAL_Sockets_Unix_GetRemoteEndPoint (gsize sock, gint32 af, gint32 *werror)
{
	gchar *sa;
	socklen_t salen;
	int ret;
	MonoObject *result;
	MonoError error;

	*werror = 0;

	salen = get_sockaddr_size (convert_family ((MonoAddressFamily)af));
	if (salen == 0) {
		*werror = WSAEAFNOSUPPORT;
		return NULL;
	}
	sa = (salen <= 128) ? (gchar *)alloca (salen) : (gchar *)g_malloc0 (salen);
	/* Note: linux returns just 2 for AF_UNIX. Always. */

	ret = mono_w32socket_getpeername (sock, (struct sockaddr *)sa, &salen);
	if (ret == SOCKET_ERROR) {
		*werror = mono_w32socket_get_last_error ();
		if (salen > 128)
			g_free (sa);
		return NULL;
	}

	LOGDEBUG (g_message("%s: connected to %s port %d", __func__, inet_ntoa (((struct sockaddr_in *)&sa)->sin_addr), ntohs (((struct sockaddr_in *)&sa)->sin_port)));

	result = create_object_from_sockaddr ((struct sockaddr *)sa, salen, werror, &error);
	if (salen > 128)
		g_free (sa);
	if (!mono_error_ok (&error))
		mono_error_set_pending_exception (&error);
	return result;
}

void
ves_icall_Mono_PAL_Sockets_Unix_SetBlocking (gsize sock, MonoBoolean blocking, gint32 *werror)
{
	gint res;

	*werror = 0;

	if (mono_w32handle_get_type (GINT_TO_POINTER (sock)) != MONO_W32HANDLE_SOCKET) {
		*werror = WSAENOTSOCK;
		return;
	}

#ifdef O_NONBLOCK
	/* This works better than ioctl(...FIONBIO...)
	 * on Linux (it causes connect to return
	 * EINPROGRESS, but the ioctl doesn't seem to) */
	res = fcntl (sock, F_GETFL, 0);
	if (res == -1) {
		*werror = mono_w32socket_convert_error (errno);
		return;
	}

	res = fcntl (sock, F_SETFL, blocking ? (res & (~O_NONBLOCK)) : (res | (O_NONBLOCK)));
	if (res == -1) {
		*werror = mono_w32socket_convert_error (errno);
		return;
	}
#endif /* O_NONBLOCK */
}

gint32
ves_icall_Mono_PAL_Sockets_Unix_GetAvailable (gsize sock, gint32 *werror)
{
	gulong amount;
	gint res;

	*werror = 0;

	if (mono_w32handle_get_type (GINT_TO_POINTER (sock)) != MONO_W32HANDLE_SOCKET) {
		*werror = WSAENOTSOCK;
		return 0;
	}

#if defined (PLATFORM_MACOSX)
	/* ioctl (sock, FIONREAD, XXX) returns the size of
	 * the UDP header as well on Darwin.
	 *
	 * Use getsockopt SO_NREAD instead to get the
	 * right values for TCP and UDP.
	 *
	 * ai_canonname can be null in some cases on darwin,
	 * where the runtime assumes it will be the value of
	 * the ip buffer. */

	socklen_t optlen = sizeof (int);
	res = getsockopt (sock, SOL_SOCKET, SO_NREAD, &amount, &optlen);
#else
	res = ioctl (sock, FIONREAD, &amount);
#endif

	if (res == -1) {
		*werror = mono_w32socket_convert_error (errno);
		return 0;
	}

	return amount;
}

void
ves_icall_Mono_PAL_Sockets_Unix_Bind (gsize sock, MonoObject *sockaddr, gint32 *werror)
{
	MonoError error;
	struct sockaddr *sa;
	socklen_t sa_size;
	int ret;

	*werror = 0;

	sa = create_sockaddr_from_object (sockaddr, &sa_size, werror, &error);
	if (*werror != 0)
		return;
	if (!mono_error_ok (&error)) {
		mono_error_set_pending_exception (&error);
		return;
	}

	LOGDEBUG (g_message("%s: binding to %s port %d", __func__, inet_ntoa (((struct sockaddr_in *)sa)->sin_addr), ntohs (((struct sockaddr_in *)sa)->sin_port)));

	ret = mono_w32socket_bind (sock, sa, sa_size);
	if (ret == SOCKET_ERROR)
		*werror = mono_w32socket_get_last_error ();

	g_free (sa);
}

void
ves_icall_Mono_PAL_Sockets_Unix_Connect (gsize sock, MonoObject *sockaddr, gint32 *werror)
{
	MonoError error;
	struct sockaddr *sa;
	socklen_t sa_size;
	int ret;

	*werror = 0;

	sa = create_sockaddr_from_object (sockaddr, &sa_size, werror, &error);
	if (*werror != 0)
		return;
	if (!mono_error_ok (&error)) {
		mono_error_set_pending_exception (&error);
		return;
	}

	LOGDEBUG (g_message("%s: connecting to %s port %d", __func__, inet_ntoa (((struct sockaddr_in *)sa)->sin_addr), ntohs (((struct sockaddr_in *)sa)->sin_port)));

	ret = mono_w32socket_connect ((MonoSocket) sock, sa, sa_size);
	if (ret == SOCKET_ERROR)
		*werror = mono_w32socket_get_last_error ();

	g_free (sa);
}

gint32
ves_icall_Mono_PAL_Sockets_Unix_Receive (gsize sock, gpointer buffer, gint32 count, gint32 flags, gint32 *werror)
{
	gint recvflags;
	gint32 ret;

	*werror = 0;

	recvflags = convert_socketflags (flags);
	if (recvflags == -1) {
		*werror = WSAEOPNOTSUPP;
		return 0;
	}

	ret = mono_w32socket_recvfrom (sock, (gchar*) buffer, count, recvflags, NULL, NULL);
	if (ret == SOCKET_ERROR)
		*werror = mono_w32socket_get_last_error ();

	return ret;
}

gint32
ves_icall_Mono_PAL_Sockets_Unix_ReceiveBuffers (gsize sock, gpointer buffers, gint32 count, gint32 flags, gint32 *werror)
{
	guint32 recv;
	guint32 recvflags;
	gint32 ret;

	*werror = 0;

	recvflags = convert_socketflags (flags);
	if (recvflags == -1) {
		*werror = WSAEOPNOTSUPP;
		return 0;
	}

	ret = mono_w32socket_recvbuffers (sock, (WSABUF*) buffers, count, &recv, &recvflags);
	if (ret == SOCKET_ERROR) {
		*werror = mono_w32socket_get_last_error ();
		return 0;
	}

	return recv;
}

gint32
ves_icall_Mono_PAL_Sockets_Unix_ReceiveFrom (gsize sock, gpointer buffer, gint32 count, gint32 flags, MonoObject **sockaddr, gint32 *werror)
{
	MonoError error;
	int ret;
	int recvflags = 0;
	struct sockaddr *sa;
	socklen_t sa_size;

	*werror = 0;

	sa = create_sockaddr_from_object (*sockaddr, &sa_size, werror, &error);
	if (*werror != 0)
		return 0;
	if (!mono_error_ok (&error)) {
		mono_error_set_pending_exception (&error);
		return 0;
	}

	recvflags = convert_socketflags (flags);
	if (recvflags == -1) {
		*werror = WSAEOPNOTSUPP;
		g_free(sa);
		return 0;
	}

	ret = mono_w32socket_recvfrom (sock, (gchar*) buffer, count, recvflags, sa, &sa_size);
	if (ret == SOCKET_ERROR) {
		*werror = mono_w32socket_get_last_error ();
		g_free(sa);
		return 0;
	}

	/* If we didn't get a socket size, then we're probably a
	 * connected connection-oriented socket and the stack hasn't
	 * returned the remote address. All we can do is return null.
	 */
	if (sa_size) {
		*sockaddr = create_object_from_sockaddr (sa, sa_size, werror, &error);
		if (!mono_error_ok (&error)) {
			mono_error_set_pending_exception (&error);
			g_free (sa);
			return 0;
		}
	} else {
		*sockaddr = NULL;
	}

	g_free (sa);
	return ret;
}

gint32
ves_icall_Mono_PAL_Sockets_Unix_Send (gsize sock, gpointer buffer, gint32 count, gint32 flags, gint32 *werror)
{
	gint sendflags;
	gint ret;

	*werror = 0;

	sendflags = convert_socketflags (flags);
	if (sendflags == -1) {
		*werror = WSAEOPNOTSUPP;
		return 0;
	}

	ret = mono_w32socket_send (sock, (gchar*) buffer, count, sendflags);
	if (ret == SOCKET_ERROR) {
		*werror = mono_w32socket_get_last_error ();
		return 0;
	}

	return ret;
}

gint32
ves_icall_Mono_PAL_Sockets_Unix_SendBuffers (gsize sock, gpointer buffers, gint32 count, gint32 flags, gint32 *werror)
{
	guint32 sent;
	guint32 sendflags;
	gint ret;

	*werror = 0;

	sendflags = convert_socketflags (flags);
	if (sendflags == -1) {
		*werror = WSAEOPNOTSUPP;
		return 0;
	}

	ret = mono_w32socket_sendbuffers (sock, (WSABUF*) buffers, count, &sent, sendflags);
	if (ret == SOCKET_ERROR) {
		*werror = mono_w32socket_get_last_error ();
		return 0;
	}

	return sent;
}

gint32
ves_icall_Mono_PAL_Sockets_Unix_SendTo (gsize sock, gpointer buffer, gint32 count, gint32 flags, MonoObject *sockaddr, gint32 *werror)
{
	MonoError error;
	int ret;
	int sendflags = 0;
	struct sockaddr *sa;
	socklen_t sa_size;

	*werror = 0;

	sa = create_sockaddr_from_object(sockaddr, &sa_size, werror, &error);
	if (*werror != 0)
		return 0;
	if (!mono_error_ok (&error)) {
		mono_error_set_pending_exception (&error);
		return 0;
	}

	LOGDEBUG (g_message("%s: Sending %d bytes", __func__, count));

	sendflags = convert_socketflags (flags);
	if (sendflags == -1) {
		g_free (sa);
		*werror = WSAEOPNOTSUPP;
		return 0;
	}

	ret = mono_w32socket_sendto (sock, (gchar*) buffer, count, sendflags, sa, sa_size);
	if (ret == SOCKET_ERROR)
		*werror = mono_w32socket_get_last_error ();

	g_free(sa);

	if (*werror)
		return 0;

	return ret;
}

void
ves_icall_Mono_PAL_Sockets_Unix_SendFile (gsize sock, gpointer file, gpointer buffers, gint flags, gint32 *werror)
{
	gboolean res;

	*werror = 0;

	res = mono_w32socket_transmit_file (sock, file, (TRANSMIT_FILE_BUFFERS*) buffers, flags);
	if (!res)
		*werror = mono_w32socket_get_last_error ();
}

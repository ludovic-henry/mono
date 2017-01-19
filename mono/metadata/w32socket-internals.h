/*
* w32socket-internals.h
*
* Copyright 2016 Microsoft
* Licensed under the MIT license. See LICENSE file in the project root for full license information.
*/
#ifndef __MONO_METADATA_W32SOCKET_INTERNALS_H__
#define __MONO_METADATA_W32SOCKET_INTERNALS_H__

#include <config.h>
#include <glib.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <mono/utils/w32api.h>

#ifndef HAVE_SOCKLEN_T
#define socklen_t int
#endif

#ifndef HOST_WIN32

#define TF_DISCONNECT 0x01
#define TF_REUSE_SOCKET 0x02

#endif

void
mono_w32socket_initialize (void);

void
mono_w32socket_cleanup (void);

#ifndef HOST_WIN32

gint
mono_w32socket_getsockopt (MonoSocket sock, gint level, gint optname, gpointer optval, socklen_t *optlen);

gint
mono_w32socket_setsockopt (MonoSocket sock, gint level, gint optname, const gpointer optval, socklen_t optlen);

gint
mono_w32socket_listen (MonoSocket sock, gint backlog);

gint
mono_w32socket_shutdown (MonoSocket sock, gint how);

gint
mono_w32socket_ioctl (MonoSocket sock, gint32 command, gchar *input, gint inputlen, gchar *output, gint outputlen, glong *written);

gboolean
mono_w32socket_close (MonoSocket sock);

#endif /* HOST_WIN32 */

gint
mono_w32socket_disconnect (MonoSocket sock, gboolean reuse);

void
mono_w32socket_set_last_error (gint32 error);

gint32
mono_w32socket_get_last_error (void);

gint32
mono_w32socket_convert_error (gint error);

#endif // __MONO_METADATA_W32SOCKET_INTERNALS_H__

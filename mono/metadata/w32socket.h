/*
 * w32socket.h: System.Net.Sockets.Socket support
 *
 * Author:
 *	Dick Porter (dick@ximian.com)
 *
 * (C) 2001 Ximian, Inc.
 */

#ifndef _MONO_METADATA_W32SOCKET_H_
#define _MONO_METADATA_W32SOCKET_H_

#include <config.h>
#include <glib.h>

#include <mono/metadata/object-internals.h>

MonoBoolean
ves_icall_System_Net_Dns_GetHostByName_internal (MonoString *host, MonoString **h_name, MonoArray **h_aliases,
	MonoArray **h_addr_list, gint32 hint);

MonoBoolean
ves_icall_System_Net_Dns_GetHostByAddr_internal (MonoString *addr, MonoString **h_name, MonoArray **h_aliases,
	MonoArray **h_addr_list, gint32 hint);

MonoBoolean
ves_icall_System_Net_Dns_GetHostName_internal (MonoString **h_name);

#endif /* _MONO_METADATA_W32SOCKET_H_ */

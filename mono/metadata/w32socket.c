/*
 * socket-io.c: Socket IO internal calls
 *
 * Authors:
 *	Dick Porter (dick@ximian.com)
 *	Gonzalo Paniagua Javier (gonzalo@ximian.com)
 *
 * Copyright 2001-2003 Ximian, Inc (http://www.ximian.com)
 * Copyright 2004-2009 Novell, Inc (http://www.novell.com)
 *
 * This file has been re-licensed under the MIT License:
 * http://opensource.org/licenses/MIT
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>

#ifndef DISABLE_SOCKETS

#if defined(__APPLE__) || defined(__FreeBSD__)
#define __APPLE_USE_RFC_3542
#endif

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#ifdef HOST_WIN32
#include <ws2tcpip.h>
#else
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
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include <sys/types.h>

#include <mono/metadata/object.h>
#include <mono/metadata/exception.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/w32file.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/threads-types.h>
#include <mono/utils/mono-poll.h>
/* FIXME change this code to not mess so much with the internals */
#include <mono/metadata/class-internals.h>
#include <mono/metadata/domain-internals.h>
#include <mono/utils/mono-threads.h>
#include <mono/utils/mono-memory-model.h>
#include <mono/utils/networking.h>
#include <mono/metadata/w32handle.h>
#include <mono/metadata/w32socket.h>
#include <mono/metadata/w32error.h>

#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>     /* defines FIONBIO and FIONREAD */
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>    /* defines SIOCATMARK */
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_GETIFADDRS
// <net/if.h> must be included before <ifaddrs.h>
#include <ifaddrs.h>
#endif

#if defined(_MSC_VER) && G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT | HAVE_UWP_WINAPI_SUPPORT)
#include <MSWSock.h>
#endif

static gboolean 
addrinfo_to_IPHostEntry (MonoAddressInfo *info, MonoString **h_name, MonoArray **h_aliases, MonoArray **h_addr_list, gboolean add_local_ips, MonoError *error)
{
	gint32 count, i;
	MonoAddressEntry *ai = NULL;
	struct in_addr *local_in = NULL;
	int nlocal_in = 0;
	struct in6_addr *local_in6 = NULL;
	int nlocal_in6 = 0;
	int addr_index;
	MonoDomain *domain = mono_domain_get ();

	error_init (error);
	addr_index = 0;
	*h_aliases = mono_array_new_checked (domain, mono_get_string_class (), 0, error);
	return_val_if_nok (error, FALSE);
	if (add_local_ips) {
		local_in = (struct in_addr *) mono_get_local_interfaces (AF_INET, &nlocal_in);
		local_in6 = (struct in6_addr *) mono_get_local_interfaces (AF_INET6, &nlocal_in6);
		if (nlocal_in || nlocal_in6) {
			char addr [INET6_ADDRSTRLEN];
			*h_addr_list = mono_array_new_checked (domain, mono_get_string_class (), nlocal_in + nlocal_in6, error);
			if (!is_ok (error))
				goto leave;
			
			if (nlocal_in) {
				MonoString *addr_string;
				int i;

				for (i = 0; i < nlocal_in; i++) {
					MonoAddress maddr;
					mono_address_init (&maddr, AF_INET, &local_in [i]);
					if (mono_networking_addr_to_str (&maddr, addr, sizeof (addr))) {
						addr_string = mono_string_new (domain, addr);
						mono_array_setref (*h_addr_list, addr_index, addr_string);
						addr_index++;
					}
				}
			}

			if (nlocal_in6) {
				MonoString *addr_string;
				int i;

				for (i = 0; i < nlocal_in6; i++) {
					MonoAddress maddr;
					mono_address_init (&maddr, AF_INET6, &local_in6 [i]);
					if (mono_networking_addr_to_str (&maddr, addr, sizeof (addr))) {
						addr_string = mono_string_new (domain, addr);
						mono_array_setref (*h_addr_list, addr_index, addr_string);
						addr_index++;
					}
				}
			}

		leave:
			g_free (local_in);
			g_free (local_in6);
			if (info)
				mono_free_address_info (info);
			return is_ok (error);;
		}

		g_free (local_in);
		g_free (local_in6);
	}

	for (count = 0, ai = info->entries; ai != NULL; ai = ai->next) {
		if (ai->family != AF_INET && ai->family != AF_INET6)
			continue;
		count++;
	}

	*h_addr_list = mono_array_new_checked (domain, mono_get_string_class (), count, error);
	if (!is_ok (error))
		goto leave2;

	for (ai = info->entries, i = 0; ai != NULL; ai = ai->next) {
		MonoAddress maddr;
		MonoString *addr_string;
		char buffer [INET6_ADDRSTRLEN]; /* Max. size for IPv6 */

		if ((ai->family != PF_INET) && (ai->family != PF_INET6))
			continue;

		mono_address_init (&maddr, ai->family, &ai->address);
		if (mono_networking_addr_to_str (&maddr, buffer, sizeof (buffer)))
			addr_string = mono_string_new (domain, buffer);
		else
			addr_string = mono_string_new (domain, "");

		mono_array_setref (*h_addr_list, addr_index, addr_string);

		if (!i) {
			i++;
			if (ai->canonical_name != NULL) {
				*h_name = mono_string_new (domain, ai->canonical_name);
			} else {
				*h_name = mono_string_new (domain, buffer);
			}
		}

		addr_index++;
	}

leave2:
	if (info)
		mono_free_address_info (info);

	return is_ok (error);
}

MonoBoolean
ves_icall_System_Net_Dns_GetHostByName_internal (MonoString *host, MonoString **h_name, MonoArray **h_aliases, MonoArray **h_addr_list, gint32 hint)
{
	MonoError error;
	gboolean add_local_ips = FALSE, add_info_ok = TRUE;
	gchar this_hostname [256];
	MonoAddressInfo *info = NULL;

	char *hostname = mono_string_to_utf8_checked (host, &error);
	if (mono_error_set_pending_exception (&error))
		return FALSE;

	if (*hostname == '\0') {
		add_local_ips = TRUE;
		*h_name = host;
	}

	if (!add_local_ips && gethostname (this_hostname, sizeof (this_hostname)) != -1) {
		if (!strcmp (hostname, this_hostname)) {
			add_local_ips = TRUE;
			*h_name = host;
		}
	}

#ifdef HOST_WIN32
	// Win32 APIs already returns local interface addresses for empty hostname ("")
	// so we never want to add them manually.
	add_local_ips = FALSE;
	if (mono_get_address_info(hostname, 0, MONO_HINT_CANONICAL_NAME | hint, &info))
		add_info_ok = FALSE;
#else
	if (*hostname && mono_get_address_info (hostname, 0, MONO_HINT_CANONICAL_NAME | hint, &info))
		add_info_ok = FALSE;
#endif

	g_free(hostname);

	if (add_info_ok) {
		MonoBoolean result = addrinfo_to_IPHostEntry (info, h_name, h_aliases, h_addr_list, add_local_ips, &error);
		mono_error_set_pending_exception (&error);
		return result;
	}
	return FALSE;
}

MonoBoolean
ves_icall_System_Net_Dns_GetHostByAddr_internal (MonoString *addr, MonoString **h_name, MonoArray **h_aliases, MonoArray **h_addr_list, gint32 hint)
{
	char *address;
	struct sockaddr_in saddr;
	struct sockaddr_in6 saddr6;
	MonoAddressInfo *info = NULL;
	MonoError error;
	gint32 family;
	gchar hostname [NI_MAXHOST] = { 0 };
	gboolean ret;

	address = mono_string_to_utf8_checked (addr, &error);
	if (mono_error_set_pending_exception (&error))
		return FALSE;

	if (inet_pton (AF_INET, address, &saddr.sin_addr ) == 1) {
		family = AF_INET;
		saddr.sin_family = AF_INET;
	} else if (inet_pton (AF_INET6, address, &saddr6.sin6_addr) == 1) {
		family = AF_INET6;
		saddr6.sin6_family = AF_INET6;
	} else {
		g_free (address);
		return FALSE;
	}

	g_free (address);

	MONO_ENTER_GC_SAFE;

	switch (family) {
	case AF_INET: {
#if HAVE_SOCKADDR_IN_SIN_LEN
		saddr.sin_len = sizeof (saddr);
#endif
		ret = getnameinfo ((struct sockaddr*)&saddr, sizeof (saddr), hostname, sizeof (hostname), NULL, 0, 0) == 0;
		break;
	}
	case AF_INET6: {
#if HAVE_SOCKADDR_IN6_SIN_LEN
		saddr6.sin6_len = sizeof (saddr6);
#endif
		ret = getnameinfo ((struct sockaddr*)&saddr6, sizeof (saddr6), hostname, sizeof (hostname), NULL, 0, 0) == 0;
		break;
	}
	default:
		g_assert_not_reached ();
	}

	MONO_EXIT_GC_SAFE;

	if (!ret)
		return FALSE;

	if (mono_get_address_info (hostname, 0, hint | MONO_HINT_CANONICAL_NAME | MONO_HINT_CONFIGURED_ONLY, &info) != 0)
		return FALSE;

	MonoBoolean result = addrinfo_to_IPHostEntry (info, h_name, h_aliases, h_addr_list, FALSE, &error);
	mono_error_set_pending_exception (&error);
	return result;
}

MonoBoolean
ves_icall_System_Net_Dns_GetHostName_internal (MonoString **h_name)
{
	gchar hostname [NI_MAXHOST] = { 0 };
	int ret;

	ret = gethostname (hostname, sizeof (hostname));
	if (ret == -1)
		return FALSE;

	*h_name = mono_string_new (mono_domain_get (), hostname);

	return TRUE;
}

#endif /* #ifndef DISABLE_SOCKETS */

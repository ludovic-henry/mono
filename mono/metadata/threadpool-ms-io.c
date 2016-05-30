/*
 * threadpool-ms-io.c: Microsoft IO threadpool runtime support
 *
 * Author:
 *	Ludovic Henry (ludovic.henry@xamarin.com)
 *
 * Copyright 2015 Xamarin, Inc (http://www.xamarin.com)
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>

#ifndef DISABLE_SOCKETS

#include <mono/metadata/threadpool-ms-io.h>

MonoBoolean
ves_icall_System_IOSelector_PlatformSupportEpoll (void)
{
#if defined(HAVE_EPOLL)
	return TRUE;
#else
	return FALSE;
#endif
}

MonoBoolean
ves_icall_System_IOSelector_PlatformSupportKqueue (void)
{
#if defined(HAVE_KQUEUE)
	return TRUE;
#else
	return FALSE;
#endif
}

#else

MonoBoolean
ves_icall_System_IOSelector_HasEpollSupport (void)
{
	g_assert_not_reached ();
}

MonoBoolean
ves_icall_System_IOSelector_PlatformSupportKqueue (void)
{
	g_assert_not_reached ();
}

#endif

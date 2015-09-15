/*
 * mono-syscall-abort-posix.c: Low-level syscall aborting
 *
 * Author:
 *	Ludovic Henry (ludovic@xamarin.com)
 *
 * (C) 2015 Xamarin, Inc
 */

#include "config.h"
#include <glib.h>

#if defined (__MACH__)
#define _DARWIN_C_SOURCE 1
#endif

#include <mono/utils/mono-threads.h>

#if defined(USE_POSIX_SYSCALL_ABORT)

void
mono_threads_core_abort_syscall (MonoThreadInfo *info)
{
	/* We signal a thread to break it from the current syscall.
	 * This signal should not be interpreted as a suspend request. */
	info->syscall_break_signal = TRUE;
	if (!mono_threads_pthread_kill (info, mono_thread_get_abort_signal_num ()))
		mono_threads_add_to_pending_operation_set (info);
}

gboolean
mono_threads_core_needs_abort_syscall (void)
{
	return TRUE;
}

#endif
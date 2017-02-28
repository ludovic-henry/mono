/*
 * mono-threads-posix.c: Low-level threading, posix version
 *
 * Author:
 *	Rodrigo Kumpera (kumpera@gmail.com)
 *
 * (C) 2011 Novell, Inc
 */

#include <config.h>

/* For pthread_main_np, pthread_get_stackaddr_np and pthread_get_stacksize_np */
#if defined (__MACH__)
#define _DARWIN_C_SOURCE 1
#endif

#include <mono/utils/mono-threads.h>
#include <mono/utils/mono-coop-semaphore.h>
#include <mono/metadata/gc-internals.h>
#include <mono/utils/mono-threads-debug.h>

#include <errno.h>

#if defined(PLATFORM_ANDROID) && !defined(TARGET_ARM64) && !defined(TARGET_AMD64)
#define USE_TKILL_ON_ANDROID 1
#endif

#ifdef USE_TKILL_ON_ANDROID
extern int tkill (pid_t tid, int signal);
#endif

#if defined(_POSIX_VERSION) || defined(__native_client__)

#include <pthread.h>

#include <sys/resource.h>

int
mono_threads_pthread_kill (MonoThreadInfo *info, int signum)
{
	THREADS_SUSPEND_DEBUG ("sending signal %d to %p[%p]\n", signum, info, mono_thread_info_get_tid (info));
#ifdef USE_TKILL_ON_ANDROID
	int result, old_errno = errno;
	result = tkill (info->native_handle, signum);
	if (result < 0) {
		result = errno;
		errno = old_errno;
	}
	return result;
#elif defined(__native_client__)
	/* Workaround pthread_kill abort() in NaCl glibc. */
	return 0;
#elif !defined(HAVE_PTHREAD_KILL)
	g_error ("pthread_kill() is not supported by this platform");
#else
	return pthread_kill (mono_thread_info_get_tid (info), signum);
#endif
}

MonoNativeThreadId
mono_thread_platform_get_tid (void)
{
	return pthread_self ();
}

gboolean
mono_native_thread_id_equals (MonoNativeThreadId id1, MonoNativeThreadId id2)
{
	return pthread_equal (id1, id2);
}

void
mono_native_thread_set_name (MonoNativeThreadId tid, const char *name)
{
#ifdef __MACH__
	/*
	 * We can't set the thread name for other threads, but we can at least make
	 * it work for threads that try to change their own name.
	 */
	if (tid != mono_thread_platform_get_tid ())
		return;

	if (!name) {
		pthread_setname_np ("");
	} else {
		char n [63];

		memcpy (n, name, sizeof (n) - 1);
		n [sizeof (n) - 1] = '\0';
		pthread_setname_np (n);
	}
#elif defined (__NetBSD__)
	if (!name) {
		pthread_setname_np (tid, "%s", (void*)"");
	} else {
		char n [PTHREAD_MAX_NAMELEN_NP];

		memcpy (n, name, sizeof (n) - 1);
		n [sizeof (n) - 1] = '\0';
		pthread_setname_np (tid, "%s", (void*)n);
	}
#elif defined (HAVE_PTHREAD_SETNAME_NP)
	if (!name) {
		pthread_setname_np (tid, "");
	} else {
		char n [16];

		memcpy (n, name, sizeof (n) - 1);
		n [sizeof (n) - 1] = '\0';
		pthread_setname_np (tid, n);
	}
#endif
}

gboolean
mono_native_thread_join (MonoNativeThreadId tid)
{
	void *res;

	return !pthread_join (tid, &res);
}

#endif /* defined(_POSIX_VERSION) || defined(__native_client__) */

#if defined(USE_POSIX_BACKEND)

gboolean
mono_threads_suspend_begin_async_suspend (MonoThreadInfo *info, gboolean interrupt_kernel)
{
	int sig = interrupt_kernel ? mono_threads_suspend_get_abort_signal () :  mono_threads_suspend_get_suspend_signal ();

	if (!mono_threads_pthread_kill (info, sig)) {
		mono_threads_add_to_pending_operation_set (info);
		return TRUE;
	}
	return FALSE;
}

gboolean
mono_threads_suspend_check_suspend_result (MonoThreadInfo *info)
{
	return info->suspend_can_continue;
}

/*
This begins async resume. This function must do the following:

- Install an async target if one was requested.
- Notify the target to resume.
*/
gboolean
mono_threads_suspend_begin_async_resume (MonoThreadInfo *info)
{
	int sig = mono_threads_suspend_get_restart_signal ();

	if (!mono_threads_pthread_kill (info, sig)) {
		mono_threads_add_to_pending_operation_set (info);
		return TRUE;
	}
	return FALSE;
}

void
mono_threads_suspend_abort_syscall (MonoThreadInfo *info)
{
	/* We signal a thread to break it from the current syscall.
	 * This signal should not be interpreted as a suspend request. */
	info->syscall_break_signal = TRUE;
	if (mono_threads_pthread_kill (info, mono_threads_suspend_get_abort_signal ()) == 0) {
		mono_threads_add_to_pending_operation_set (info);
	}
}

void
mono_threads_suspend_register (MonoThreadInfo *info)
{
#if defined (PLATFORM_ANDROID)
	info->native_handle = gettid ();
#endif
}

void
mono_threads_suspend_free (MonoThreadInfo *info)
{
}

void
mono_threads_suspend_init (void)
{
}

#endif /* defined(USE_POSIX_BACKEND) */

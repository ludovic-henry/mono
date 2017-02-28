
#ifndef __MONO_UTILS_THREADS_PLATFORM_H__
#define __MONO_UTILS_THREADS_PLATFORM_H__

#include <glib.h>

#include "mono-threads.h"

void
mono_thread_platform_init (void);

gboolean
mono_thread_platform_create_thread (MonoThreadStart thread_fn, gpointer thread_data,
	gsize* const stack_size, MonoNativeThreadId *tid);

void
mono_thread_platform_exit (gsize exit_code);

void
mono_thread_platform_get_stack_bounds (guint8 **staddr, gsize *stsize);

int
mono_thread_platform_get_stack_max_size (void);

gboolean
mono_thread_platform_in_critical_region (MonoNativeThreadId tid);

gboolean
mono_thread_platform_yield (void);

#endif /* __MONO_UTILS_THREADS_PLATFORM_H__ */

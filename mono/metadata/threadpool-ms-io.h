#ifndef _MONO_THREADPOOL_MS_IO_H_
#define _MONO_THREADPOOL_MS_IO_H_

#include <config.h>
#include <glib.h>

#include <mono/metadata/object-internals.h>

typedef struct _MonoThreadPoolIO MonoThreadPoolIO;
typedef struct _MonoThreadPoolIOUpdate MonoThreadPoolIOUpdate;

static gboolean
mono_threadpool_ms_is_io (MonoObject *target, MonoObject *state)
{
#ifdef DISABLE_SOCKETS
	return FALSE;
#else
	static MonoClass *io_async_result_class = NULL;

	if (!io_async_result_class)
		io_async_result_class = mono_class_from_name (mono_defaults.corlib, "System.Threading", "IOAsyncResult");
	g_assert (io_async_result_class);

	return mono_class_has_parent (target->vtable->klass, io_async_result_class);
#endif
}

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadCreateWakeupPipes_internal (gpointer *rdhandle, gpointer *wrhandle);;

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadWakeup_internal (gpointer wrhandle);

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_SelectorThreadDrainWakeupPipes_internal (gpointer rdhandle);

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_Init_internal (MonoThreadPoolIO *ioselector, gpointer wakeup_handle);

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_Cleanup_internal (MonoThreadPoolIO *ioselector);

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_UpdateAdd_internal (MonoThreadPoolIO *ioselector, MonoThreadPoolIOUpdate update);

gint
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventWait_internal (MonoThreadPoolIO *ioselector);

gint
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventGetHandleMax_internal (MonoThreadPoolIO *ioselector);

gpointer
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventGetHandleAt_internal (MonoThreadPoolIO *ioselector, gint i, gint *operations);

void
ves_icall_System_Threading_NativeThreadPoolIOBackend_EventResetHandleAt_internal (MonoThreadPoolIO *ioselector, gint i, gint operations);

#endif /* _MONO_THREADPOOL_MS_IO_H_ */

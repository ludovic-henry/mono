#ifndef _MONO_THREADPOOL_MS_IO_H_
#define _MONO_THREADPOOL_MS_IO_H_

#include <config.h>
#include <glib.h>

#include <mono/metadata/object-internals.h>

typedef struct _MonoIOSelector MonoIOSelector;
typedef struct _MonoIOSelectorUpdate MonoIOSelectorUpdate;

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
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadCreateWakeupPipes_internal (gpointer *rdhandle, gpointer *wrhandle);

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadWakeup_internal (gpointer wrhandle);

void
ves_icall_System_Threading_NativePollingIOSelector_SelectorThreadDrainWakeupPipes_internal (gpointer rdhandle);

void
ves_icall_System_Threading_NativePollingIOSelector_Init_internal (MonoIOSelector *ioselector, gpointer wakeup_handle);

void
ves_icall_System_Threading_NativePollingIOSelector_Cleanup_internal (MonoIOSelector *ioselector);

void
ves_icall_System_Threading_NativePollingIOSelector_UpdateAdd_internal (MonoIOSelector *ioselector, MonoIOSelectorUpdate update);

gint
ves_icall_System_Threading_NativePollingIOSelector_EventWait_internal (MonoIOSelector *ioselector);

gint
ves_icall_System_Threading_NativePollingIOSelector_EventGetHandleMax_internal (MonoIOSelector *ioselector);

gpointer
ves_icall_System_Threading_NativePollingIOSelector_EventGetHandleAt_internal (MonoIOSelector *ioselector, gint i, gint *operations);

void
ves_icall_System_Threading_NativePollingIOSelector_EventResetHandleAt_internal (MonoIOSelector *ioselector, gint i, gint operations);

#endif /* _MONO_THREADPOOL_MS_IO_H_ */

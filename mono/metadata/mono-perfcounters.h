/**
 * \file
 */

#ifndef __MONO_PERFCOUNTERS_H__
#define __MONO_PERFCOUNTERS_H__

#include <config.h>
#include <glib.h>

#ifndef DISABLE_PERFCOUNTERS

#include <mono/metadata/object.h>
#include <mono/utils/mono-compiler.h>

typedef struct _MonoCounterSample MonoCounterSample;

gpointer
ves_icall_System_Diagnostics_PerformanceCounter_GetImpl (gchar *category, gchar *counter,
	gchar *instance, gchar *machine, gint32 *type, MonoBoolean *custom);

MonoBoolean
ves_icall_System_Diagnostics_PerformanceCounter_GetSample (gpointer impl, MonoBoolean only_value, MonoCounterSample *sample);

gint64
ves_icall_System_Diagnostics_PerformanceCounter_UpdateValue (gpointer impl, MonoBoolean do_incr, gint64 value);

void
ves_icall_System_Diagnostics_PerformanceCounter_FreeData (gpointer impl);

/* Category icalls */
MonoBoolean
ves_icall_System_Diagnostics_PerformanceCounterCategory_CategoryDelete (gchar *name);

MonoString*
ves_icall_System_Diagnostics_PerformanceCounterCategory_CategoryHelpInternal (gchar *category, gchar *machine);

MonoBoolean
ves_icall_System_Diagnostics_PerformanceCounterCategory_CounterCategoryExists (gchar *counter, gchar *category, gchar *machine);

MonoBoolean
ves_icall_System_Diagnostics_PerformanceCounterCategory_Create (gchar *category, gchar *help, gint32 type, MonoArray *items);

MonoBoolean
ves_icall_System_Diagnostics_PerformanceCounterCategory_InstanceExistsInternal (gchar *instance, gchar *category, gchar *machine);

MonoArray*
ves_icall_System_Diagnostics_PerformanceCounterCategory_GetCategoryNames (gchar *machine);

MonoArray*
ves_icall_System_Diagnostics_PerformanceCounterCategory_GetCounterNames (gchar *category, gchar *machine);

MonoArray*
ves_icall_System_Diagnostics_PerformanceCounterCategory_GetInstanceNames (gchar *category, gchar *machine);

typedef gboolean (*PerfCounterEnumCallback) (char *category_name, char *name, unsigned char type, gint64 value, gpointer user_data);
MONO_API void mono_perfcounter_foreach (PerfCounterEnumCallback cb, gpointer user_data);

#endif /* DISABLE_PERFCOUNTERS */

#endif /* __MONO_PERFCOUNTERS_H__ */


#ifndef _MONO_THREADPOOL_MS_IO_H_
#define _MONO_THREADPOOL_MS_IO_H_

#include <config.h>
#include <glib.h>

#include <mono/metadata/object-internals.h>
#include <mono/metadata/socket-io.h>

typedef struct _MonoIOAsyncResult MonoIOAsyncResult;

void
mono_threadpool_ms_io_remove_domain_jobs (MonoDomain *domain);
void
mono_threadpool_ms_io_cleanup (void);

void
ves_icall_System_Threading_IOSelector_Add (gpointer handle, MonoDelegate *cb, MonoIOAsyncResult *state);
void
ves_icall_System_Threading_IOSelector_Remove (gpointer handle);

#endif /* _MONO_THREADPOOL_MS_IO_H_ */

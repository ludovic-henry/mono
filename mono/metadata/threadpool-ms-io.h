#ifndef _MONO_THREADPOOL_MS_IO_H_
#define _MONO_THREADPOOL_MS_IO_H_

#include <config.h>
#include <glib.h>

#include <mono/metadata/object-internals.h>
#include <mono/metadata/socket-io.h>

typedef struct _MonoIOAsyncResult MonoIOAsyncResult;

gboolean
mono_threadpool_ms_is_io (MonoObject *target, MonoObject *state);

MonoAsyncResult *
mono_threadpool_ms_io_add (MonoAsyncResult *ares, MonoIOAsyncResult *sockares);
void
mono_threadpool_ms_io_remove_socket (int fd);
void
mono_threadpool_ms_io_remove_domain_jobs (MonoDomain *domain);
void
mono_threadpool_ms_io_cleanup (void);

void
icall_append_io_job (MonoObject *target, MonoIOAsyncResult *state);

#endif /* _MONO_THREADPOOL_MS_IO_H_ */

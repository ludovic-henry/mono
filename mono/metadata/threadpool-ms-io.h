#ifndef _MONO_THREADPOOL_MS_IO_H_
#define _MONO_THREADPOOL_MS_IO_H_

#include "object.h"

MonoBoolean
ves_icall_System_IOSelector_PlatformSupportEpoll (void);

MonoBoolean
ves_icall_System_IOSelector_PlatformSupportKqueue (void);

#endif /* _MONO_THREADPOOL_MS_IO_H_ */

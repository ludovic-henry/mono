
#ifndef __MONO_METADATA_ANDROID_H__
#define __MONO_METADATA_ANDROID_H__

#include <glib.h>

struct _monodroid_ifaddrs;

gpointer
ves_icall_System_TimezoneInfo_AndroidTimeZones_GetDefaultTimeZoneId (void);

gint32
ves_icall_System_Net_NetworkInformation_NetworkInterfaceFactory_UnixNetworkInterfaceAPI_getifaddrs (struct _monodroid_ifaddrs **ifap);

void
ves_icall_System_Net_NetworkInformation_NetworkInterfaceFactory_UnixNetworkInterfaceAPI_freeifaddrs (struct _monodroid_ifaddrs *ifa);

void
ves_icall_Mono_Unix_Android_AndroidUtils_DetectCpuAndArchitecture (guint16 *built_for_cpu, guint16 *running_on_cpu, MonoBoolean *is64bit);

#endif /* __MONO_METADATA_ANDROID_H__ */

/**
 * \file
 * UWP process support for Mono.
 *
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
*/
#include <config.h>
#include <glib.h>
#include "mono/utils/mono-compiler.h"

#if G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT)
#include <windows.h>
#include <mono/metadata/object-internals.h>
#include "mono/metadata/w32process.h"
#include "mono/metadata/w32process-internals.h"

MonoArray *
ves_icall_System_Diagnostics_Process_GetProcesses_internal (void)
{
	MonoError error;

	g_unsupported_api ("EnumProcesses");

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, "EnumProcesses");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return NULL;
}

HANDLE
ves_icall_System_Diagnostics_Process_GetProcess_internal (guint32 pid)
{
	MonoError error;

	g_unsupported_api ("OpenProcess");

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, "OpenProcess");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return NULL;
}

void
ves_icall_System_Diagnostics_FileVersionInfo_GetVersionInfo_internal (MonoObject *this_obj, MonoString *filename)
{
	MonoError error;

	g_unsupported_api ("GetFileVersionInfoSize, GetFileVersionInfo, VerQueryValue, VerLanguageName");

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, "GetFileVersionInfoSize, GetFileVersionInfo, VerQueryValue, VerLanguageName");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);
}

MonoArray *
ves_icall_System_Diagnostics_Process_GetModules_internal (MonoObject *this_obj, HANDLE process)
{
	MonoError error;

	g_unsupported_api ("EnumProcessModules, GetModuleBaseName, GetModuleFileNameEx");

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, "EnumProcessModules, GetModuleBaseName, GetModuleFileNameEx");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return NULL;
}

MonoBoolean
ves_icall_System_Diagnostics_Process_ShellExecuteEx_internal (MonoW32ProcessStartInfo *proc_start_info, MonoW32ProcessInfo *process_info)
{
	MonoError error;

	g_unsupported_api ("ShellExecuteEx");

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, "ShellExecuteEx");
	mono_error_set_pending_exception (&error);

	process_info->pid = (guint32)(-ERROR_NOT_SUPPORTED);
	SetLastError (ERROR_NOT_SUPPORTED);

	return FALSE;
}

MonoString *
ves_icall_System_Diagnostics_Process_ProcessName_internal (HANDLE process)
{
	MonoError error;
	MonoString *string;
	gunichar2 name[MAX_PATH];
	guint32 len;

	len = GetModuleFileName (NULL, name, G_N_ELEMENTS (name));
	if (len == 0)
		return NULL;

	string = mono_string_new_utf16_checked (mono_domain_get (), name, len, &error);
	if (!mono_error_ok (&error))
		mono_error_set_pending_exception (&error);

	return string;
}

MonoBoolean
ves_icall_System_Diagnostics_Process_CreateProcess_internal (MonoW32ProcessStartInfo *proc_start_info,
	HANDLE stdin_handle, HANDLE stdout_handle, HANDLE stderr_handle, MonoW32ProcessInfo *process_info)
{
	MonoError error;
	gchar *api_name;

	api_name = mono_process_info->username ? "CreateProcessWithLogonW" : "CreateProcess";

	g_unsupported_api (api_name);

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, api_name);
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return FALSE;
}

MonoBoolean
ves_icall_Microsoft_Win32_NativeMethods_GetProcessWorkingSetSize (gpointer handle, gsize *min, gsize *max)
{
	MonoError error;

	g_unsupported_api ("GetProcessWorkingSetSize");

	error_init (&error);
	mono_error_set_not_supported(&error, G_UNSUPPORTED_API, "GetProcessWorkingSetSize");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return FALSE;
}

MonoBoolean
ves_icall_Microsoft_Win32_NativeMethods_SetProcessWorkingSetSize (gpointer handle, gsize min, gsize max)
{
	MonoError error;

	g_unsupported_api ("SetProcessWorkingSetSize");

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, "SetProcessWorkingSetSize");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return FALSE;
}

gint32
ves_icall_Microsoft_Win32_NativeMethods_GetPriorityClass (gpointer handle)
{
	MonoError error;

	g_unsupported_api ("GetPriorityClass");

	error_init (&error);
	mono_error_set_not_supported (&error, G_UNSUPPORTED_API, "GetPriorityClass");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return FALSE;
}

MonoBoolean
ves_icall_Microsoft_Win32_NativeMethods_SetPriorityClass (gpointer handle, gint32 priorityClass)
{
	MonoError error;

	g_unsupported_api ("SetPriorityClass");

	error_init (&error);
	mono_error_set_not_supported(&error, G_UNSUPPORTED_API, "SetPriorityClass");
	mono_error_set_pending_exception (&error);

	SetLastError (ERROR_NOT_SUPPORTED);

	return FALSE;
}

#else /* G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT) */

MONO_EMPTY_SOURCE_FILE (process_windows_uwp);
#endif /* G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT) */

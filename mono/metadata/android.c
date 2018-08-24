
#include <config.h>
#include <glib.h>

#include <jni.h>

#include <errno.h>

#if defined(HOST_ANDROID) || defined(HOST_LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#elif defined(HOST_DARWIN)
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <mach/machine.h>
#elif defined(HOST_WIN32)
#include <windows.h>
#endif

#include "object.h"

#include "utils/mono-logger-internals.h"
#include "utils/mono-dl.h"

/* Symbols which are accessed via `DllImport("__Internal")` or `dlsym` */

struct _monodroid_ifaddrs;

MONO_API void
monodroid_free (gpointer ptr);

MONO_API gint32
_monodroid_max_gref_get (void);

MONO_API gint32
_monodroid_gref_get (void);

MONO_API void
_monodroid_gref_log (const gchar *message);

MONO_API void
_monodroid_weak_gref_new (jobject curHandle, gchar curType, jobject newHandle, gchar newType, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);

MONO_API void
_monodroid_weak_gref_delete (jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);

MONO_API void
_monodroid_lref_log_new (gint32 lrefc, jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);

MONO_API void
_monodroid_lref_log_delete (gint32 lrefc, jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);

MONO_API const gchar *
monodroid_typemap_java_to_managed (const gchar *java);

MONO_API const gchar *
monodroid_typemap_managed_to_java (const gchar *managed);

MONO_API gpointer
_monodroid_get_identity_hash_code (JNIEnv *env, gpointer v);

MONO_API void
_monodroid_gc_wait_for_bridge_processing (void);

MONO_API gint32
_monodroid_gref_log_new (jobject curHandle, gchar curType, jobject newHandle, gchar newType, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);

MONO_API void
_monodroid_gref_log_delete (jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);

MONO_API gint
_monodroid_get_dns_servers (gpointer *dns_servers_array);

MONO_API MonoBoolean
_monodroid_get_network_interface_up_state (const gchar *ifname, MonoBoolean *is_up);

MONO_API MonoBoolean
_monodroid_get_network_interface_supports_multicast (const gchar *ifname, MonoBoolean *supports_multicast);

MONO_API void
mono_jvm_initialize (JavaVM *vm);

MONO_API void
_monodroid_detect_cpu_and_architecture (gushort *built_for_cpu, gushort *running_on_cpu, guchar *is64bit);

MONO_API gint
_monodroid_getifaddrs (struct _monodroid_ifaddrs **ifap);

MONO_API void
_monodroid_freeifaddrs (struct _monodroid_ifaddrs *ifa);

MONO_API gpointer
_monodroid_timezone_get_default_id (void);

JNIEXPORT void JNICALL
Java_mono_android_Runtime_init (JNIEnv *env, jclass klass, jstring lang, jobjectArray runtimeApks, jstring runtimeNativeLibDir, jobjectArray appDirs, jobject loader, jobjectArray externalStorageDirs, jobjectArray assemblies, jstring packageName);

JNIEXPORT void JNICALL
Java_mono_android_Runtime_register (JNIEnv *, jclass, jstring, jclass, jstring);

JNIEXPORT void JNICALL
Java_mono_android_Runtime_notifyTimeZoneChanged (JNIEnv *, jclass);

JNIEXPORT jint JNICALL
Java_mono_android_Runtime_createNewContext (JNIEnv *, jclass, jobjectArray, jobjectArray, jobject);

JNIEXPORT void JNICALL
Java_mono_android_Runtime_switchToContext (JNIEnv *, jclass, jint);

JNIEXPORT void JNICALL
Java_mono_android_Runtime_destroyContexts (JNIEnv *, jclass, jintArray);

JNIEXPORT void JNICALL
Java_mono_android_Runtime_propagateUncaughtException (JNIEnv *, jclass, jobject, jthrowable);

/* Functions from `libmonodroid` that we are dynamically loading */

static struct {
	gboolean loaded;
	MonoDl *dlhandle;

	gint32 (*_monodroid_max_gref_get) (void);
	gint32 (*_monodroid_gref_get) (void);
	void (*_monodroid_gref_log) (const gchar *message);
	void (*_monodroid_weak_gref_new) (jobject curHandle, gchar curType, jobject newHandle, gchar newType, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);
	void (*_monodroid_weak_gref_delete) (jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);
	void (*_monodroid_lref_log_new) (gint32 lrefc, jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);
	void (*_monodroid_lref_log_delete) (gint32 lrefc, jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);
	const gchar* (*monodroid_typemap_java_to_managed) (const gchar *java);
	const gchar* (*monodroid_typemap_managed_to_java) (const gchar *managed);
	gpointer (*_monodroid_get_identity_hash_code) (JNIEnv *env, gpointer v);
	void (*_monodroid_gc_wait_for_bridge_processing) (void);
	gint32 (*_monodroid_gref_log_new) (jobject curHandle, gchar curType, jobject newHandle, gchar newType, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);
	void (*_monodroid_gref_log_delete) (jobject handle, gchar type, const gchar *threadName, gint32 threadId, gchar *from, gint32 from_writable);
	void (*monodroid_free) (gpointer ptr);
	void (*_monodroid_detect_cpu_and_architecture) (gushort*, gushort*, guchar*);
	gint (*_monodroid_getifaddrs) (struct _monodroid_ifaddrs**);
	void (*_monodroid_freeifaddrs) (struct _monodroid_ifaddrs*);
	gpointer (*_monodroid_timezone_get_default_id) (void);
	gint (*_monodroid_get_dns_servers) (gpointer *dns_servers_array);
	MonoBoolean (*_monodroid_get_network_interface_up_state) (const gchar *ifname, MonoBoolean *is_up);
	MonoBoolean (*_monodroid_get_network_interface_supports_multicast) (const gchar *ifname, MonoBoolean *supports_multicast);

	void (*monodroid_jvm_initialize) (JavaVM *);
	void (*monodroid_runtime_init) (JNIEnv*, jclass, jstring, jobjectArray, jstring, jobjectArray, jobject, jobjectArray, jobjectArray, jstring);
	void (*monodroid_runtime_register) (JNIEnv*, jclass, jstring, jclass, jstring);
	void (*monodroid_runtime_notifyTimeZoneChanged) (JNIEnv*, jclass);
	jint (*monodroid_runtime_createNewContext) (JNIEnv*, jclass, jobjectArray, jobjectArray, jobject);
	void (*monodroid_runtime_switchToContext) (JNIEnv*, jclass, jint);
	void (*monodroid_runtime_destroyContexts) (JNIEnv*, jclass, jintArray);
	void (*monodroid_runtime_propagateUncaughtException) (JNIEnv*, jclass, jobject, jthrowable);
} monodroid;

static gboolean initialized = FALSE;

static JavaVM *jvm;

void
mono_jvm_initialize (JavaVM *vm)
{
	if (initialized)
		return;

	jvm = vm;

	initialized = TRUE;
}

JNIEXPORT jint JNICALL
JNI_OnLoad (JavaVM *vm, gpointer reserved)
{
	mono_jvm_initialize (vm);
	return JNI_VERSION_1_6;
}

#ifdef HOST_WIN32

static char *msbuild_folder_path = NULL;

static const char*
monodroid_get_msbuild_directory_path (void)
{
	const char *suffix = "MSBuild\\Xamarin\\Android";
	char *base_path = NULL;
	wchar_t *buffer = NULL;

	if (msbuild_folder_path != NULL)
		return msbuild_folder_path;

	// Get the base path for 'Program Files' on Windows
	if (!SUCCEEDED (SHGetKnownFolderPath (&FOLDERID_ProgramFilesX86, 0, NULL, &buffer))) {
		if (buffer)
			CoTaskMemFree (buffer);
		// returns current directory if a global one couldn't be found
		return ".";
	}

	// Compute the final path
	base_path = u16to8 (buffer);
	CoTaskMemFree (buffer);
	msbuild_folder_path = g_build_filename (base_path, suffix);
	g_free (base_path);

	return msbuild_folder_path;
}

static const gchar*
monodroid_get_lib_directory_path (void)
{
	static char *libmonoandroid_directory_path = NULL;
	wchar_t module_path[MAX_PATH];
	HMODULE module = NULL;

	if (libmonoandroid_directory_path)
		return libmonoandroid_directory_path;

	DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
	if (!GetModuleHandleExW (flags, (void*)&libmonoandroid_directory_path, &module))
		return NULL;

	GetModuleFileNameW (module, module_path, sizeof (module_path) / sizeof (module_path[0]));
	PathRemoveFileSpecW (module_path);
	libmonoandroid_directory_path = u16to8 (module_path);
	return libmonoandroid_directory_path;
}

#endif /* HOST_WIN32 */

static void
monodroid_copy_file_content (const gchar *from_filename, const gchar *to_filename)
{
	FILE *from_file = NULL, *to_file = NULL;
	gchar buf[4096];
	gsize n;

	from_file = g_fopen (from_filename, "r");
	if (!from_file) {
		mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: failed to open \"%s\", error: (%d) \"%s\"", __func__, from_filename, errno, g_strerror (errno));
		goto exit;
	}

	to_file = g_fopen (to_filename, "wx");
	if (!to_file) {
		mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: failed to open \"%s\", error: (%d) \"%s\"", __func__, to_filename, errno, g_strerror (errno));
		goto exit;
	}

	while ((n = fread (buf, sizeof(char), sizeof(buf), from_file)) > 0) {
		if (fwrite (buf, sizeof(char), n, to_file) != n) {
			mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: failed to copy \"%s\" to \"%s\", error: (%d) \"%s\"", __func__, from_filename, to_filename, errno, g_strerror(errno));
			goto exit;
		}
	}

exit:
	if (from_file)
		fclose (from_file);
	if (to_file)
		fclose (to_file);
}

static void
monodroid_copy_file (const gchar *from, const gchar *to, const gchar *filename)
{
	gchar *from_filename, *to_filename;
	
	from_filename = g_build_filename (from, filename);
	g_assert (from_filename);

	if (!g_file_test (from_filename, G_FILE_TEST_EXISTS)) {
		mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: failed to copy \"%s\" from \"%s\", file doesn't exist", __func__, filename, from);
		g_free (from_filename);
		return;
	}

	mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: copying \"%s\" from \"%s\" to \"%s\"", __func__, filename, from, to);

	to_filename = g_build_filename (to, filename);
	g_assert (to_filename);

	g_unlink (to_filename);

	monodroid_copy_file_content (from_filename, to_filename);

	g_free (from_filename);
	g_free (to_filename);
}

static const gchar*
monodroid_get_path (JNIEnv *env, jstring primary_override_dir_j, jstring external_override_dir_j, jstring external_legacy_override_dir_j, jstring app_libdir_j, jstring runtime_libdir_j)
{
	char *primary_override_dir, *external_override_dir, *external_legacy_override_dir, *app_libdir, *runtime_libdir;
	const char *temp;

	temp = (*env)->GetStringUTFChars (env, primary_override_dir_j, NULL);
	primary_override_dir = g_build_filename (temp, ".__override__");
	(*env)->ReleaseStringUTFChars (env, primary_override_dir_j, temp);

	temp = (*env)->GetStringUTFChars (env, external_override_dir_j, NULL);
	external_override_dir = g_strdup (temp);
	(*env)->ReleaseStringUTFChars (env, external_override_dir_j, temp);

	temp = (*env)->GetStringUTFChars (env, external_legacy_override_dir_j, NULL);
	external_legacy_override_dir = g_strdup (temp);
	(*env)->ReleaseStringUTFChars (env, external_legacy_override_dir_j, temp);

	temp = (*env)->GetStringUTFChars (env, app_libdir_j, NULL);
	app_libdir = g_strdup (temp);
	(*env)->ReleaseStringUTFChars (env, app_libdir_j, temp);

	temp = (*env)->GetStringUTFChars (env, runtime_libdir_j, NULL);
	runtime_libdir = g_strdup (temp);
	(*env)->ReleaseStringUTFChars (env, runtime_libdir_j, temp);

#if defined(HOST_ANDROID) || defined(HOST_LINUX)
#define LIBMONODROID "libmonodroid.so"
#elif defined(HOST_DARWIN)
#define LIBMONODROID "libmonodroid.dylib"
#elif defined(HOST_WIN32)
#define LIBMONODROID "libmonodroid.dll"
#else
#error "missing definition of LIBMONODROID"
#endif

#define TRY_LOAD_LIBMONODROID(dir) \
	do { \
		if (dir) { \
			const gchar *libmonodroid = g_build_filename (dir, LIBMONODROID); \
			mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: trying to load libmonodroid from \"%s\"", __func__, libmonodroid); \
			if (g_file_test (libmonodroid, G_FILE_TEST_EXISTS)) \
				return libmonodroid; \
			g_free ((gpointer) libmonodroid); \
		} \
	} while (0)

#ifndef RELEASE
	TRY_LOAD_LIBMONODROID (primary_override_dir);

	// Android 5 includes some restrictions on loading dynamic libraries via dlopen() from
	// external storage locations so we need to file copy the shared object to an internal
	// storage location before loading it.

	monodroid_copy_file (external_override_dir, primary_override_dir, LIBMONODROID);
	TRY_LOAD_LIBMONODROID (primary_override_dir);

	monodroid_copy_file (external_legacy_override_dir, primary_override_dir, LIBMONODROID);
	TRY_LOAD_LIBMONODROID (primary_override_dir);
#endif

	TRY_LOAD_LIBMONODROID (app_libdir);

	if (runtime_libdir) {
		/* Copy libmonodroid to "`primary_override_dir`/links" */
		const gchar *libmonodroid = g_build_filename(runtime_libdir, LIBMONODROID);
		if (g_file_test (libmonodroid, G_FILE_TEST_EXISTS)) {
			const gchar *links_dir = g_build_filename (primary_override_dir, "links");
			if (!g_file_test (primary_override_dir, G_FILE_TEST_EXISTS))
				g_mkdir (primary_override_dir, 0777);
			if (!g_file_test (links_dir, G_FILE_TEST_EXISTS))
				g_mkdir (links_dir, 0777);

			const gchar *libmonodroid_link = g_build_filename (links_dir, LIBMONODROID);
			if (!g_file_test (libmonodroid_link, G_FILE_TEST_EXISTS))
				monodroid_copy_file (libmonodroid, links_dir, LIBMONODROID);

			g_free ((gpointer) libmonodroid);
			libmonodroid = libmonodroid_link;

			g_free ((gpointer) links_dir);
		}
		mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: trying to load libmonodroid from \"%s\"", __func__, libmonodroid);
		if (g_file_test (libmonodroid, G_FILE_TEST_EXISTS))
			return libmonodroid;
		g_free ((gpointer) libmonodroid);
	}

#if defined(HOST_ANDROID) && defined(ANDROID64)
	TRY_LOAD_LIBMONODROID ("/system/lib64");
#elif defined(HOST_ANDROID)
	TRY_LOAD_LIBMONODROID ("/system/lib");
#elif HOST_LINUX
	TRY_LOAD_LIBMONODROID ("/usr/lib");
#elif HOST_DARWIN
	TRY_LOAD_LIBMONODROID ("/Library/Frameworks/Xamarin.Android.framework/Libraries/");
#elif HOST_WIN32
	TRY_LOAD_LIBMONODROID (monodroid_get_lib_directory_path ());
	TRY_LOAD_LIBMONODROID (monodroid_get_msbuild_directory_path ());
#else
	TRY_LOAD_LIBMONODROID ("");
#endif

#undef TRY_LOAD_LIBMONODROID
#undef LIBMONODROID

	g_error ("%s: failed to load libmonodroid.", __func__);
}

static void
monodroid_load (const gchar *libmonodroid_path)
{
	g_assert (!monodroid.loaded);

	g_assert (libmonodroid_path);

	char *error_msg;
	monodroid.dlhandle = mono_dl_open (libmonodroid_path, MONO_DL_LAZY, &error_msg);
	if (!monodroid.dlhandle) {
		mono_trace (G_LOG_LEVEL_WARNING, MONO_TRACE_ANDROID_DEFAULT, "%s: failed to load libmonodroid, err: \"%s\"", __func__, error_msg);
		g_free (error_msg);
		return;
	}

#define LOAD_SYMBOL(symbol) \
	do { \
		error_msg = mono_dl_symbol (monodroid.dlhandle, #symbol, (gpointer*) &monodroid.symbol); \
		g_assertf(monodroid.symbol, "%s: failed to load libmonodroid symbol \"%s\", err: \"%s\"", __func__, #symbol, error_msg); \
	} while (0)

	LOAD_SYMBOL (_monodroid_gc_wait_for_bridge_processing);
	LOAD_SYMBOL (_monodroid_get_identity_hash_code);
	LOAD_SYMBOL (_monodroid_gref_get);
	LOAD_SYMBOL (_monodroid_gref_log_delete);
	LOAD_SYMBOL (_monodroid_gref_log_new);
	LOAD_SYMBOL (_monodroid_gref_log);
	LOAD_SYMBOL (_monodroid_lref_log_delete);
	LOAD_SYMBOL (_monodroid_lref_log_new);
	LOAD_SYMBOL (_monodroid_max_gref_get);
	LOAD_SYMBOL (_monodroid_weak_gref_delete);
	LOAD_SYMBOL (_monodroid_weak_gref_new);
	LOAD_SYMBOL (monodroid_typemap_java_to_managed);
	LOAD_SYMBOL (monodroid_typemap_managed_to_java);
	LOAD_SYMBOL (monodroid_free);
	LOAD_SYMBOL (_monodroid_detect_cpu_and_architecture);
	LOAD_SYMBOL (_monodroid_getifaddrs);
	LOAD_SYMBOL (_monodroid_freeifaddrs);
	LOAD_SYMBOL (_monodroid_timezone_get_default_id);
	LOAD_SYMBOL (_monodroid_get_dns_servers);
	LOAD_SYMBOL (_monodroid_get_network_interface_up_state);
	LOAD_SYMBOL (_monodroid_get_network_interface_supports_multicast);

	LOAD_SYMBOL (monodroid_jvm_initialize);
	LOAD_SYMBOL (monodroid_runtime_init);
	LOAD_SYMBOL (monodroid_runtime_register);
	LOAD_SYMBOL (monodroid_runtime_notifyTimeZoneChanged);
	LOAD_SYMBOL (monodroid_runtime_createNewContext);
	LOAD_SYMBOL (monodroid_runtime_switchToContext);
	LOAD_SYMBOL (monodroid_runtime_destroyContexts);
	LOAD_SYMBOL (monodroid_runtime_propagateUncaughtException);

#undef LOAD_SYMBOL

	monodroid.loaded = TRUE;

	monodroid.monodroid_jvm_initialize (jvm);
}

void
monodroid_free (gpointer ptr)
{
	monodroid.monodroid_free (ptr);
}

void
_monodroid_detect_cpu_and_architecture (gushort *built_for_cpu, gushort *running_on_cpu, guchar *is64bit)
{
	monodroid._monodroid_detect_cpu_and_architecture (built_for_cpu, running_on_cpu, is64bit);
}

gint
_monodroid_getifaddrs (struct _monodroid_ifaddrs **ifap)
{
	return monodroid._monodroid_getifaddrs (ifap);
}

void
_monodroid_freeifaddrs (struct _monodroid_ifaddrs *ifa)
{
	monodroid._monodroid_freeifaddrs (ifa);
}

void
_monodroid_gc_wait_for_bridge_processing (void)
{
	monodroid._monodroid_gc_wait_for_bridge_processing ();
}

gpointer
_monodroid_get_identity_hash_code (JNIEnv *env, gpointer v)
{
	return monodroid._monodroid_get_identity_hash_code (env, v);
}

gint
_monodroid_gref_get (void)
{
	return monodroid._monodroid_gref_get ();
}

void
_monodroid_gref_log (const gchar *message)
{
	monodroid._monodroid_gref_log (message);
}

void
_monodroid_gref_log_delete (jobject handle, gchar type, const gchar *threadName, gint threadId, gchar *from, gint from_writable)
{
	monodroid._monodroid_gref_log_delete (handle, type, threadName, threadId, from, from_writable);
}

gint
_monodroid_gref_log_new (jobject curHandle, gchar curType, jobject newHandle, gchar newType, const gchar *threadName, gint threadId, gchar *from, gint from_writable)
{
	return monodroid._monodroid_gref_log_new (curHandle, curType, newHandle, newType, threadName, threadId, from, from_writable);
}

void
_monodroid_lref_log_delete (gint lrefc, jobject handle, gchar type, const gchar *threadName, gint threadId, gchar *from, gint from_writable)
{
	monodroid._monodroid_lref_log_delete (lrefc, handle, type, threadName, threadId, from, from_writable);
}

void
_monodroid_lref_log_new (gint lrefc, jobject handle, gchar type, const gchar *threadName, gint threadId, gchar *from, gint from_writable)
{
	monodroid._monodroid_lref_log_new (lrefc, handle, type, threadName, threadId, from, from_writable);
}

void
_monodroid_weak_gref_delete (jobject handle, gchar type, const gchar *threadName, gint threadId, gchar *from, gint from_writable)
{
	monodroid._monodroid_weak_gref_delete (handle, type, threadName, threadId, from, from_writable);
}

void
_monodroid_weak_gref_new (jobject curHandle, gchar curType, jobject newHandle, gchar newType, const gchar *threadName, gint threadId, gchar *from, gint from_writable)
{
	monodroid._monodroid_weak_gref_new (curHandle, curType, newHandle, newType, threadName, threadId, from, from_writable);
}

gint
_monodroid_max_gref_get (void)
{
	return monodroid._monodroid_max_gref_get ();
}

gpointer
_monodroid_timezone_get_default_id (void)
{
	return monodroid._monodroid_timezone_get_default_id ();
}

const gchar *
monodroid_typemap_java_to_managed (const gchar *java)
{
	return monodroid.monodroid_typemap_java_to_managed (java);
}

const gchar *
monodroid_typemap_managed_to_java (const gchar *managed)
{
	return monodroid.monodroid_typemap_managed_to_java (managed);
}

gint
_monodroid_get_dns_servers (gpointer *dns_servers_array)
{
	return monodroid._monodroid_get_dns_servers (dns_servers_array);
}

MonoBoolean
_monodroid_get_network_interface_up_state (const gchar *ifname, MonoBoolean *is_up)
{
	return monodroid._monodroid_get_network_interface_up_state (ifname, is_up);
}

MonoBoolean
_monodroid_get_network_interface_supports_multicast (const gchar *ifname, MonoBoolean *supports_multicast)
{
	return monodroid._monodroid_get_network_interface_supports_multicast (ifname, supports_multicast);
}

JNIEXPORT void JNICALL
Java_mono_android_Runtime_init (JNIEnv *env, jclass klass, jstring lang, jobjectArray runtimeApks, jstring runtimeNativeLibDir, jobjectArray appDirs, jobject loader, jobjectArray externalStorageDirs, jobjectArray assemblies, jstring packageName)
{
	monodroid_load (
		monodroid_get_path (
			env,
			(*env)->GetObjectArrayElement (env, appDirs, 0),
			(*env)->GetObjectArrayElement (env, externalStorageDirs, 0),
			(*env)->GetObjectArrayElement (env, externalStorageDirs, 1),
			(*env)->GetObjectArrayElement (env, appDirs, 2),
			runtimeNativeLibDir));

	monodroid.monodroid_runtime_init (env, klass, lang, runtimeApks, runtimeNativeLibDir, appDirs, loader, externalStorageDirs, assemblies, packageName);
}

JNIEXPORT void JNICALL
Java_mono_android_Runtime_register (JNIEnv *env, jclass klass, jstring managedType, jclass nativeClass, jstring methods)
{
	monodroid.monodroid_runtime_register(env, klass, managedType, nativeClass, methods);
}

JNIEXPORT void JNICALL
Java_mono_android_Runtime_notifyTimeZoneChanged (JNIEnv *env, jclass klass)
{
	monodroid.monodroid_runtime_notifyTimeZoneChanged (env, klass);
}

JNIEXPORT jint JNICALL
Java_mono_android_Runtime_createNewContext (JNIEnv *env, jclass klass, jobjectArray runtimeApks, jobjectArray assemblies, jobject loader)
{
	return monodroid.monodroid_runtime_createNewContext (env, klass, runtimeApks, assemblies, loader);
}

JNIEXPORT void JNICALL
Java_mono_android_Runtime_switchToContext (JNIEnv *env, jclass klass, jint contextID)
{
	monodroid.monodroid_runtime_switchToContext (env, klass, contextID);
}

JNIEXPORT void JNICALL
Java_mono_android_Runtime_destroyContexts (JNIEnv *env, jclass klass, jintArray array)
{
	monodroid.monodroid_runtime_destroyContexts (env, klass, array);
}

JNIEXPORT void JNICALL
Java_mono_android_Runtime_propagateUncaughtException (JNIEnv *env, jclass klass, jobject javaThread, jthrowable javaException)
{
	monodroid.monodroid_runtime_propagateUncaughtException (env, klass, javaThread, javaException);
}

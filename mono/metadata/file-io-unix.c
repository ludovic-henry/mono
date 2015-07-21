/*
 * file-io.c: File IO internal calls
 *
 * Author:
 *	Dick Porter (dick@ximian.com)
 *	Gonzalo Paniagua Javier (gonzalo@ximian.com)
 *
 * Copyright 2001-2003 Ximian, Inc (http://www.ximian.com)
 * Copyright 2004-2009 Novell, Inc (http://www.novell.com)
 * Copyright 2012 Xamarin Inc (http://www.xamarin.com)
 */

#include <config.h>

#include <glib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <utime.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <mono/metadata/object.h>
#include <mono/io-layer/io-layer.h>
#include <mono/metadata/file-io.h>
#include <mono/metadata/exception.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/marshal.h>
#include <mono/utils/strenc.h>
#include <mono/utils/mono-io-portability.h>
#include <mono/utils/mono-lazy-init.h>

#if 1
#define DEBUG(...)
#else
#define DEBUG printf
#endif

#define _WAPI_PROCESS_CURRENT (gpointer)0xFFFFFFFF
#define _WAPI_THREAD_CURRENT (gpointer)0xFFFFFFFE

static mono_lazy_init_t status = MONO_LAZY_INIT_STATUS_NOT_INITIALIZED;

static GHashTable* fd_metadata_table = NULL;
static mono_mutex_t fd_metadata_table_lock;

typedef struct {
	gchar *filename;
	gint delete_on_close : 1;
} FDMetadata;

static void
initialize (void)
{
	fd_metadata_table = g_hash_table_new (g_direct_hash, g_direct_equal);
	mono_mutex_init (&fd_metadata_table_lock);
}

/* conversion functions */

static guint32
convert_mode (MonoFileMode mode)
{
	switch (mode) {
	case FileMode_CreateNew:
		return O_CREAT | O_EXCL;
		break;
	case FileMode_Create:
		return O_CREAT | O_TRUNC;
		break;
	case FileMode_Open:
		return 0;
		break;
	case FileMode_OpenOrCreate:
		return O_CREAT;
		break;
	case FileMode_Truncate:
		return O_TRUNC;
		break;
	case FileMode_Append:
		return O_CREAT;
		break;
	default:
		g_warning("System.IO.FileMode has unknown value 0x%x", mode);
		return 0;
	}
}

static guint32
convert_access (MonoFileAccess access)
{
	switch (access) {
	case FileAccess_Read:
		return O_RDONLY;
	case FileAccess_Write:
		return O_WRONLY;
	case FileAccess_ReadWrite:
		return O_RDWR;
	default:
		g_warning("System.IO.FileAccess has unknown value 0x%x", access);
		/* Safe fallback */
		return O_RDONLY;
	}
}

static guint32
convert_share (MonoFileShare share)
{
	guint32 res = 0;

	if (share & FileShare_Read)
		res |= FILE_SHARE_READ;
	if (share & FileShare_Write)
		res |= FILE_SHARE_WRITE;
	if (share & FileShare_Delete)
		res |= FILE_SHARE_DELETE;
	if (share & ~(FileShare_Read | FileShare_Write | FileShare_Delete)) {
		g_warning("System.IO.FileShare has unknown value 0x%x", share);
		/* Safe fallback */
		res = 0;
	}

	return res;
}

static guint32
convert_seekorigin (MonoSeekOrigin origin)
{
	switch (origin) {
	case SeekOrigin_Begin:
		return SEEK_SET;
	case SeekOrigin_Current:
		return SEEK_CUR;
	case SeekOrigin_End:
		return SEEK_END;
	default:
		g_warning("System.IO.SeekOrigin has unknown value 0x%x", origin);
		/* Safe fallback */
		return SEEK_CUR;
	}
}

static gint64 convert_filetime (const FILETIME *filetime)
{
	guint64 ticks = filetime->dwHighDateTime;
	ticks <<= 32;
	ticks += filetime->dwLowDateTime;
	return (gint64)ticks;
}

static void convert_win32_file_attribute_data (const WIN32_FILE_ATTRIBUTE_DATA *data, MonoIOStat *stat)
{
	stat->attributes = data->dwFileAttributes;
	stat->creation_time = convert_filetime (&data->ftCreationTime);
	stat->last_access_time = convert_filetime (&data->ftLastAccessTime);
	stat->last_write_time = convert_filetime (&data->ftLastWriteTime);
	stat->length = ((gint64)data->nFileSizeHigh << 32) | data->nFileSizeLow;
}

/* Managed file attributes have nearly but not quite the same values
 * as the w32 equivalents.
 */
static guint32 convert_attrs(MonoFileAttributes attrs)
{
	if(attrs & FileAttributes_Encrypted) {
		attrs |= FILE_ATTRIBUTE_ENCRYPTED;
	}
	
	return(attrs);
}

/*
 * On Win32, GetFileAttributes|Ex () seems to try opening the file,
 * which might lead to sharing violation errors, whereas FindFirstFile
 * always succeeds. These 2 wrappers resort to FindFirstFile if
 * GetFileAttributes|Ex () has failed.
 */
static guint32
get_file_attributes (const gunichar2 *path)
{
	guint32 res;
	WIN32_FIND_DATA find_data;
	HANDLE find_handle;
	gint32 error;

	res = GetFileAttributes (path);
	if (res != -1)
		return res;

	error = GetLastError ();

	if (error != ERROR_SHARING_VIOLATION)
		return res;

	find_handle = FindFirstFile (path, &find_data);

	if (find_handle == INVALID_HANDLE_VALUE)
		return res;

	FindClose (find_handle);

	return find_data.dwFileAttributes;
}

static gboolean
get_file_attributes_ex (const gunichar2 *path, WIN32_FILE_ATTRIBUTE_DATA *data)
{
	gboolean res;
	WIN32_FIND_DATA find_data;
	HANDLE find_handle;
	gint32 error;

	res = GetFileAttributesEx (path, GetFileExInfoStandard, data);
	if (res)
		return TRUE;

	error = GetLastError ();

	if (error != ERROR_SHARING_VIOLATION)
		return FALSE;

	find_handle = FindFirstFile (path, &find_data);

	if (find_handle == INVALID_HANDLE_VALUE)
		return FALSE;

	FindClose (find_handle);

	data->dwFileAttributes = find_data.dwFileAttributes;
	data->ftCreationTime = find_data.ftCreationTime;
	data->ftLastAccessTime = find_data.ftLastAccessTime;
	data->ftLastWriteTime = find_data.ftLastWriteTime;
	data->nFileSizeHigh = find_data.nFileSizeHigh;
	data->nFileSizeLow = find_data.nFileSizeLow;
	
	return TRUE;
}

/* Code imported from the io layer */

static void
_wapi_set_last_error_from_errno (void)
{
	SetLastError (_wapi_get_win32_file_error (errno));
}

static gint
_wapi_open (const gchar *pathname, gint flags, mode_t mode)
{
	gint fd;
	gchar *located_filename;

	MONO_PREPARE_BLOCKING

	if (flags & O_CREAT) {
		located_filename = mono_portability_find_file (pathname, FALSE);
		if (located_filename == NULL) {
			fd = open (pathname, flags, mode);
		} else {
			fd = open (located_filename, flags, mode);
			g_free (located_filename);
		}
	} else {
		fd = open (pathname, flags, mode);
		if (fd == -1 && (errno == ENOENT || errno == ENOTDIR) && IS_PORTABILITY_SET) {
			gint saved_errno = errno;
			located_filename = mono_portability_find_file (pathname, TRUE);

			if (located_filename == NULL) {
				errno = saved_errno;
			} else {
				fd = open (located_filename, flags, mode);
				g_free (located_filename);
			}
		}
	}

	MONO_FINISH_BLOCKING

	return fd;
}

static gchar*
_wapi_dirname (const gchar *filename)
{
	gchar *new_filename = g_strdup (filename);
	gchar *ret;

	MONO_PREPARE_BLOCKING

	if (IS_PORTABILITY_SET)
		g_strdelimit (new_filename, "\\", '/');

	if (IS_PORTABILITY_DRIVE && g_ascii_isalpha (new_filename[0]) && (new_filename[1] == ':')) {
		gint len = strlen (new_filename);
		g_memmove (new_filename, new_filename + 2, len - 2);
		new_filename[len - 2] = '\0';
	}

	ret = g_path_get_dirname (new_filename);
	g_free (new_filename);

	MONO_FINISH_BLOCKING

	return ret;
}

static gint
_wapi_access (const gchar *pathname, gint mode)
{
	gint ret;

	MONO_PREPARE_BLOCKING

	ret = access (pathname, mode);
	if (ret == -1 && (errno == ENOENT || errno == ENOTDIR) && IS_PORTABILITY_SET) {
		gint saved_errno = errno;
		gchar *located_filename = mono_portability_find_file (pathname, TRUE);

		if (located_filename == NULL) {
			errno = saved_errno;
		} else {
			ret = access (located_filename, mode);
			g_free (located_filename);
		}
	}

	MONO_FINISH_BLOCKING

	return ret;
}

static gint
_wapi_unlink (const gchar *pathname)
{
	gint ret;

	MONO_PREPARE_BLOCKING

	ret = unlink (pathname);
	if (ret == -1 && (errno == ENOENT || errno == ENOTDIR || errno == EISDIR) && IS_PORTABILITY_SET) {
		gint saved_errno = errno;
		gchar *located_filename = mono_portability_find_file (pathname, TRUE);

		if (located_filename == NULL) {
			errno = saved_errno;
		} else {
			ret = unlink (located_filename);
			g_free (located_filename);
		}
	}

	MONO_FINISH_BLOCKING

	return ret;
}

static int
_wapi_utime (const gchar *filename, const struct utimbuf *buf)
{
	gint ret;

	MONO_PREPARE_BLOCKING

	ret = utime (filename, buf);
	if (ret == -1 && errno == ENOENT && IS_PORTABILITY_SET) {
		gint saved_errno = errno;
		gchar *located_filename = mono_portability_find_file (filename, TRUE);

		if (located_filename == NULL) {
			errno = saved_errno;
		} else {
			ret = utime (located_filename, buf);
			g_free (located_filename);
		}
	}

	MONO_FINISH_BLOCKING

	return ret;
}

gboolean
_wapi_thread_cur_apc_pending (void);

/* System.IO.MonoIO internal calls */

MonoBoolean
ves_icall_System_IO_MonoIO_CreateDirectory (MonoString *path, gint32 *error)
{
	gboolean ret;
	MONO_PREPARE_BLOCKING
	
	*error=ERROR_SUCCESS;
	
	ret=CreateDirectory (mono_string_chars (path), NULL);
	if(ret==FALSE) {
		*error=GetLastError ();
	}

	MONO_FINISH_BLOCKING
	return(ret);
}

MonoBoolean
ves_icall_System_IO_MonoIO_RemoveDirectory (MonoString *path, gint32 *error)
{
	gboolean ret;
	MONO_PREPARE_BLOCKING
	
	*error=ERROR_SUCCESS;
	
	ret=RemoveDirectory (mono_string_chars (path));
	if(ret==FALSE) {
		*error=GetLastError ();
	}

	MONO_FINISH_BLOCKING
	return(ret);
}

static gchar *
get_search_dir (MonoString *pattern)
{
	gchar *p;
	gchar *result;

	p = mono_string_to_utf8 (pattern);
	result = g_path_get_dirname (p);
	g_free (p);
	return result;
}

static GPtrArray *
get_filesystem_entries (MonoString *path,
						 MonoString *path_with_pattern,
						 gint attrs, gint mask,
						 gint32 *error)
{
	int i;
	WIN32_FIND_DATA data;
	HANDLE find_handle;
	GPtrArray *names = NULL;
	gchar *utf8_path = NULL, *utf8_result, *full_name;
	gint32 attributes;

	mask = convert_attrs (mask);
	attributes = get_file_attributes (mono_string_chars (path));
	if (attributes != -1) {
		if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			*error = ERROR_INVALID_NAME;
			goto fail;
		}
	} else {
		*error = GetLastError ();
		goto fail;
	}
	
	find_handle = FindFirstFile (mono_string_chars (path_with_pattern), &data);
	if (find_handle == INVALID_HANDLE_VALUE) {
		gint32 find_error = GetLastError ();
		
		if (find_error == ERROR_FILE_NOT_FOUND || find_error == ERROR_NO_MORE_FILES) {
			/* No files, so just return an empty array */
			goto fail;
		}
		
		*error = find_error;
		goto fail;
	}

	utf8_path = get_search_dir (path_with_pattern);
	names = g_ptr_array_new ();

	do {
		if ((data.cFileName[0] == '.' && data.cFileName[1] == 0) ||
		    (data.cFileName[0] == '.' && data.cFileName[1] == '.' && data.cFileName[2] == 0)) {
			continue;
		}
		
		if ((data.dwFileAttributes & mask) == attrs) {
			utf8_result = g_utf16_to_utf8 (data.cFileName, -1, NULL, NULL, NULL);
			if (utf8_result == NULL) {
				continue;
			}
			
			full_name = g_build_filename (utf8_path, utf8_result, NULL);
			g_ptr_array_add (names, full_name);

			g_free (utf8_result);
		}
	} while(FindNextFile (find_handle, &data));

	if (FindClose (find_handle) == FALSE) {
		*error = GetLastError ();
		goto fail;
	}

	g_free (utf8_path);
	return names;
fail:
	if (names) {
		for (i = 0; i < names->len; i++)
			g_free (g_ptr_array_index (names, i));
		g_ptr_array_free (names, TRUE);
	}
	g_free (utf8_path);
	return FALSE;
}


MonoArray *
ves_icall_System_IO_MonoIO_GetFileSystemEntries (MonoString *path,
						 MonoString *path_with_pattern,
						 gint attrs, gint mask,
						 gint32 *error)
{
	MonoDomain *domain = mono_domain_get ();
	MonoArray *result;
	int i;
	GPtrArray *names;
	
	*error = ERROR_SUCCESS;

	MONO_PREPARE_BLOCKING
	names = get_filesystem_entries (path, path_with_pattern, attrs, mask, error);
	MONO_FINISH_BLOCKING

	if (!names) {
		// If there's no array and no error, then return an empty array.
		if (*error == ERROR_SUCCESS)
			return mono_array_new (domain, mono_defaults.string_class, 0);
		return NULL;
	}

	result = mono_array_new (domain, mono_defaults.string_class, names->len);
	for (i = 0; i < names->len; i++) {
		mono_array_setref (result, i, mono_string_new (domain, g_ptr_array_index (names, i)));
		g_free (g_ptr_array_index (names, i));
	}
	g_ptr_array_free (names, TRUE);
	return result;
}

typedef struct {
	MonoDomain *domain;
	gchar *utf8_path;
	HANDLE find_handle;
} IncrementalFind;
	
static gboolean
incremental_find_check_match (IncrementalFind *handle, WIN32_FIND_DATA *data, MonoString **result)
{
	gchar *utf8_result;
	gchar *full_name;
	
	if ((data->cFileName[0] == '.' && data->cFileName[1] == 0) || (data->cFileName[0] == '.' && data->cFileName[1] == '.' && data->cFileName[2] == 0))
		return FALSE;

	utf8_result = g_utf16_to_utf8 (data->cFileName, -1, NULL, NULL, NULL);
	if (utf8_result == NULL) 
		return FALSE;
	
	full_name = g_build_filename (handle->utf8_path, utf8_result, NULL);
	g_free (utf8_result);
	*result = mono_string_new (mono_domain_get (), full_name);
	g_free (full_name);
	
	return TRUE;
}

/* FIXME make gc suspendable */
MonoString *
ves_icall_System_IO_MonoIO_FindFirst (MonoString *path,
				      MonoString *path_with_pattern,
				      gint32 *result_attr, gint32 *error,
				      gpointer *handle)
{
	WIN32_FIND_DATA data;
	HANDLE find_handle;
	IncrementalFind *ifh;
	MonoString *result;
	
	*error = ERROR_SUCCESS;
	
	find_handle = FindFirstFile (mono_string_chars (path_with_pattern), &data);
	
	if (find_handle == INVALID_HANDLE_VALUE) {
		gint32 find_error = GetLastError ();
		*handle = NULL;
		
		if (find_error == ERROR_FILE_NOT_FOUND) 
			return NULL;
		
		*error = find_error;
		return NULL;
	}

	ifh = g_new (IncrementalFind, 1);
	ifh->find_handle = find_handle;
	ifh->utf8_path = mono_string_to_utf8 (path);
	ifh->domain = mono_domain_get ();
	*handle = ifh;

	while (incremental_find_check_match (ifh, &data, &result) == 0){
		if (FindNextFile (find_handle, &data) == FALSE){
			int e = GetLastError ();
			if (e != ERROR_NO_MORE_FILES)
				*error = e;
			return NULL;
		}
	}
	*result_attr = data.dwFileAttributes;
	
	return result;
}

/* FIXME make gc suspendable */
MonoString *
ves_icall_System_IO_MonoIO_FindNext (gpointer handle, gint32 *result_attr, gint32 *error)
{
	IncrementalFind *ifh = handle;
	WIN32_FIND_DATA data;
	MonoString *result;

	*error = ERROR_SUCCESS;
	do {
		if (FindNextFile (ifh->find_handle, &data) == FALSE){
			int e = GetLastError ();
			if (e != ERROR_NO_MORE_FILES)
				*error = e;
			return NULL;
		}
	} while (incremental_find_check_match (ifh, &data, &result) == 0);

	*result_attr = data.dwFileAttributes;
	return result;
}

int
ves_icall_System_IO_MonoIO_FindClose (gpointer handle)
{
	IncrementalFind *ifh = handle;
	gint32 error;

	MONO_PREPARE_BLOCKING
	if (FindClose (ifh->find_handle) == FALSE){
		error = GetLastError ();
	} else
		error = ERROR_SUCCESS;
	g_free (ifh->utf8_path);
	g_free (ifh);
	MONO_FINISH_BLOCKING

	return error;
}

MonoString *
ves_icall_System_IO_MonoIO_GetCurrentDirectory (gint32 *error)
{
	MonoString *result;
	gunichar2 *buf;
	int len, res_len;

	len = MAX_PATH + 1; /*FIXME this is too smal under most unix systems.*/
	buf = g_new (gunichar2, len);
	
	*error=ERROR_SUCCESS;
	result = NULL;

	res_len = GetCurrentDirectory (len, buf);
	if (res_len > len) { /*buf is too small.*/
		int old_res_len = res_len;
		g_free (buf);
		buf = g_new (gunichar2, res_len);
		res_len = GetCurrentDirectory (res_len, buf) == old_res_len;
	}
	
	if (res_len) {
		len = 0;
		while (buf [len])
			++ len;

		result = mono_string_new_utf16 (mono_domain_get (), buf, len);
	} else {
		*error=GetLastError ();
	}

	g_free (buf);
	return result;
}

MonoBoolean
ves_icall_System_IO_MonoIO_SetCurrentDirectory (MonoString *path,
						gint32 *error)
{
	gboolean ret;
	
	*error=ERROR_SUCCESS;
	
	ret=SetCurrentDirectory (mono_string_chars (path));
	if(ret==FALSE) {
		*error=GetLastError ();
	}
	
	return(ret);
}

MonoBoolean
ves_icall_System_IO_MonoIO_MoveFile (MonoString *path, MonoString *dest,
				     gint32 *error)
{
	gboolean ret;
	MONO_PREPARE_BLOCKING
	
	*error=ERROR_SUCCESS;

	ret=MoveFile (mono_string_chars (path), mono_string_chars (dest));
	if(ret==FALSE) {
		*error=GetLastError ();
	}

	MONO_FINISH_BLOCKING
	return(ret);
}

MonoBoolean
ves_icall_System_IO_MonoIO_ReplaceFile (MonoString *sourceFileName, MonoString *destinationFileName,
					MonoString *destinationBackupFileName, MonoBoolean ignoreMetadataErrors,
					gint32 *error)
{
	gboolean ret;
	gunichar2 *utf16_sourceFileName = NULL, *utf16_destinationFileName = NULL, *utf16_destinationBackupFileName = NULL;
	guint32 replaceFlags = REPLACEFILE_WRITE_THROUGH;
	MONO_PREPARE_BLOCKING

	if (sourceFileName)
		utf16_sourceFileName = mono_string_chars (sourceFileName);
	if (destinationFileName)
		utf16_destinationFileName = mono_string_chars (destinationFileName);
	if (destinationBackupFileName)
		utf16_destinationBackupFileName = mono_string_chars (destinationBackupFileName);

	*error = ERROR_SUCCESS;
	if (ignoreMetadataErrors)
		replaceFlags |= REPLACEFILE_IGNORE_MERGE_ERRORS;

	/* FIXME: source and destination file names must not be NULL, but apparently they might be! */
	ret = ReplaceFile (utf16_destinationFileName, utf16_sourceFileName, utf16_destinationBackupFileName,
			 replaceFlags, NULL, NULL);
	if (ret == FALSE)
		*error = GetLastError ();

	MONO_FINISH_BLOCKING
	return ret;
}

MonoBoolean
ves_icall_System_IO_MonoIO_CopyFile (MonoString *path, MonoString *dest,
				     MonoBoolean overwrite, gint32 *error)
{
	gboolean ret;
	MONO_PREPARE_BLOCKING
	
	*error=ERROR_SUCCESS;
	
	ret=CopyFile (mono_string_chars (path), mono_string_chars (dest), !overwrite);
	if(ret==FALSE) {
		*error=GetLastError ();
	}
	
	MONO_FINISH_BLOCKING
	return(ret);
}

MonoBoolean
ves_icall_System_IO_MonoIO_DeleteFile (MonoString *path, gint32 *error)
{
	gboolean ret;
	MONO_PREPARE_BLOCKING
	
	*error=ERROR_SUCCESS;
	
	ret=DeleteFile (mono_string_chars (path));
	if(ret==FALSE) {
		*error=GetLastError ();
	}
	
	MONO_FINISH_BLOCKING
	return(ret);
}

gint32 
ves_icall_System_IO_MonoIO_GetFileAttributes (MonoString *path, gint32 *error)
{
	gint32 ret;
	MONO_PREPARE_BLOCKING

	*error=ERROR_SUCCESS;
	
	ret=get_file_attributes (mono_string_chars (path));

	/* 
	 * The definition of INVALID_FILE_ATTRIBUTES in the cygwin win32
	 * headers is wrong, hence this temporary workaround.
	 * See
	 * http://cygwin.com/ml/cygwin/2003-09/msg01771.html
	 */
	if (ret==-1) {
	  /* if(ret==INVALID_FILE_ATTRIBUTES) { */
		*error=GetLastError ();
	}
	
	MONO_FINISH_BLOCKING
	return(ret);
}

MonoBoolean
ves_icall_System_IO_MonoIO_SetFileAttributes (MonoString *path, gint32 attrs,
					      gint32 *error)
{
	gboolean ret;
	MONO_PREPARE_BLOCKING
	
	*error=ERROR_SUCCESS;
	
	ret=SetFileAttributes (mono_string_chars (path),
			       convert_attrs (attrs));
	if(ret==FALSE) {
		*error=GetLastError ();
	}

	MONO_FINISH_BLOCKING
	return(ret);
}

gint32
ves_icall_System_IO_MonoIO_GetFileType (HANDLE handle, gint32 *error)
{
	gint fd;
	gint ret;
	struct stat statbuf;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	MONO_PREPARE_BLOCKING

	ret = fstat (fd, &statbuf);

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FILE_TYPE_UNKNOWN;
	}

#ifdef S_ISREG
	if (S_ISREG (statbuf.st_mode))
		return FILE_TYPE_DISK;
#endif

	return FILE_TYPE_UNKNOWN;
}

MonoBoolean
ves_icall_System_IO_MonoIO_GetFileStat (MonoString *path, MonoIOStat *stat,
					gint32 *error)
{
	gboolean result;
	WIN32_FILE_ATTRIBUTE_DATA data;
	MONO_PREPARE_BLOCKING

	*error=ERROR_SUCCESS;
	
	result = get_file_attributes_ex (mono_string_chars (path), &data);

	if (result) {
		convert_win32_file_attribute_data (&data, stat);
	} else {
		*error=GetLastError ();
		memset (stat, 0, sizeof (MonoIOStat));
	}

	MONO_FINISH_BLOCKING
	return result;
}

HANDLE 
ves_icall_System_IO_MonoIO_Open (MonoString *filename, gint32 mode,
		gint32 access_mode, gint32 share, gint32 options, gint32 *error)
{
	FDMetadata *metadata;
	gpointer handle;
	gpointer p1, p2;
	gunichar2 *chars;
	gchar *chars_external;
	gint flags;
	gint fd;
	mode_t perms;

	mono_lazy_initialize (&status, initialize);

	*error = ERROR_SUCCESS;

	flags  = 0;
	flags |= convert_access (access_mode);
	flags |= convert_mode (mode);

	if (options & FileOptions_Temporary) {
		perms = 0600;
	} else {
		/* we don't use sharemode, because that relates to sharing of
		 * the file when the file is open and is already handled by
		 * other code, perms instead are the on-disk permissions and
		 * this is a sane default. */
		perms = 0666;
	}

	if (options & FileOptions_Encrypted) {
		*error = ERROR_ENCRYPTION_FAILED;
		return INVALID_HANDLE_VALUE;
	}

	chars = mono_string_chars (filename);
	if (!chars) {
		DEBUG ("%s: name is NULL", __func__);

		*error = ERROR_INVALID_NAME;
		return INVALID_HANDLE_VALUE;
	}

	chars_external = mono_unicode_to_external (chars);
	if (!chars_external) {
		DEBUG("%s: unicode conversion returned NULL", __func__);

		*error = ERROR_INVALID_NAME;
		return INVALID_HANDLE_VALUE;
	}

	fd = _wapi_open (chars_external, flags, perms);

	/* If we were trying to open a directory with write permissions
	 * (e.g. O_WRONLY or O_RDWR), this call will fail with
	 * EISDIR. However, this is a bit bogus because calls to
	 * manipulate the directory (e.g. SetFileTime) will still work on
	 * the directory because they use other API calls
	 * (e.g. utime()). Hence, if we failed with the EISDIR error, try
	 * to open the directory again without write permission. */
	if (fd == -1 && errno == EISDIR) {
		/* Try again but don't try to make it writable */
		fd = _wapi_open (chars_external, flags & ~(O_RDWR|O_WRONLY), perms);
	}

	if (fd == -1) {
		DEBUG("%s: Error opening file %s: %s", __func__, filename, strerror(errno));

		if (errno != ENOENT) {
			*error = _wapi_get_win32_file_error (errno);
		} else {
			/* Check the path - if it's a missing directory then
			 * we need to set PATH_NOT_FOUND not FILE_NOT_FOUND */
			gchar *dirname;

			if (filename == NULL)
				dirname = _wapi_dirname (chars_external);
			else
				dirname = g_strdup (chars_external);

			if (_wapi_access (dirname, F_OK) == 0)
				*error = ERROR_FILE_NOT_FOUND;
			else
				*error = ERROR_PATH_NOT_FOUND;

			g_free (dirname);
		}

		g_free (chars_external);

		return INVALID_HANDLE_VALUE;
	}

#ifdef HAVE_POSIX_FADVISE
	MONO_PREPARE_BLOCKING

	if (options & FileOptions_SequentialScan)
		posix_fadvise (fd, 0, 0, POSIX_FADV_SEQUENTIAL);
	if (options & FileOptions_RandomAccess)
		posix_fadvise (fd, 0, 0, POSIX_FADV_RANDOM);

	MONO_FINISH_BLOCKING
#endif

	handle = GINT_TO_POINTER (fd);

	metadata = g_new0 (FDMetadata, 1);
	metadata->filename = g_strdup (chars_external);
	metadata->delete_on_close = (options & FileOptions_DeleteOnClose) ? 1 : 0;

	g_free (chars_external);

	mono_mutex_lock (&fd_metadata_table_lock);

	if (g_hash_table_lookup_extended (fd_metadata_table, handle, &p1, &p2))
		g_error ("fd %d already in fd_metadata_table", fd);

	g_hash_table_insert (fd_metadata_table, handle, metadata);

	mono_mutex_unlock (&fd_metadata_table_lock);

	DEBUG("%s: returning handle %p", __func__, handle);

	return handle;
}

MonoBoolean
ves_icall_System_IO_MonoIO_Close (HANDLE handle, gint32 *error)
{
	FDMetadata *metadata;
	gint fd;
	gint ret;
	gboolean removed;
	gpointer p1;

	mono_lazy_initialize (&status, initialize);

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	mono_mutex_lock (&fd_metadata_table_lock);

	if (!g_hash_table_lookup_extended (fd_metadata_table, handle, &p1, (gpointer*) &metadata))
		g_error ("fd %d not in fd_metadata_table", fd);

	removed = g_hash_table_remove (fd_metadata_table, handle);
	g_assert (removed);

	mono_mutex_unlock (&fd_metadata_table_lock);

	g_assert (metadata);

	if (metadata->delete_on_close)
		_wapi_unlink (metadata->filename);

	g_free (metadata->filename);
	g_free (metadata);

	MONO_PREPARE_BLOCKING

	ret = close (fd);

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}

	return TRUE;
}

gint32 
ves_icall_System_IO_MonoIO_Read (HANDLE handle, MonoArray *dest, gint32 offset, gint32 count, gint32 *error)
{
	gint fd;
	guchar *buffer;
	gsize bytesread;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	MONO_CHECK_ARG_NULL (dest, 0);

	if (offset > mono_array_length (dest) - count) {
		mono_set_pending_exception (mono_get_exception_argument ("array", "array too small. numBytes/offset wrong."));
		return 0;
	}

	buffer = mono_array_addr (dest, guchar, offset);

	MONO_PREPARE_BLOCKING

	do {
		bytesread = read (fd, buffer, count);
	} while (bytesread == -1 && errno == EINTR && !_wapi_thread_cur_apc_pending());

	MONO_FINISH_BLOCKING

	if (bytesread == -1) {
		DEBUG ("%s: read of handle %p error: %s", __func__, handle, strerror (errno));

		*error = _wapi_get_win32_file_error (errno);
		return -1;
	}

	return bytesread;
}

gint32 
ves_icall_System_IO_MonoIO_Write (HANDLE handle, MonoArray *src, gint32 offset, gint32 count, gint32 *error)
{
	gint fd;
	guchar *buffer;
	gsize byteswritten;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	MONO_CHECK_ARG_NULL (src, 0);

	if (offset > mono_array_length (src) - count) {
		mono_set_pending_exception (mono_get_exception_argument ("array", "array too small. numBytes/offset wrong."));
		return 0;
	}

	buffer = mono_array_addr (src, guchar, offset);

	MONO_PREPARE_BLOCKING

	do {
		byteswritten = write (fd, buffer, count);
	} while (byteswritten == -1 && errno == EINTR && !_wapi_thread_cur_apc_pending());

	MONO_FINISH_BLOCKING

	if (byteswritten == -1) {
		if (errno == EINTR) {
			byteswritten = 0;
		} else {
			DEBUG ("%s: write of handle %p error: %s", __func__, handle, strerror(errno));

			*error = _wapi_get_win32_file_error (errno);
			return -1;
		}
	}

	return byteswritten;
}

gint64
ves_icall_System_IO_MonoIO_Seek (HANDLE handle, gint64 offset, gint32 origin, gint32 *error)
{
	gint fd;
	gint whence;
	gint64 position;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	whence = convert_seekorigin (origin);

#ifndef HAVE_LARGE_FILE_SUPPORT
	offset &= 0xFFFFFFFF;
#endif

	DEBUG ("%s: moving handle %p by %lld bytes from %d", __func__, handle, offset, whence);

	MONO_PREPARE_BLOCKING

#ifdef PLATFORM_ANDROID
	/* bionic doesn't support -D_FILE_OFFSET_BITS=64 */
	position = lseek64 (fd, offset, whence);
#else
	position = lseek (fd, offset, whence);
#endif

	MONO_FINISH_BLOCKING

	if (position == -1) {
		DEBUG("%s: lseek on handle %p returned error %s", __func__, handle, strerror(errno));

		*error = _wapi_get_win32_file_error (errno);
		return INVALID_SET_FILE_POINTER;
	}

	DEBUG ("%s: lseek returns %lld", __func__, position);

	return position;
}

MonoBoolean
ves_icall_System_IO_MonoIO_Flush (HANDLE handle, gint32 *error)
{
	gint fd;
	gint res;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	MONO_PREPARE_BLOCKING

	res = fsync (fd);

	MONO_FINISH_BLOCKING

	if (res == -1) {
		DEBUG("%s: fsync of handle %p error: %s", __func__, handle, strerror(errno));

		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}

	return TRUE;
}

gint64 
ves_icall_System_IO_MonoIO_GetLength (HANDLE handle, gint32 *error)
{
	gint fd;
	gint ret;
	struct stat statbuf;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	MONO_PREPARE_BLOCKING

	ret = fstat (fd, &statbuf);

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		DEBUG ("%s: handle %p fstat failed: %s", __func__, handle, strerror (errno));

		*error = _wapi_get_win32_file_error (errno);
		return INVALID_FILE_SIZE;
	}

#ifdef BLKGETSIZE64
	if (S_ISBLK (statbuf.st_mode)) {
		guint64 length;
		if (ioctl(fd, BLKGETSIZE64, &length) == -1) {
			DEBUG ("%s: handle %p ioctl BLKGETSIZE64 failed: %s", __func__, handle, strerror (errno));

			*error = _wapi_get_win32_file_error (errno);
			return INVALID_FILE_SIZE;
		}

		DEBUG ("%s: Returning block device size %lld", __func__, (gint64) length);

		return (gint64) length;
	}
#endif

#ifdef HAVE_LARGE_FILE_SUPPORT
	return statbuf.st_size;
#else
	return statbuf.st_size & 0xFFFFFFFF;
#endif
}

/* FIXME make gc suspendable */
MonoBoolean
ves_icall_System_IO_MonoIO_SetLength (HANDLE handle, gint64 length, gint32 *error)
{
	gint fd;
	gint64 offset;
	gint64 ret;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	/* save file pointer */

	MONO_PREPARE_BLOCKING

#ifdef PLATFORM_ANDROID
	/* bionic doesn't support -D_FILE_OFFSET_BITS=64 */
	offset = lseek64 (fd, 0, SEEK_CUR);
#else
	offset = lseek (fd, 0, SEEK_CUR);
#endif

	MONO_FINISH_BLOCKING

	if (offset == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}

	/* extend or truncate */

#ifndef HAVE_LARGE_FILE_SUPPORT
	length &= 0xFFFFFFFF;
#endif

	MONO_PREPARE_BLOCKING

#ifdef PLATFORM_ANDROID
	/* bionic doesn't support -D_FILE_OFFSET_BITS=64 */
	ret = lseek64 (fd, length, SEEK_SET);
#else
	ret = lseek (fd, length, SEEK_SET);
#endif

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}

	g_assert (ret == length);

#ifndef __native_client__
	MONO_PREPARE_BLOCKING

	do {
		/* always truncate, because the extend write()
		 * adds an extra byte to the end of the file */
		ret = ftruncate (fd, length);
	} while (ret == -1 && errno == EINTR && !_wapi_thread_cur_apc_pending ());

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}
#endif

	/* restore file pointer */

	MONO_PREPARE_BLOCKING

#ifdef PLATFORM_ANDROID
	/* bionic doesn't support -D_FILE_OFFSET_BITS=64 */
	ret = lseek64 (fd, offset, SEEK_SET);
#else
	ret = lseek (fd, offset, SEEK_SET);
#endif

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}

	return TRUE;
}

MonoBoolean
ves_icall_System_IO_MonoIO_SetFileTime (HANDLE handle, gint64 creation_time, gint64 last_access_time, gint64 last_write_time, gint32 *error)
{
	FDMetadata *metadata;
	gint fd;
	gint ret;
	gpointer p1;
	struct stat statbuf;
	struct utimbuf utbuf;

	mono_lazy_initialize (&status, initialize);

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

	MONO_PREPARE_BLOCKING

	ret = fstat (fd, &statbuf);

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}

	if (last_access_time < 0) {
		utbuf.actime = statbuf.st_atime;
	} else {
		/* This is (time_t)0.  We can actually go
		 * to INT_MIN, but this will do for now. */
		if (last_access_time < 116444736000000000ULL) {
			*error = ERROR_INVALID_PARAMETER;
			return FALSE;
		}

		utbuf.actime = (last_access_time - 116444736000000000ULL) / 10000000;
	}

	if (last_write_time < 0) {
		utbuf.modtime = statbuf.st_mtime;
	} else {
		/* This is (time_t)0.  We can actually go
		 * to INT_MIN, but this will do for now. */
		if (last_write_time < 116444736000000000ULL) {
			*error = ERROR_INVALID_PARAMETER;
			return FALSE;
		}

		utbuf.modtime = (last_write_time - 116444736000000000ULL) / 10000000;
	}

	mono_mutex_lock (&fd_metadata_table_lock);

	if (!g_hash_table_lookup_extended (fd_metadata_table, handle, &p1, (gpointer*) &metadata))
		g_error ("fd %d not in fd_metadata_table", fd);

	mono_mutex_unlock (&fd_metadata_table_lock);

	ret = _wapi_utime (metadata->filename, &utbuf);
	if (ret == -1) {
		*error = _wapi_get_win32_file_error (errno);
		return FALSE;
	}

	return TRUE;
}

HANDLE 
ves_icall_System_IO_MonoIO_get_ConsoleOutput ()
{
	return GetStdHandle (STD_OUTPUT_HANDLE);
}

HANDLE 
ves_icall_System_IO_MonoIO_get_ConsoleInput ()
{
	return GetStdHandle (STD_INPUT_HANDLE);
}

HANDLE 
ves_icall_System_IO_MonoIO_get_ConsoleError ()
{
	return GetStdHandle (STD_ERROR_HANDLE);
}

MonoBoolean
ves_icall_System_IO_MonoIO_CreatePipe (HANDLE *read_handle, HANDLE *write_handle)
{
	gint ret;
	gint fds [2];

	MONO_PREPARE_BLOCKING

	ret = pipe (fds);

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		// FIXME: throw exception?
		return FALSE;
	}

	g_assert (read_handle);
	*read_handle = GINT_TO_POINTER (fds [0]);

	g_assert (write_handle);
	*write_handle = GINT_TO_POINTER (fds [1]);

	return TRUE;
}

MonoBoolean ves_icall_System_IO_MonoIO_DuplicateHandle (HANDLE source_process_handle, HANDLE source_handle, HANDLE target_process_handle,
		HANDLE *target_handle, gint32 access, gint32 inherit, gint32 options)
{
	if (source_process_handle != _WAPI_PROCESS_CURRENT || target_process_handle != _WAPI_PROCESS_CURRENT)
		return FALSE;

	g_assert (source_handle != _WAPI_THREAD_CURRENT);

	g_assert (target_handle);
	*target_handle = source_handle;

	return TRUE;
}

gunichar2 
ves_icall_System_IO_MonoIO_get_VolumeSeparatorChar ()
{
#if defined (TARGET_WIN32)
	return (gunichar2) ':';	/* colon */
#else
	return (gunichar2) '/';	/* forward slash */
#endif
}

gunichar2 
ves_icall_System_IO_MonoIO_get_DirectorySeparatorChar ()
{
#if defined (TARGET_WIN32)
	return (gunichar2) '\\';	/* backslash */
#else
	return (gunichar2) '/';	/* forward slash */
#endif
}

gunichar2 
ves_icall_System_IO_MonoIO_get_AltDirectorySeparatorChar ()
{
#if defined (TARGET_WIN32)
	return (gunichar2) '/';	/* forward slash */
#else
	if (IS_PORTABILITY_SET)
		return (gunichar2) '\\';	/* backslash */
	else
		return (gunichar2) '/';	/* forward slash */
#endif
}

gunichar2 
ves_icall_System_IO_MonoIO_get_PathSeparator ()
{
#if defined (TARGET_WIN32)
	return (gunichar2) ';';	/* semicolon */
#else
	return (gunichar2) ':';	/* colon */
#endif
}

static const gunichar2
invalid_path_chars [] = {
#if defined (TARGET_WIN32)
	0x0022,				/* double quote, which seems allowed in MS.NET but should be rejected */
	0x003c,				/* less than */
	0x003e,				/* greater than */
	0x007c,				/* pipe */
	0x0008,
	0x0010,
	0x0011,
	0x0012,
	0x0014,
	0x0015,
	0x0016,
	0x0017,
	0x0018,
	0x0019,
#endif
	0x0000				/* null */
};

MonoArray *
ves_icall_System_IO_MonoIO_get_InvalidPathChars ()
{
	MonoArray *chars;
	MonoDomain *domain;
	int i, n;

	domain = mono_domain_get ();
	n = sizeof (invalid_path_chars) / sizeof (gunichar2);
	chars = mono_array_new (domain, mono_defaults.char_class, n);

	for (i = 0; i < n; ++ i)
		mono_array_set (chars, gunichar2, i, invalid_path_chars [i]);
	
	return chars;
}

gint32
ves_icall_System_IO_MonoIO_GetTempPath (MonoString **mono_name)
{
	gunichar2 *name;
	int ret;

	MONO_PREPARE_BLOCKING
	name=g_new0 (gunichar2, 256);
	
	ret=GetTempPath (256, name);
	if(ret>255) {
		/* Buffer was too short. Try again... */
		g_free (name);
		name=g_new0 (gunichar2, ret+2);	/* include the terminator */
		ret=GetTempPath (ret, name);
	}
	MONO_FINISH_BLOCKING
	
	if(ret>0) {
		DEBUG ("%s: Temp path is [%s] (len %d)", __func__, name, ret);

		mono_gc_wbarrier_generic_store ((gpointer) mono_name,
				(MonoObject*) mono_string_new_utf16 (mono_domain_get (), name, ret));
	}

	g_free (name);
	
	return(ret);
}

void
ves_icall_System_IO_MonoIO_Lock (HANDLE handle, gint64 offset, gint64 length, gint32 *error)
{
#if defined(__native_client__)
	g_warning ("fcntl() not available on Native Client");
#else
	gint fd;
	gint ret;
	struct flock lock_data;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

#ifndef HAVE_LARGE_FILE_SUPPORT
	offset &= 0xFFFFFFFF;
	length &= 0xFFFFFFFF;
#endif

	if (offset < 0 || length < 0) {
		*error = ERROR_INVALID_PARAMETER;
		return;
	}

	lock_data.l_type = F_WRLCK;
	lock_data.l_whence = SEEK_SET;
	lock_data.l_start = offset;
	lock_data.l_len = length;

	MONO_PREPARE_BLOCKING

	do {
		ret = fcntl (fd, F_SETLK, &lock_data);
	} while (ret == -1 && errno == EINTR);

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		/* if locks are not available (NFS for example), ignore the error */
		if (errno != ENOLCK
#ifdef EOPNOTSUPP
		     && errno != EOPNOTSUPP
#endif
#ifdef ENOTSUP
		     && errno != ENOTSUP
#endif
		) {
			*error = ERROR_LOCK_VIOLATION;
		}
	}
#endif /* __native_client__ */
}

void
ves_icall_System_IO_MonoIO_Unlock (HANDLE handle, gint64 offset, gint64 length, gint32 *error)
{
#if defined(__native_client__)
	g_warning ("fcntl() not available on Native Client");
#else
	gint fd;
	gint ret;
	struct flock lock_data;

	*error = ERROR_SUCCESS;

	fd = GPOINTER_TO_INT (handle);
	g_assert (fd >= 0);

#ifndef HAVE_LARGE_FILE_SUPPORT
	offset &= 0xFFFFFFFF;
	length &= 0xFFFFFFFF;
#endif

	lock_data.l_type = F_UNLCK;
	lock_data.l_whence = SEEK_SET;
	lock_data.l_start = offset;
	lock_data.l_len = length;

	MONO_PREPARE_BLOCKING

	do {
		ret = fcntl (fd, F_SETLK, &lock_data);
	} while (ret == -1 && errno == EINTR);

	MONO_FINISH_BLOCKING

	if (ret == -1) {
		/* if locks are not available (NFS for example), ignore the error */
		if (errno != ENOLCK
#ifdef EOPNOTSUPP
		     && errno != EOPNOTSUPP
#endif
#ifdef ENOTSUP
		     && errno != ENOTSUP
#endif
		) {
			*error = ERROR_LOCK_VIOLATION;
		}
	}
#endif /* __native_client__ */
}


#include <config.h>

#if !defined(HOST_WIN32)
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#else
#include <windows.h>
#include <winbase.h>
#endif

#include "file.h"

static gint
mono_error (void)
{
#if !defined(HOST_WIN32)
	switch (errno) {
	case EACCES:       return MONO_EACCES;
	case EAGAIN:       return MONO_EAGAIN;
	case EBADF:        return MONO_EBADF;
	case EDQUOT:       return MONO_EDQUOT;
	case EEXIST:       return MONO_EEXIST;
	case EFAULT:       return MONO_EFAULT;
	case EFBIG:        return MONO_EFBIG;
	case EINTR:        return MONO_EINTR;
	case EINVAL:       return MONO_EINVAL;
	case EIO:          return MONO_EIO;
	case EISDIR:       return MONO_EISDIR;
	case ELOOP:        return MONO_ELOOP;
	case EMFILE:       return MONO_EMFILE;
	case ENAMETOOLONG: return MONO_ENAMETOOLONG;
	case ENFILE:       return MONO_ENFILE;
	case ENODEV:       return MONO_ENODEV;
	case ENOENT:       return MONO_ENOENT;
	case ENOMEM:       return MONO_ENOMEM;
	case ENOSPC:       return MONO_ENOSPC;
	case ENOTDIR:      return MONO_ENOTDIR;
	case ENXIO:        return MONO_ENXIO;
	case EOPNOTSUPP:   return MONO_EOPNOTSUPP;
	case EOVERFLOW:    return MONO_EOVERFLOW;
	case EPERM:        return MONO_EPERM;
	case EROFS:        return MONO_EROFS;
	case ETXTBSY:      return MONO_ETXTBSY;
	default:           return MONO_EUNKNOWN;
	}
#else
	switch (GetLastError ()) {
	case ERROR_ACCESS_DENIED:     return MONO_EPERM;
	case ERROR_ALREADY_EXISTS:    return MONO_EEXIST;
	case ERROR_FILE_EXISTS:       return MONO_EEXIST;
	case ERROR_FILE_NOT_FOUND:    return MONO_ENOENT;
	case ERROR_SHARING_VIOLATION: return MONO_EAGAIN;
	default:                      return MONO_EUNKNOWN;
	}
#endif
}

gint
mono_file_open (const gchar * const filename, const gint flags, gint *error)
{
	gint fd;

#if !defined(HOST_WIN32)
	if ((fd = open (filename, flags)) != -1)
		return fd;
#else /* defined(HOST_WIN32) */
	LPCTSTR filename_utf16 = NULL;
	HANDLE handle = NULL;
	DWORD access = 0;
	DWORD creation_disposition = 0;

	filename_utf16 = u8to16 (filename);

	if (flags & MONO_O_RDONLY)
		access |= GENERIC_READ;
	if (flags & MONO_O_WRONLY)
		access |= GENERIC_WRITE;

	if (flags & MONO_O_CREAT)
		creation_disposition |= (flags & MONO_O_EXCL) ? CREATE_NEW : CREATE_ALWAYS;
	else
		creation_disposition |= OPEN_EXISTING;
	if (flags & MONO_O_TRUNC)
		creation_disposition |= TRUNCATE_EXISTING

	handle = CreateFile (filename_utf16, access, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, creation_disposition, FILE_ATTRIBUTE_NORMAL, NULL)

	g_free (filename_utf16);

	if (handle != INVALID_HANDLE_VALUE)
		return GPOINTER_TO_INT (handle);
#endif /* !defined(HOST_WIN32) */

	if (error)
		*error = mono_error ();

	return -1;
}

gint
mono_file_close (const gint fd, gint *error)
{
#if !defined(HOST_WIN32)
	if (close (fd) != -1)
		return 0;
#else
	if (CloseHandle (GINT_TO_POINTER (fd)))
		return 0;
#endif

	if (error)
		*error = mono_error ();

	return -1;
}

gint
mono_file_write (const gint fd, const gchar * const buf, const gsize count, gint *error)
{
#if !defined(HOST_WIN32)
	gint written;
	if ((written = write (fd, buf, count)) != -1)
		return written;
#else /* defined(HOST_WIN32) */
	LPDWORD written = 0;
	if (WriteFile (GINT_TO_POINTER (fd), buf, count, &written, NULL))
		return written;
#endif /* !defined(HOST_WIN32) */

	if (error)
		*error = mono_error ();

	return -1;
}

gint
mono_file_flush (const gint fd, gint *error)
{
#if !defined(HOST_WIN32)
	if (fsync (fd) != -1)
		return 0;
#else
	if (FlushFileBuffers (GINT_TO_POINTER (fd)))
		return 0;
#endif

	if (error)
		*error = mono_error ();

	return -1;
}

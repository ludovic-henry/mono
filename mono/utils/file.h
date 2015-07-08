#ifndef MONO_UTILS_FILE_H_
#define MONO_UTILS_FILE_H_

#include <glib.h>

#if !defined(HOST_WIN32)
#include <fcntl.h>
#endif

#if !defined(HOST_WIN32)
#define MONO_O_RDONLY O_RDONLY
#define MONO_O_WRONLY O_WRONLY
#define MONO_O_RDWR   O_RDWR
#define MONO_O_CREAT  O_CREAT
#define MONO_O_EXCL   O_EXCL
#define MONO_O_APPEND O_APPEND
#define MONO_O_TRUNC  O_TRUNC
#else
#define MONO_O_RDONLY 0x01
#define MONO_O_WRONLY 0x02
#define MONO_O_RDWR   MONO_O_RDONLY | MONO_O_WRONLY
#define MONO_O_CREAT  0x04
#define MONO_O_EXCL   0x08
#define MONO_O_APPEND 0x10
#define MONO_O_TRUNC  0x20
#endif

#define MONO_EUNKNOWN    -0x01

#if !defined(HOST_WIN32)
#define MONO_EACCES       EACCES
#define MONO_EAGAIN       EAGAIN
#define MONO_EBADF        EBADF
#define MONO_EDQUOT       EDQUOT
#define MONO_EEXIST       EEXIST
#define MONO_EFAULT       EFAULT
#define MONO_EFBIG        EFBIG
#define MONO_EINTR        EINTR
#define MONO_EINVAL       EINVAL
#define MONO_EIO          EIO
#define MONO_EISDIR       EISDIR
#define MONO_ELOOP        ELOOP
#define MONO_EMFILE       EMFILE
#define MONO_ENAMETOOLONG ENAMETOOLONG
#define MONO_ENFILE       ENFILE
#define MONO_ENODEV       ENODEV
#define MONO_ENOENT       ENOENT
#define MONO_ENOMEM       ENOMEM
#define MONO_ENOSPC       ENOSPC
#define MONO_ENOTDIR      ENOTDIR
#define MONO_ENXIO        ENXIO
#define MONO_EOPNOTSUPP   EOPNOTSUPP
#define MONO_EOVERFLOW    EOVERFLOW
#define MONO_EPERM        EPERM
#define MONO_EROFS        EROFS
#define MONO_ETXTBSY      ETXTBSY
#else
#define MONO_EACCES       0x01
#define MONO_EAGAIN       0x02
#define MONO_EBADF        0x03
#define MONO_EDQUOT       0x04
#define MONO_EEXIST       0x05
#define MONO_EFAULT       0x06
#define MONO_EFBIG        0x07
#define MONO_EINTR        0x08
#define MONO_EINVAL       0x09
#define MONO_EIO          0x0a
#define MONO_EISDIR       0x0b
#define MONO_ELOOP        0x0c
#define MONO_EMFILE       0x0d
#define MONO_ENAMETOOLONG 0x0e
#define MONO_ENFILE       0x10
#define MONO_ENODEV       0x11
#define MONO_ENOENT       0x12
#define MONO_ENOMEM       0x13
#define MONO_ENOSPC       0x14
#define MONO_ENOTDIR      0x15
#define MONO_ENXIO        0x16
#define MONO_EOPNOTSUPP   0x17
#define MONO_EOVERFLOW    0x18
#define MONO_EPERM        0x19
#define MONO_EROFS        0x1a
#define MONO_ETXTBSY      0x1b
#endif

gint
mono_file_open (const gchar * const filename, const gint flags, gint *error);

gint
mono_file_close (const gint fd, gint *error);

gint
mono_file_write (const gint fd, const gchar * const buf, const gsize count, gint *error);

gint
mono_file_flush (const gint fd, gint *error);

#endif /* MONO_UTILS_FILE_H_ */

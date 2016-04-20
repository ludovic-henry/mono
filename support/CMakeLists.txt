#AM_CPPFLAGS =					\
#$(GLIB_CFLAGS)				\
#-I$(top_srcdir)
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${GLIB_INCLUDE_DIRECTORIES}
)

# Source code which helps implement the ANSI C standards, and thus *should* be
# portable to any platform having a C compiler.
set(MPH_C_SOURCE
  errno.c
  map.c
  map.h
  mph.h
  signal.c
  stdio.c
  string.c
  stdlib.c
)

# Source code which helps implement POSIX and other related Unix standards,
# and *may* be portable between Unix platforms.
set(MPH_UNIX_SOURCE
  dirent.c
  fcntl.c
  fstab.c
  grp.c
  macros.c
  nl.c
  nl.h
  old-map.c
  old-map.h
  pwd.c
  serial.c
  sys-mman.c
  sys-sendfile.c
  sys-socket.c
  sys-stat.c
  sys-statvfs.c
  sys-time.c
  sys-uio.c
  sys-uio.h
  sys-utsname.c
  sys-wait.c
  sys-xattr.c
  syslog.c
  time.c
  unistd.c
  utime.c
  x-struct-str.c
)

if(HOST_WIN32)
  set(MPH_SOURCE ${MPH_C_SOURCE})
  set(MPH_LIBS ${GLIB_LIBS} ${LIBICONV})
else()
  set(MPH_SOURCE ${MPH_C_SOURCE} ${MPH_UNIX_SOURCE})
  set(MPH_LIBS ${GLIB_LIBS} ${LIBICONV})
endif()

set(MINIZIP_SOURCE
  minizip/crypt.h
  minizip/ioapi.c
  minizip/ioapi.h
  minizip/unzip.c
  minizip/unzip.h
  minizip/zip.c
  minizip/zip.h
)

set(ZLIB_SOURCES
  adler32.c
  compress.c
  crc32.c
  uncompr.c
  deflate.c
  trees.c
  zutil.c
  inflate.c
  infback.c
  inftrees.c
  inffast.c
  crc32.h
  deflate.h
  inffast.h
  inffixed.h
  inflate.h
  inftrees.h
  trees.h
  zconf.h
  zlib.h
  zutil.h
)

if(HAVE_ZLIB)
  set(Z_SOURCE zlib-helper.c)
  set(Z_LIBS z)
else()
  set(Z_SOURCE zlib-helper.c ${ZLIB_SOURCES})
endif()

add_library(MonoPosixHelper SHARED ${MPH_SOURCES} ${Z_SOURCE} ${MINIZIP_SOURCE})
target_link_libraries(MonoPosixHelper ${MPH_LIBS} ${Z_LIBS} ${XATTR_LIB})
if(NOT MSVC AND NOT PLATFORM_DARWIN)
  target_link_libraries(MonoPosixHelper -Wl,--no-undefined)
endif()
add_library(MonoPosixHelper-static STATIC ${MPH_SOURCES} ${Z_SOURCE} ${MINIZIP_SOURCE})
target_link_libraries(MonoPosixHelper-static ${MPH_LIBS} ${Z_LIBS} ${XATTR_LIB})

if(NOT HOST_WINDOWS)
  add_library(MonoSupportW SHARED supportw.c support-heap.c supportw.h)
  target_link_libraries(MonoSupportW eglib ${LIBICONV})
  if(NOT PLATFORM_DARWIN)
    target_link_libraries(MonoSupportW -Wl,--no-undefined)
  endif()
  add_library(MonoSupportW-static STATIC supportw.c support-heap.c supportw.h)
  target_link_libraries(MonoSupportW-static eglib ${LIBICONV})
endif()

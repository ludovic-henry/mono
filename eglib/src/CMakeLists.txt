
set(WIN_FILES
  eglib-config.hw
  gdate-win32.c gdir-win32.c gfile-win32.c gmisc-win32.c
  gmodule-win32.c gtimer-win32.c
)

set(UNIX_FILES
  gdate-unix.c  gdir-unix.c  gfile-unix.c  gmisc-unix.c
  gmodule-unix.c gtimer-unix.c
)

# some unices and windows do not have an implementation of vasprintf
# used by eglib, use provided implementation instead
if(NEED_VASPRINTF)
  set(VASPRINTF_FILES vasprintf.c)
endif()

if(HOST_WIN32)
  set(OS_FILES ${WIN_FILES})
else()
  set(OS_FILES ${UNIX_FILES})
endif()

set(SOURCES
  eglib-remap.h
  sort.frag.h
  glib.h
  garray.c
  gbytearray.c
  gerror.c
  vasprintf.h
  ghashtable.c
  giconv.c
  gmem.c
  gmodule.h
  goutput.c
  gqsort.c
  gstr.c
  gslist.c
  gstring.c
  gptrarray.c
  glist.c
  gqueue.c
  gpath.c
  gshell.c
  gspawn.c
  gfile.c
  gfile-posix.c
  gpattern.c
  gmarkup.c
  gutf8.c
  gunicode.c
  unicode-data.h
  ${OS_FILES}
  ${VASPRINTF_FILES}
)

if(NOT MSVC)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eglib-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/eglib-config.h)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  add_definitions("/W4")
else()
  add_definitions(-Wall -D_FORTIFY_SOURCE=2)
endif()

add_library(eglib_objects OBJECT ${SOURCES})
add_library(eglib INTERFACE)
target_sources(eglib INTERFACE $<TARGET_OBJECTS:eglib_objects>)
if(HAVE_LIBICONV)
  target_link_libraries(eglib INTERFACE iconv)
endif()
if(PLATFORM_ANDROID)
  target_link_libraries(eglib INTERFACE log)
endif()

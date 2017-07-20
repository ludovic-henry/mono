
set(GNUC_PRETTY "")
set(GNUC_UNUSED "")
set(BREAKPOINT "G_STMT_START { raise (SIGTRAP); } G_STMT_END")
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  set(GNUC_UNUSED "__attribute__((__unused__))")
  set(GNUC_NORETURN "__attribute__((__noreturn__))")
  if(HOST_X86 OR HOST_X86_64)
    set(BREAKPOINT "G_STMT_START { __asm__ (\"int \$03\"); } G_STMT_END")
  endif()
endif()

test_big_endian(BIG_ENDIAN)
if(BIG_ENDIAN)
  set(ORDER "G_BIG_ENDIAN")
else()
  set(ORDER "G_LITTLE_ENDIAN")
endif()

check_c_source_compiles("\
    int foo (int i, int j);\
    #define bar(...) foo (1, __VA_ARGS__)\
    void main () {\
        bar (2);\
    }\
  " G_HAVE_ISO_VARARGS)

if(HOST_WIN32)
  if(CYGWIN)
    # Cygwin cmake needs double escapes
    set(PATHSEP "\\\\")
  else()
    set(PATHSEP "\\")
  endif()
  set(SEARCHSEP ";")
  set(OS "WIN32")
  set(PIDTYPE "void *")
else()
  set(PATHSEP "/")
  set(SEARCHSEP ":")
  set(OS "UNIX")
  set(PIDTYPE "int")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eglib-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/eglib-config.h)

set(SOURCES
  eglib-remap.h
  sort.frag.h
  glib.h
  garray.c
  gbytearray.c
  gerror.c
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
)

if(HOST_WIN32)
  set(SOURCES ${SOURCES}
    eglib-config.hw
    gdate-win32.c
    gdir-win32.c
    gfile-win32.c
    gmisc-win32.c
    gmodule-win32.c
    gmodule-win32-internals.h
    gtimer-win32.c
    gunicode-win32.c
  )
else()
  set(SOURCES ${SOURCES}
    gdate-unix.c
    gdir-unix.c
    gfile-unix.c
    gmisc-unix.c
    gmodule-unix.c
    gtimer-unix.c
  )
endif()

add_library(eglib STATIC ${SOURCES})

target_compile_definitions(eglib PRIVATE _FORTIFY_SOURCE=2 _GNU_SOURCE)

target_include_directories(eglib PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

check_library_exists(m sqrtf "" HAVE_SQRTF)
if(HAVE_SQRTF)
  target_link_libraries(eglib m)
endif()

# nanosleep may not be part of libc, also search it in other libraries
check_library_exists(rt nanosleep "" HAVE_NANOSLEEP)
if(HAVE_NANOSLEEP)
  target_link_libraries(eglib rt)
endif()

check_library_exists(dl dlopen "" HAVE_DLOPEN)
if(HAVE_DLOPEN)
  target_link_libraries(eglib dl)
endif()

if(HOST_WIN32)
  target_link_libraries(eglib m ${LIBICONV} psapi)
elseif(PLATFORM_ANDROID)
  target_link_libraries(eglib log)
endif()

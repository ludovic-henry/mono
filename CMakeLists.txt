cmake_minimum_required(VERSION 3.1)
project(mono)

include(ExternalProject)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckTypeSize)
include(CheckStructHasMember)
include(CheckSymbolExists)

# Works around a cmake bug in target_link_libraries() when linking INTERFACE libraries with transitive dependencies on
# other INTERFACE libraries.
function(target_link_object_libraries TARGET)
  get_target_property(TYPE ${TARGET} TYPE)
  if(NOT ${TYPE} STREQUAL "INTERFACE_LIBRARY")
    message(FATAL_ERROR "TARGET ${TARGET} must be an INTERFACE library")
  endif()

  foreach(LIB ${ARGN})
    get_target_property(TYPE ${TARGET} TYPE)
    if(NOT ${TYPE} STREQUAL "INTERFACE_LIBRARY")
      message(FATAL_ERROR "Argument ${LIB} must be an INTERFACE library")
    endif()
    target_sources(${TARGET} INTERFACE $<TARGET_PROPERTY:${LIB},INTERFACE_SOURCES>)

    get_target_property(LINK_LIBS ${LIB} INTERFACE_LINK_LIBRARIES)
    if(NOT "${LINK_LIBS}" STREQUAL "LINK_LIBS-NOTFOUND")
      foreach(LINK_LIB ${LINK_LIBS})
        target_link_libraries(${TARGET} INTERFACE ${LINK_LIB})
      endforeach()
    endif()
  endforeach()
endfunction()

# Like check_include_files() but sets the HAVE_* variables automatically. The var name is formed from the include
# file path, e.g. sys/event.h -> HAVE_SYS_EVENT_H.
macro(check_include_files_ext)
  foreach(F ${ARGV})
    string(REGEX REPLACE "[/\\.]" "_" VAR "${F}")
    string(TOUPPER ${VAR} VAR)
    check_include_files(${F} "HAVE_${VAR}")
  endforeach()
endmacro()

# Like check_function_exists() but allows many functions to be checked at once and automatically sets HAVE_* variables.
macro(check_functions_exists_ext)
  foreach(F ${ARGV})
    string(TOUPPER ${F} VAR)
    check_function_exists(${F} "HAVE_${VAR}")
  endforeach()
endmacro()

# Like check_type_size() but allows many types to be checked at once and automatically sets SIZEOF_* and HAVE_* variables.
macro(check_types_size_ext)
  foreach(T ${ARGV})
    string(REGEX REPLACE "^struct +" "STRUCT_" X__T "${T}")
    string(REGEX REPLACE " *[*]" "_P" X__T "${X__T}")
    string(REGEX REPLACE " " "_" X__T "${X__T}")
    string(TOUPPER ${X__T} X__T)
    set(X__SIZEOF "SIZEOF_${X__T}")
    set(X__HAVE "HAVE_${X__T}")
    check_type_size(${T} "${X__SIZEOF}")
    if(${X__SIZEOF})
      set(${X__HAVE} YES)
    endif()
  endforeach()
endmacro()

# Like check_type_size() but allows includes to be set as an arg and automatically sets SIZEOF_* and HAVE_* variables.
macro(check_type_size_ext TYPE INCLUDES)
  set(CMAKE_EXTRA_INCLUDE_FILES "${INCLUDES}")
  check_types_size_ext("${TYPE}")
  unset(CMAKE_EXTRA_INCLUDE_FILES)
endmacro()

# Like check_type_size() but allows many types to be checked at once and automatically sets SIZEOF_* and HAVE_* variables.
macro(check_struct_has_member_ext _STRUCT _MEMBERS _HEADERS)
  string(REGEX REPLACE "^struct +" "STRUCT_" X__T "${_STRUCT}")
  string(REGEX REPLACE " " "" X__T "${X__T}")
  string(REGEX REPLACE "[*]" "_P" X__T "${X__T}")
  string(TOUPPER ${X__T} X__T)
  foreach(X__MEMBER ${_MEMBERS})
    string(TOUPPER ${X__MEMBER} X__MEMBER_UPPER)
    set(X__HAVE "HAVE_${X__T}_${X__MEMBER_UPPER}")
    check_struct_has_member("${_STRUCT}" "${X__MEMBER}" "${_HEADERS}" "${X__HAVE}")
    #message(STATUS "${_STRUCT}.${X__MEMBER} : ${X__HAVE} = ${X__HAVE}")
  endforeach()
endmacro()

# Like check_c_source_compiles() but takes a snippet of code which will be inserted into a main() function.
macro(check_c_snippet_compiles SNIPPET INCLUDES VARIABLE)
  set(X__HEADER "")
  foreach(X__INCLUDE ${INCLUDES})
    set(X__HEADER "${X__HEADER}\n#include <${X__INCLUDE}>")
  endforeach()
  set(X__CODE "
    ${X__HEADER}
    int main() {
      ${SNIPPET}
      return 0;
    }")
  check_c_source_compiles("${X__CODE}" "${VARIABLE}")
endmacro()

# Searches for the specified function in the specified libraries.
macro(search_libraries_for_function FUNCTION VARIABLE)
  set(X__LIBS ${ARGV})
  list(REMOVE_AT X__LIBS 0) # Remove the function
  list(REMOVE_AT X__LIBS 0) # Remove the variable
  set(OUT_VAR "HAVE_${FUNCTION}_IN_STDLIB")
  check_function_exists(${FUNCTION} ${OUT_VAR})
  if(${OUT_VAR})
    set("${VARIABLE}" "")
  else()
    foreach(X__LIB ${X__LIBS})
      set(OUT_VAR "HAVE_${FUNCTION}_IN_${X__LIB}")
      check_library_exists(${X__LIB} ${FUNCTION} ${X__LIB} ${OUT_VAR})
      if(${OUT_VAR})
        set(${VARIABLE} ${X__LIB})
        break()
      endif()
    endforeach()
  endif()
endmacro()

# Sets up ${PREFIX}_OS and other variables given an OS name.
macro(set_os NAME PREFIX)
  if(${NAME} MATCHES "Darwin")
    set("${PREFIX}_OS" "Darwin")
    set("${PREFIX}_DARWIN" YES)
    set("${PREFIX}_UNIX" YES)
    set("${PREFIX}_MACH" YES)
  elseif(${NAME} MATCHES "Linux")
    set("${PREFIX}_OS" "Linux")
    set("${PREFIX}_LINUX" YES)
    set("${PREFIX}_UNIX" YES)
  elseif(${NAME} MATCHES "Windows")
    set("${PREFIX}_OS" "Windows")
    set("${PREFIX}_WIN32" YES)
    set("${PREFIX}_WINDOWS" YES)
  else()
    message(FATAL_ERROR "Unsupported OS name ${NAME}")
  endif()
endmacro()

# Sets up ${PREFIX}_CPU and other variables given a processor.
macro(set_cpu PROCESSOR PREFIX)
  if(${PROCESSOR} MATCHES "(x86_64|amd64|AMD64)")
    set("${PREFIX}_CPU" "x86_64")
    set("${PREFIX}_X86_64" YES)
    set("${PREFIX}_AMD64" YES)
    set("${PREFIX}_64BIT" YES)
  elseif(${PROCESSOR} MATCHES "(x86|i.86)")
    set("${PREFIX}_CPU" "x86")
    set("${PREFIX}_X86" YES)
    set("${PREFIX}_I386" YES)
    set("${PREFIX}_32BIT" YES)
  else()
    message(FATAL_ERROR "Unsupported CPU ${PROCESSOR}")
  endif()
endmacro()

# Sets up ${PREFIX}_CPU and other variables given a processor.
macro(set_triple OS CPU OUT_VAR)
  string(TOLOWER "${CPU}-unknown-${OS}" ${OUT_VAR})
endmacro()

message(STATUS "CMAKE_GENERATOR = ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_VERSION = ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_HOST_SYSTEM = ${CMAKE_HOST_SYSTEM}")
message(STATUS "CMAKE_HOST_SYSTEM_NAME = ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "CMAKE_HOST_SYSTEM_VERSION = ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR = ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_CROSSCOMPILING = ${CMAKE_CROSSCOMPILING}")
message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")

# Set up BUILD_*, HOST_* and TARGET_* variables. NOTE: Beware of naming confusion. With autotools the "build platform"
# is the platform we are building on, "host platform" is the system where built programs and libraries will run and
# "target platform" is the system for which the tools will create output when building a cross-compiler. CMake uses
# CMAKE_HOST_SYSTEM_* for the platform we are building on and CMAKE_SYSTEM_* for the system where built binaries will
# run. We use the autotools naming. So if we are cross-compiling a mono on a Linux system which will run on OS X and
# output iOS binaries the build platform is Linux, the host platform is OS X and the target platform is iOS.

if(MSVC)
  # We cannot rely on CMAKE_SYSTEM_PROCESSOR on Windows with MSVC
  check_symbol_exists("_M_AMD64" "" MSVC_X64)
  if(MSVC_X64)
    set_cpu("amd64" "BUILD")
    set_cpu("amd64" "HOST")
    set_cpu("amd64" "TARGET")
  else()
    set_cpu("x86" "BUILD")
    set_cpu("x86" "HOST")
    set_cpu("x86" "TARGET")
  endif()
else()
  set_cpu(${CMAKE_HOST_SYSTEM_PROCESSOR} "BUILD")
  set_cpu(${CMAKE_SYSTEM_PROCESSOR} "HOST")
  # TODO: Support "three-way cross-compilation"
  set_cpu(${CMAKE_SYSTEM_PROCESSOR} "TARGET")
endif()

set_os(${CMAKE_HOST_SYSTEM_NAME} "BUILD")
set_triple(${BUILD_OS} ${BUILD_CPU} BUILD_TRIPLE)
set_os(${CMAKE_SYSTEM_NAME} "HOST")
set_triple(${HOST_OS} ${HOST_CPU} HOST_TRIPLE)
# TODO: Support "three-way cross-compilation"
set_os(${CMAKE_SYSTEM_NAME} "TARGET")
set_triple(${TARGET_OS} ${TARGET_CPU} TARGET_TRIPLE)

message(STATUS "BUILD_OS = ${BUILD_OS}")
message(STATUS "BUILD_CPU = ${BUILD_CPU}")
message(STATUS "BUILD_TRIPLE = ${BUILD_TRIPLE}")
message(STATUS "HOST_OS = ${HOST_OS}")
message(STATUS "HOST_CPU = ${HOST_CPU}")
message(STATUS "HOST_TRIPLE = ${HOST_TRIPLE}")
message(STATUS "TARGET_OS = ${TARGET_OS}")
message(STATUS "TARGET_CPU = ${TARGET_CPU}")
message(STATUS "TARGET_TRIPLE = ${TARGET_TRIPLE}")

if(NOT HOST_TRIPLE STREQUAL TARGET_TRIPLE)
  set(MONO_CROSS_COMPILE YES)
endif()

# Package info
set(PACKAGE_NAME mono)
set(PACKAGE "${PACKAGE_NAME}")
set(PACKAGE_VERSION 4.5.0)
set(PACKAGE_BUGREPORT "http://bugzilla.xamarin.com/enter_bug.cgi?classification=Mono")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_URL "")

set(API_VER 2.0)

# These variables are passed to the libgc build
set(LIBGC_COMPILE_OPTIONS)
set(LIBGC_COMPILE_DEFINITIONS)

# These variables are the CPPFLAGS/CFLAGS passed to the eglib build
set(CPPFLAGS_FOR_EGLIB ${CPPFLAGS})
set(CFLAGS_FOR_EGLIB ${CFLAGS})

# libgc uses some deprecated APIs
if(NOT MSVC)
  set(LIBGC_COMPILE_OPTIONS ${LIBGC_COMPILE_OPTIONS} -Wno-deprecated-declarations)
endif()

#
# These are the flags that need to be stored in the mono.pc file for
# compiling code that will embed Mono
#
set(LIBMONO_CFLAGS "")
set(LIBMONO_LDFLAGS "")

## TODO: Port to cmake
# Variable to have relocatable .pc files (lib, or lib64)
#reloc_libdir=`basename ${libdir}`
#AC_SUBST(reloc_libdir)

# Set to yes if Unix sockets cannot be created in an anonymous namespace
set(NEED_LINK_UNLINK NO)

# Set to extra linker flags to be passed to the runtime binaries (mono /mono-sgen)
set(EXTRA_RUNTIME_LDFLAGS "")

# Thread configuration inspired by sleepycat's db
set(LIBGC_THREADS NO)
set(HAS_DTRACE NO)
set(PARALLEL_MARK YES)
set(IKVM_NATIVE YES)
set(MONO_DL_NEED_USCORE NO)

if(HOST_WINDOWS)
  set(DISABLE_PORTABILITY YES)
  set(PLATFORM_NO_SYMLINKS YES)
  # mono_cv_clang=no
  # Windows Vista or later is required
  set(CPPFLAGS "${CPPFLAGS} -DWINVER=0x0600 -D_WIN32_WINNT=0x0600 -D_WIN32_IE=0x0501 -D_UNICODE -DUNICODE -DWIN32_THREADS -DFD_SETSIZE=1024 -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS")
#  set(LDFLAGS "${LDFLAGS} -lmswsock -lws2_32 -lole32 -loleaut32 -lpsapi -lversion -ladvapi32 -lwinmm -lkernel32 -liphlpapi")
  set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -D_CRT_SECURE_NO_WARNINGS)
  set(LIBMONO_CFLAGS "-mms-bitfields -mwindows")
  set(LIBMONO_LDFLAGS "-mms-bitfields -mwindows")
  set(LIBDL "")
  set(LIBGC_THREADS win32)
  set(WITH_SIGALTSTACK NO)
  set(WITH_TLS pthread)
  # This forces libgc to use the DllMain based thread registration code on win32
  set(LIBGC_ENABLE_WIN32_DLLMAIN YES)
elseif(HOST_LINUX)
  set(CPPFLAGS "${CPPFLAGS} -DGC_LINUX_THREADS -D_GNU_SOURCE -D_REENTRANT -DUSE_MMAP")
  set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DUSE_MMAP)
  set(LIBMONO_CFLAGS "-D_REENTRANT")
  set(LIBDL "-ldl")
  set(LIBGC_THREADS pthreads)
  set(USE_SIGPOSIX YES)
  if(HOST_AARCH64)
    set(DISABLE_BOEHM YES)
    set(WITH_GC sgen)
  elseif(HOST_POWERPC)
    # https://bugzilla.novell.com/show_bug.cgi?id=504411
    set(DISABLE_MUNMAP YES)
  endif()

  if(NOT DISABLE_MUNMAP)
    set(CPPFLAGS "${CPPFLAGS} -DUSE_MUNMAP")
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DUSE_MUNMAP)
  endif()

elseif(HOST_DARWIN)
  set(PARALLEL_MARK NO)
  set(CPPFLAGS "${CPPFLAGS} -D_THREAD_SAFE -DGC_MACOSX_THREADS -DPLATFORM_MACOSX -DUSE_MMAP -DUSE_MUNMAP")
  set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DUSE_MMAP -DUSE_MUNMAP -DGC_MACOSX_THREADS)
  set(LIBMONO_CFLAGS "-D_THREAD_SAFE")
  set(NEED_LINK_UNLINK YES)
  set(PTHREAD_POINTER_ID YES)
  set(USE_MACH_SEMA YES)
  set(LIBDL "")
  set(LIBGC_THREADS pthreads)
  set(HAS_DTRACE YES)
  if(CMAKE_CROSSCOMPILING)
    set(HAS_BROKEN_APPLE_CPP YES)
  endif()
  # Snow Leopard is horribly broken -- it reports itself as i386-apple-darwin*, but
  # its gcc defaults to 64-bit mode.  They have also deprecated the usage of ucontext
  # we need to set some flags to build our 32-bit binaries on 10.6 properly
  if(HOST_X86)
    set(BROKEN_DARWIN_FLAGS "-arch i386 -D_XOPEN_SOURCE")
    set(BROKEN_DARWIN_CPPFLAGS "-D_XOPEN_SOURCE")
    set(CPPFLAGS "${CPPFLAGS} ${BROKEN_DARWIN_CPPFLAGS}")
    set(CFLAGS "${CFLAGS} ${BROKEN_DARWIN_FLAGS}")
    set(CXXFLAGS "${CXXFLAGS} ${BROKEN_DARWIN_FLAGS}")
    set(CCASFLAGS "${CCASFLAGS} ${BROKEN_DARWIN_FLAGS}")
    set(LIBGC_COMPILE_OPTIONS ${LIBGC_COMPILE_OPTIONS} "-arch i386")
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -D_XOPEN_SOURCE)
    set(CPPFLAGS_FOR_EGLIB "${CPPFLAGS_FOR_EGLIB} ${BROKEN_DARWIN_CPPFLAGS}")
    set(CFLAGS_FOR_EGLIB "${CFLAGS_FOR_EGLIB} ${BROKEN_DARWIN_FLAGS}")
  elseif(HOST_ARM OR HOST_ARM64)
    set(HAS_DTRACE NO)
  endif()
endif()

if(TARGET_LINUX)
  set(PLATFORM_LINUX YES)
elseif(TARGET_DARWIN)
  set(PLATFORM_DARWIN YES)
elseif(TARGET_ANDROID)
  set(PLATFORM_ANDROID YES)
endif()
if(USE_SIGPOSIX)
  set(PLATFORM_SIGPOSIX YES)
endif()

# Sort of emulate AM_ICONV
check_library_exists(iconv iconv "" HAVE_LIBICONV)
check_include_files(iconv.h HAVE_ICONV_H)
if(HAVE_LIBICONV)
  set(CMAKE_REQUIRED_LIBRARIES iconv)
  check_function_exists(iconv HAVE_ICONV)
  set(CMAKE_REQUIRED_LIBRARIES)
else()
  check_function_exists(iconv HAVE_ICONV)
endif()
if(HAVE_LIBICONV AND NOT HAVE_ICONV)
  set(HAVE_LIBICONV)
endif()
if(HAVE_ICONV)
  check_c_source_compiles("
    #include <stdlib.h>
    #include <iconv.h>
    #if defined(__STDC__)
      extern size_t iconv(iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
    #else
      extern size_t iconv();
    #endif
    int main(void) { return 0; }
  " CHECK_ICONV_WITHOUT_CONST)
  if(NOT CHECK_ICONV_WITHOUT_CONST)
    set(ICONV_CONST "const")
  else()
    set(ICONV_CONST "")
  endif()
endif()
if(HAVE_LIBICONV)
  set(LIBS ${LIBS} iconv)
endif()

# Emulate AC_HEADER_STDC
check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
check_include_files_ext(dlfcn.h inttypes.h memory.h stdlib.h strings.h string.h sys/stat.h)

check_include_files_ext(sys/filio.h sys/sockio.h netdb.h utime.h sys/utime.h semaphore.h sys/un.h linux/rtc.h sys/syscall.h sys/mkdev.h sys/uio.h sys/param.h sys/sysctl.h libproc.h sys/prctl.h)
check_include_files_ext(sys/param.h sys/socket.h sys/ipc.h sys/utsname.h alloca.h ucontext.h pwd.h sys/select.h netinet/tcp.h netinet/in.h unistd.h sys/types.h link.h asm/sigcontext.h sys/inotify.h arpa/inet.h complex.h unwind.h)
check_c_snippet_compiles("" "stddef.h;sys/socket.h;linux/socket.h;linux/netlink.h" HAVE_LINUX_NETLINK_H)

if(HAVE_SYS_PARAM_H)
  check_c_snippet_compiles("" "sys/param.h;sys/user.h" HAVE_SYS_USER_H)
elseif()
  check_include_files_ext(sys/user.h)
endif()

check_include_files_ext(linux/serial.h)

check_include_files(zlib.h HAVE_ZLIB)
if(HAVE_ZLIB)
  check_c_snippet_compiles("
    #if defined(ZLIB_VERNUM) && (ZLIB_VERNUM >= 0x1230)
    #else
      #error No good zlib found
    #endif
  " "zlib.h" CHECK_SYS_ZLIB_VERSION)

  if(CHECK_SYS_ZLIB_VERSION)
    message(STATUS "Using system zlib")
    set(ZLIB_MSG "system zlib")
    set(HAVE_SYS_ZLIB YES)
  else()
    message(STATUS "Using embedded zlib")
    set(ZLIB_MSG "bundled zlib")
    set(HAVE_ZLIB NO)
  endif()
  unset(CHECK_SYS_ZLIB_VERSION)
endif()

# for mono/metadata/debug-symfile.c
check_include_files_ext(elf.h)

# for support
check_include_files_ext(poll.h)
check_include_files_ext(sys/poll.h)
check_include_files_ext(sys/wait.h)
check_include_files_ext(grp.h)
check_include_files_ext(syslog.h)

# for mono/dis
check_include_files_ext(wchar.h)
check_include_files_ext(ieeefp.h)
check_c_snippet_compiles("isinf (1.0);" "math.h" HAVE_ISINF)
# mingw
check_c_snippet_compiles("_finite(0.0);" "math.h" HAVE__FINITE)

# for Linux statfs support
check_include_files_ext(linux/magic.h)

# not 64 bit clean in cross-compile
check_types_size_ext("void *")

set(DISABLE_VISIBILITY_HIDDEN NO CACHE BOOL "disable usage of -fvisiblity=hidden")

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  set(WARN "-Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes  -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wno-cast-qual -Wwrite-strings -Wno-switch -Wno-switch-enum -Wno-unused-value -Wno-attributes")

  # We require C99 with some GNU extensions, e.g. `linux` macro
  set(CFLAGS "${CFLAGS} -std=gnu99")

  # The runtime code does not respect ANSI C strict aliasing rules
  set(CFLAGS "${CFLAGS} -fno-strict-aliasing")

  # We rely on signed overflow to behave
  set(CFLAGS "${CFLAGS} -fwrapv")

  set(CFLAGS "${CFLAGS} -DMONO_DLL_EXPORT")
  if(NOT DISABLE_VISIBILITY_HIDDEN)
    # Don't export any symbols by default
    set(SHARED_CFLAGS "-fvisibility=hidden")
    set(CFLAGS_FOR_EGLIB "${CFLAGS_FOR_EGLIB} -fvisibility=hidden")
    set(CXXFLAGS "${CXXFLAGS} -fvisibility=hidden")
  endif()

  # Check for the normal version, since gcc ignores unknown -Wno options
  set(CMAKE_REQUIRED_FLAGS "-Wunused-but-set-variable -Werror")
  check_c_snippet_compiles("" "" CHECK_FLAG_UNUSED_BUT_SET_VARIABLE)
  unset(CMAKE_REQUIRED_FLAGS)
  if(CHECK_FLAG_UNUSED_BUT_SET_VARIABLE)
    set(CFLAGS "${CFLAGS} -Wno-unused-but-set-variable")
  endif()

  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    # https://bugzilla.samba.org/show_bug.cgi?id=8118
    set(WARN "${WARN} -Qunused-arguments")
    set(WARN "${WARN} -Wno-unused-function -Wno-tautological-compare -Wno-parentheses-equality -Wno-self-assign -Wno-return-stack-address -Wno-constant-logical-operand")
  endif()

  # TODO: Sun Forte inline functions fix
#else
#  # The Sun Forte compiler complains about inline functions that access static variables
#  # so disable all inlining.
#  case "$host" in
#  *-*-solaris*)
#      CFLAGS="$CFLAGS -Dinline="
#      ;;
#  esac

endif()

set(CFLAGS "${CFLAGS} ${WARN}")
set(CPPFLAGS "${CPPFLAGS} ${WARN}")

# TODO: Alternate mcs source tree support
set(MCSDIR "mcs")
# Where's the 'mcs' source tree?
#if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/mcs")
#  set(MCSDIR "mcs")
#else()
#  set(MCSDIR "../mcs")
#endif()
#
#set(WITH_MCS_PATH "" CACHE PATH "Specify an alternate mcs source tree")
#if(NOT WITH_MCS_PATH STREQUAL "")
#  if(NOT IS_DIRECTORY ${WITH_MCS_PATH})
#    message(FATAL_ERROR "${WITH_MCS_PATH} is not a directory")
#  endif()
#  set(MCSDIR ${WITH_MCS_PATH})
#endif()

set(JUMPTABLES NO CACHE BOOL "enable/disable support for jumptables (ARM-only for now")

# TODO: Solaris ancient tar support

# TODO: Port mcs_topdir handling from original configure.ac
get_filename_component(MCS_TOPDIR "${MCSDIR}" REALPATH)

# TODO: gettext support

set(ENABLE_WERROR NO CACHE BOOL "Pass -Werror to the C compiler")
if(ENABLE_WERROR)
  set(WERROR_CFLAGS "-Werror")
endif()

set(GLIB_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/eglib/src" "${CMAKE_BINARY_DIR}/eglib/src")

# Enable support for fast thread-local storage
# Some systems have broken support, so we allow to disable it.
if(NOT MSVC)
  set(WITH_TLS "__thread" CACHE STRING "Select Thread Local Storage implementation. One of [__thread, pthread]. (defaults to __thread)")
endif()

# Enable support for using sigaltstack for SIGSEGV and stack overflow handling
# This does not work on some platforms (bug #55253)
set(WITH_SIGALTSTACK YES CACHE BOOL "Enable/disable support for sigaltstack (defaults to yes)")

set(STATIC_MONO YES CACHE BOOL "Link mono statically to libmono (faster) (defaults to yes)")
if(MSVC)
  set(SHARED_MONO NO)
else()
  set(SHARED_MONO YES CACHE BOOL "Build a shared libmono library (defaults to yes)")
endif()
# Same as --with-shared_mono=no
set(DISABLE_LIBRARIES NO CACHE BOOL "Disable the build of libmono")

if(DISABLE_LIBRARIES)
  set(SHARED_MONO NO)
endif()

# TODO: Enable shared libmono on nacl
#case $host in
#*nacl* ) with_shared_mono=yes;;
#esac

set(ENABLE_MCS_BUILD YES CACHE BOOL "Enable the build of the mcs directory")

if(NOT MSVC)
  set(ENABLE_XEN_OPT YES CACHE BOOL "Enable Xen-specific behaviour (defaults to yes)")
  if(ENABLE_XEN_OPT AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(MONO_XEN_OPT YES)
    set(CMAKE_REQUIRED_FLAGS "-mno-tls-direct-seg-refs")
    check_c_snippet_compiles("" "" CHECK_MNO_TLS_DIRECT_SEG_REFS)
    unset(CMAKE_REQUIRED_FLAGS)
    if(CHECK_MNO_TLS_DIRECT_SEG_REFS)
      set(CFLAGS "${CFLAGS} -mno-tls-direct-seg-refs")
      # Pass it to libgc as well
      set(LIBGC_COMPILE_OPTIONS ${LIBGC_COMPILE_OPTIONS} -mno-tls-direct-seg-refs)
    endif()
  endif()
endif()

set(ENABLE_SMALL_CONFIG NO CACHE BOOL "Enable tweaks to reduce requirements (and capabilities)")

if(ENABLE_SMALL_CONFIG)
  set(MONO_SMALL_CONFIG YES)
  set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DSMALL_CONFIG)
endif()

set(ENABLE_SYSTEM_AOT "default" CACHE STRING "Enable the Ahead-Of-Time compilation of system assemblies during the build (on by default on some platforms)")

set(AVAILABLE_FEATURES
  aot profiler decimal pinvoke debug appdomains verifier reflection_emit reflection_emit_save large_code logging com ssa
  generics shadowcopy portability attach full_messages verifier jit simd soft_debug perfcounters normalization
  assembly_remapping shared_perfcounters remoting security sgen_remset sgen_marksweep_par sgen_marksweep_fixed
  sgen_marksweep_fixed_par sgen_copying
)
foreach(FEATURE ${AVAILABLE_FEATURES})
  string(TOUPPER ${FEATURE} NAME)
  set(VAR "DISABLED_${NAME}")
  set("${VAR}" NO CACHE BOOL "Disable the ${FEATURE} feature")
  if(${VAR})
    list(APPEND DISABLED_FEATURES_LIST "${FEATURE}")
    message(STATUS "Disabled the ${FEATURE} feature")
  endif()
endforeach()
string(REPLACE ";" ", " DISABLED_FEATURES "${DISABLED_FEATURES_LIST}")

if(NOT DISABLED_FEATURES)
  set(DISABLED_FEATURES "none")
endif()

message(STATUS "DISABLED_FEATURES = ${DISABLED_FEATURES}")

if(DISABLE_AOT)
  set(ENABLE_SYSTEM_AOT NO)
endif()

set(ENABLE_EXECUTABLES YES CACHE BOOL "Enable the build of the runtime executables (defaults to yes)")
if(NOT ENABLE_EXECUTABLES)
  set(DISABLE_EXECUTABLES YES)
endif()

set(ENABLE_EXTENSION_MODULE NO CACHE BOOL "Enable the core-extensions")
if(ENABLE_EXTENSION_MODULE)
  set(HAS_EXTENSION_MODULE YES)
  message(STATUS "Enabling mono extension module")
endif()

set(ENABLE_GSHAREDVT NO CACHE BOOL "Enable generic valuetype sharing")

set(ENABLE_NATIVE_TYPES NO CACHE BOOL "Enable native types")
if(ENABLE_NATIVE_TYPES)
  set(MONO_NATIVE_TYPES YES)
endif()

message(STATUS "Checking for visibility __attribute__")
check_c_source_compiles("
  void __attribute__ ((visibility (\"hidden\"))) doit (void) {}
  int main () { doit (); return 0; }" HAVE_VISIBILITY_HIDDEN)

message(STATUS "Checking for deprecated __attribute__")
check_c_source_compiles("
  int doit (void) __attribute__ ((deprecated));
  int doit (void) { return 0; }
  int main () { doit (); return 0; }" HAVE_DEPRECATED)

#
# Boehm GC configuration
#
if(NOT MSVC)
  set(ENABLE_BOEHM YES CACHE BOOL "Enable the Boehm GC")
endif()
set(SUPPORT_BOEHM ${ENABLE_BOEHM})
if(PARALLEL_MARK)
  set(ENABLE_PARALLEL_MARK YES CACHE BOOL "Enables Boehm GC Parallel Marking")
  set(LIBGC_ENABLE_PARALLEL_MARK ENABLE_PARALLEL_MARK)
endif()

set(GC_MSG "")
set(LIBGC_INCLUDE_DIRECTORIES "")
set(LIBGC_LIBS "")
set(LIBGC_STATIC_LIBS "")
set(LIBGC_DIR "")
if(ENABLE_BOEHM)
  set(LIBGC_DIR "libgc")
  set(LIBGC_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/libgc/include")
  set(BOEHM_DEFINES -DHAVE_BOEHM_GC)

  if(TARGET_WINDOWS)
    set(BOEHM_DEFINES ${BOEHM_DEFINES} -DGC_NOT_DLL)
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DGC_BUILD -DGC_NOT_DLL)
  endif()

  set(GC_MSG "Included Boehm GC with typed GC")
  if(ENABLE_PARALLEL_MARK)
    set(DEFAULT_GC_NAME "Included Boehm (with typed GC and Parallel Mark)")
    set(GC_MSG "${GC_MSG} and parallel mark")
  else()
    set(DEFAULT_GC_NAME "Included Boehm (with typed GC)")
  endif()
else()
  message(WARNING "Compiling mono without GC.")
  set(DEFAULT_GC_NAME "none")
  set(HAVE_NULL_GC YES)
  set(GC_MSG "none")
endif()

set(ENABLE_LARGE_HEAP NO CACHE BOOL "Enable support for GC heaps larger than 3GB (defaults to no)")
if(ENABLE_LARGE_HEAP)
  set(CPPFLAGS "${CPPFLAGS} -DLARGE_CONFIG")
endif()

#
# End of Boehm GC Configuration
#

# *************************************
# *** Checks for zero length arrays ***
# *************************************
message(STATUS "Checking whether ${CMAKE_C_COMPILER_ID} supports zero length arrays")
check_c_source_compiles("\
  struct s {
    int  length;
    char data [0];
  };
  struct t {
    struct s s;
    int  foo;
  };
  int main() { return 0; }
" CHECK_LENGTH_OF_ZERO_LENGTH_ARRAYS)
if(CHECK_LENGTH_OF_ZERO_LENGTH_ARRAYS)
  set(MONO_ZERO_LEN_ARRAY 0)
else()
  set(MONO_ZERO_LEN_ARRAY 1)
endif()

check_include_files_ext(nacl/nacl_dyncode.h)

# ***********************************
# *** Checks for signals
# ***********************************
check_include_files_ext(signal.h)
check_functions_exists_ext(sigaction kill  signal)

# signal() is declared inline in Android headers
# so we need to workaround it by overriding the check.
if(PLATFORM_ANDROID)
  set(HAVE_SIGNAL YES)
endif()

if(NOT HOST_WINDOWS)

  # hires monotonic clock support
  search_libraries_for_function(clock_gettime CLOCK_GETTIME_LIB rt)
  if(DEFINED CLOCK_GETTIME_LIB AND NOT CLOCK_GETTIME_LIB STREQUAL "")
    message(STATUS "clock_gettime() found in '${CLOCK_GETTIME_LIB}'")
    set(LIBS ${LIBS} ${CLOCK_GETTIME_LIB})
  endif()

  # dynamic loader support
  search_libraries_for_function(dlopen DLOPEN_LIB dl)
  if(NOT DEFINED DLOPEN_LIB)
    message(WARNING "No dynamic loading support available")
  else()
    set(HAVE_DL_LOADER YES)
    if(NOT DLOPEN_LIB STREQUAL "")
      set(LIBS ${LIBS} ${DLOPEN_LIB})
    endif()
    set(CMAKE_REQUIRED_LIBRARIES ${DLOPEN_LIB})
    check_functions_exists_ext(dlerror)
    unset(CMAKE_REQUIRED_LIBRARIES)
  endif()

  # ******************************************************************
  # *** Checks for the IKVM JNI interface library                  ***
  # ******************************************************************
  set(ENABLE_IKVM_NATIVE ${IKVM_NATIVE} BOOL "Build the IKVM JNI interface library (defaults to ${IKVM_NATIVE})")
  if(ENABLE_IKVM_NATIVE)
    set(IKVM_NATIVE_DIR "ikvm-native")
    set(JDK_HEADERS_FOUND "IKVM Native")
  endif()

  check_include_files_ext(execinfo.h)

  check_include_files_ext(sys/auxv.h sys/resource.h)

  check_functions_exists_ext(getgrgid_r)
  check_functions_exists_ext(getgrnam_r)
  check_functions_exists_ext(getpwnam_r)
  check_functions_exists_ext(getpwuid_r)
  check_functions_exists_ext(getresuid)
  check_functions_exists_ext(setresuid)
  check_functions_exists_ext(kqueue)
  check_functions_exists_ext(backtrace_symbols)
  check_functions_exists_ext(mkstemp)
  check_functions_exists_ext(mmap)
  check_functions_exists_ext(madvise)
  check_functions_exists_ext(getrusage)
  check_functions_exists_ext(getpriority)
  check_functions_exists_ext(setpriority)
  check_functions_exists_ext(dl_iterate_phdr)
  check_functions_exists_ext(dladdr)
  check_functions_exists_ext(sysconf)
  check_functions_exists_ext(getrlimit)
  check_functions_exists_ext(prctl)
  check_functions_exists_ext(sched_getaffinity)
  check_functions_exists_ext(sched_setaffinity)
  check_functions_exists_ext(sched_getcpu)

  # ****************************************************************
  # *** Check for sched_setaffinity from glibc versions before   ***
  # *** 2.3.4. The older versions of the function only take 2    ***
  # *** parameters, not 3.                                       ***
  # ***                                                          ***
  # *** Because the interface change was not made in a minor     ***
  # *** version rev, the __GLIBC__ and __GLIBC_MINOR__ macros    ***
  # *** won't always indicate the interface sched_affinity has.  ***
  # ****************************************************************
  message(STATUS "Checking for sched_setaffinity from glibc < 2.3.4")
  check_c_snippet_compiles("
    int mask = 1;
    sched_setaffinity(0, &mask);" "sched.h" GLIBC_BEFORE_2_3_4_SCHED_SETAFFINITY)

  # ******************************************************************
  # *** Check for large file support                               ***
  # ******************************************************************
  if(NOT DEFINED HAVE_LARGE_FILE_SUPPORT)
    set(CMAKE_EXTRA_INCLUDE_FILES sys/types.h limits.h)
    message(STATUS "Checking if off_t is 64 bits wide")
    check_type_size("off_t" SIZEOF_OFF_T_DEFAULT)
    set(HAVE_LARGE_FILE_SUPPORT YES)
    if(NOT "${SIZEOF_OFF_T_DEFAULT}" STREQUAL "8")
      # Maybe we need to set -D_FILE_OFFSET_BITS=64 ?
      message(STATUS "Checking if _FILE_OFFSET_BITS=64 gives 64 bit off_t")
      set(CMAKE_REQUIRED_DEFINITIONS "-D_FILE_OFFSET_BITS=64")
      check_type_size("off_t" SIZEOF_OFF_T_WITH_FILE_OFFSET_BITS)
      if("${SIZEOF_OFF_T_WITH_FILE_OFFSET_BITS}" STREQUAL "8")
        set(CPPFLAGS "${CPPFLAGS} -D_FILE_OFFSET_BITS=64")
      else()
        set(HAVE_LARGE_FILE_SUPPORT NO)
        message(WARNINIG "No 64 bit file size support available")
      endif()
      unset(CMAKE_REQUIRED_DEFINITIONS)
    endif()
    unset(CMAKE_EXTRA_INCLUDE_FILES)
  endif()

  # *****************************
  # *** Checks for libsocket  ***
  # *****************************
  check_library_exists(socket socket "" HAVE_LIBSOCKET)
  if(HAVE_LIBSOCKET)
    set(LIBS ${LIBS} "socket")
  endif()

  if(HOST_FREEBSD)
    # *****************************
    # *** Checks for libinotify ***
    # *****************************
    check_library_exists(inotify inotify_init "" HAVE_LIBINOTIFY)
    if(HAVE_LIBINOTIFY)
      set(LIBS ${LIBS} "inotify")
    endif()
  endif()

  # *******************************
  # *** Checks for MSG_NOSIGNAL ***
  # *******************************
  message(STATUS "Checking for MSG_NOSIGNAL")
  check_c_snippet_compiles("int f = MSG_NOSIGNAL;" "sys/socket.h" HAVE_MSG_NOSIGNAL)

  # *****************************
  # *** Checks for IPPROTO_IP ***
  # *****************************
  message(STATUS "Checking for IPPROTO_IP")
  check_c_snippet_compiles("int level = IPPROTO_IP;" "netinet/in.h" HAVE_IPPROTO_IP)

  # *******************************
  # *** Checks for IPPROTO_IPV6 ***
  # *******************************
  message(STATUS "Checking for IPPROTO_IPV6")
  check_c_snippet_compiles("int level = IPPROTO_IPV6;" "netinet/in.h" HAVE_IPPROTO_IPV6)

  # ******************************
  # *** Checks for IPPROTO_TCP ***
  # ******************************
  message(STATUS "Checking for IPPROTO_TCP")
  check_c_snippet_compiles("int level = IPPROTO_TCP;" "netinet/in.h" HAVE_IPPROTO_TCP)

  # *****************************
  # *** Checks for SOL_IP     ***
  # *****************************
  message(STATUS "Checking for SOL_IP")
  check_c_snippet_compiles("int level = SOL_IP;" "netdb.h" HAVE_SOL_IP)

  # *****************************
  # *** Checks for SOL_IPV6     ***
  # *****************************
  message(STATUS "Checking for SOL_IPV6")
  check_c_snippet_compiles("int level = SOL_IPV6;" "netdb.h" HAVE_SOL_IPV6)

  # *****************************
  # *** Checks for SOL_TCP    ***
  # *****************************
  message(STATUS "Checking for SOL_TCP")
  check_c_snippet_compiles("int level = SOL_TCP;" "netdb.h" HAVE_SOL_TCP)

  # *****************************
  # *** Checks for IP_PKTINFO ***
  # *****************************
  message(STATUS "Checking for IP_PKTINFO")
  check_c_snippet_compiles("int level = IP_PKTINFO;" "linux/in.h" HAVE_IP_PKTINFO)

  # *****************************
  # *** Checks for IPV6_PKTINFO ***
  # *****************************
  message(STATUS "Checking for IPV6_PKTINFO")
  check_c_snippet_compiles("int level = IPV6_PKTINFO;" "netdb.h" HAVE_IPV6_PKTINFO)

  # **********************************
  # *** Checks for IP_DONTFRAG     ***
  # **********************************
  message(STATUS "Checking for IP_DONTFRAG")
  check_c_snippet_compiles("int level = IP_DONTFRAG;" "netinet/in.h" HAVE_IP_DONTFRAG)

  # **********************************
  # *** Checks for IP_DONTFRAGMENT ***
  # **********************************
  message(STATUS "Checking for IP_DONTFRAGMENT")
  check_c_snippet_compiles("int level = IP_DONTFRAGMENT;" "Ws2ipdef.h" HAVE_IP_DONTFRAGMENT)

  # **********************************
  # *** Checks for IP_MTU_DISCOVER ***
  # **********************************
  message(STATUS "Checking for IP_MTU_DISCOVER")
  check_c_snippet_compiles("int level = IP_MTU_DISCOVER;" "linux/in.h" HAVE_IP_MTU_DISCOVER)

  # **********************************
  # *** Checks for  IP_PMTUDISC_DO ***
  # **********************************
  message(STATUS "Checking for IP_PMTUDISC_DO")
  check_c_snippet_compiles("int level = IP_PMTUDISC_DO;" "linux/in.h" HAVE_IP_PMTUDISC_DO)

  # *********************************
  # *** Check for struct ip_mreqn ***
  # *********************************
  message(STATUS "Checking for struct ip_mreqn")
  set(CMAKE_EXTRA_INCLUDE_FILES "netinet/in.h")
  check_types_size_ext("struct ip_mreqn")
  if(NOT HAVE_STRUCT_IP_MREQN)
    message(STATUS "Checking for struct ip_mreq")
    check_types_size_ext("struct ip_mreq")
  endif()
  unset(CMAKE_EXTRA_INCLUDE_FILES)

  # **********************************
  # *** Check for getaddrinfo ***
  # **********************************
  check_functions_exists_ext("getaddrinfo")

  # **********************************
  # *** Check for gethostbyname2_r ***
  # **********************************
  check_functions_exists_ext("gethostbyname2_r")

  # **********************************
  # *** Check for gethostbyname2 ***
  # **********************************
  check_functions_exists_ext("gethostbyname2")

  # **********************************
  # *** Check for gethostbyname ***
  # **********************************
  check_functions_exists_ext("gethostbyname")

  # **********************************
  # *** Check for getprotobyname ***
  # **********************************
  check_functions_exists_ext("getprotobyname")

  # **********************************
  # *** Check for getnameinfo ***
  # **********************************
  check_functions_exists_ext("getnameinfo")

  # **********************************
  # *** Check for inet_ntop ***
  # **********************************
  check_functions_exists_ext("inet_ntop")

  # *****************************
  # *** Checks for libnsl     ***
  # *****************************
  check_functions_exists_ext("gethostbyaddr")
  if(NOT HAVE_GETHOSTBYADDR)
    check_library_exists(nsl gethostbyaddr "" HAVE_LIBNSL)
    if(HAVE_LIBNSL)
      set(LIBS ${LIBS} "nsl")
    endif()
  endif()

  check_functions_exists_ext("inet_pton" "inet_aton")

  # *****************************
  # *** Checks for libxnet    ***
  # *****************************
  # TODO: Check for libxnet on Solaris

  # *****************************
  # *** Checks for libpthread ***
  # *****************************
  set(CMAKE_THREAD_PREFER_PTHREAD YES)
  include(FindThreads)
  if(CMAKE_USE_PTHREADS_INIT)
    # pthreads found!
    if(CMAKE_THREAD_LIBS_INIT MATCHES "^-l.*")
      string(REGEX REPLACE "^-l" "" PTHREAD_LIB "${CMAKE_THREAD_LIBS_INIT}")
      set(LIBS ${LIBS} ${PTHREAD_LIB})
    elseif(NOT CMAKE_THREAD_LIBS_INIT STREQUAL "")
      set(LDFLAGS "${LDFLAGS} ${CMAKE_THREAD_LIBS_INIT}")
    endif()
  endif()

  check_include_files_ext(pthread.h pthread_np.h)
  check_functions_exists_ext(pthread_mutex_timedlock pthread_getattr_np pthread_attr_get_np pthread_setname_np pthread_kill)
  message(STATUS "Checking for PTHREAD_MUTEX_RECURSIVE")
  check_c_snippet_compiles("pthread_mutexattr_settype(NULL, PTHREAD_MUTEX_RECURSIVE);" "pthread.h" HAVE_PTHREAD_MUTEX_RECURSIVE)
  if(NOT HAVE_PTHREAD_MUTEX_RECURSIVE)
    message(FATAL_ERROR "Posix system lacks support for recursive mutexes")
  endif()
  check_functions_exists_ext(pthread_attr_setstacksize pthread_attr_getstack pthread_attr_getstacksize pthread_get_stacksize_np pthread_get_stackaddr_np)

  # **********************************
  # *** Check for mincore ***
  # **********************************
  check_functions_exists_ext("mincore")

  # ***********************************
  # *** Checks for working __thread ***
  # ***********************************
  if(WITH_TLS STREQUAL "__thread")
    if(CMAKE_C_COMPILER_ID MATCHES "AppleClang")
      # __thread does not currently work with clang on Mac OS X
      set(WITH_TLS "pthread")
    else()
      message(STATUS "Checking for working __thread")
      check_c_source_compiles("
        __thread int tls;
        int main(void) { return 0; }
      " HAVE___THREAD)
      if(NOT HAVE___THREAD)
        set(WITH_TLS "pthread")
      endif()
    endif()
  endif()

  # **************************************
  # *** Checks for working sigaltstack ***
  # **************************************
  if(NOT WITH_SIGALTSTACK)
    message(STATUS "Checking for working sigaltstack - disabled")
  elseif(CMAKE_CROSSCOMPILING)
    message(STATUS "Checking for working sigaltstack - cross compiling, assuming yes")
  else()
    message(STATUS "Checking for working sigaltstack")
    check_c_source_runs("
            #include <stdio.h>
            #include <stdlib.h>
            #include <unistd.h>
            #include <signal.h>
            #include <pthread.h>
            #include <sys/wait.h>
            #if defined(__FreeBSD__) || defined(__NetBSD__)
            #define SA_STACK SA_ONSTACK
            #endif
            static void
            sigsegv_signal_handler (int _dummy, siginfo_t *info, void *context)
            {
                exit (0);
            }

            volatile char*__ptr = NULL;
            static void *
            loop (void *ignored)
            {
                *__ptr = 0;
                return NULL;
            }

            static void
            child ()
            {
                struct sigaction sa;
            #ifdef __APPLE__
                stack_t sas;
            #else
                struct sigaltstack sas;
            #endif
                pthread_t id;
                pthread_attr_t attr;

                sa.sa_sigaction = sigsegv_signal_handler;
                sigemptyset (&sa.sa_mask);
                sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
                if (sigaction (SIGSEGV, &sa, NULL) == -1) {
                    perror (\"sigaction\");
                    return;
                }

                /* x86 darwin deliver segfaults using SIGBUS */
                if (sigaction (SIGBUS, &sa, NULL) == -1) {
                    perror (\"sigaction\");
                    return;
                }
                sas.ss_sp = malloc (SIGSTKSZ);
                sas.ss_size = SIGSTKSZ;
                sas.ss_flags = 0;
                if (sigaltstack (&sas, NULL) == -1) {
                    perror (\"sigaltstack\");
                    return;
                }

                pthread_attr_init (&attr);
                if (pthread_create(&id, &attr, loop, &attr) != 0) {
                    perror (\"pthread_create\");
                    return;
                }

                sleep (100);
            }

            int
            main ()
            {
                pid_t son;
                int status;
                int i;

                son = fork ();
                if (son == -1) {
                    return 1;
                }

                if (son == 0) {
                    child ();
                    return 0;
                }

                for (i = 0; i < 300; ++i) {
                    waitpid (son, &status, WNOHANG);
                    if (WIFEXITED (status) && WEXITSTATUS (status) == 0)
                        return 0;
                    usleep (10000);
                }

                kill (son, SIGKILL);
                return 1;
            }" HAVE_WORKING_SIGALTSTACK)
    if(HAVE_WORKING_SIGALTSTACK)
      message(STATUS "Found working sigaltstack")
    else()
      set(WITH_SIGALTSTACK NO)
    endif()
  endif()

  # ********************************
  # *** Checks for semaphore lib ***
  # ********************************
  # 'Real Time' functions on Solaris
  # posix4 on Solaris 2.6
  # pthread (first!) on Linux
  search_libraries_for_function(sem_init SEM_INIT_LIB pthread rt posix4)
  search_libraries_for_function(shm_open SEM_OPEN_LIB pthread rt posix4)
  set(LIBS ${LIBS} ${SEM_INIT_LIB} ${SEM_OPEN_LIB})
  check_functions_exists_ext(shm_open)

  # ********************************
  # *** Checks for timezone stuff **
  # ********************************
  message(STATUS "Checking for tm_gmtoff in struct tm")
  check_c_snippet_compiles("struct tm tm; tm.tm_gmtoff = 1;" "time.h" HAVE_TM_GMTOFF)
  if(NOT HAVE_TM_GMTOFF)
    message(STATUS "Checking for timezone variable")
    check_c_snippet_compiles("timezone = 1;" "time.h" HAVE_TIMEZONE)
    if(NOT HAVE_TIMEZONE)
      message(FATAL_ERROR "Unable to find a way to determine timezone")
    endif()
  endif()

  # *********************************
  # *** Checks for math functions ***
  # *********************************
  search_libraries_for_function(sqrtf SQRTF_LIB m)
  set(LIBS ${LIBS} ${SQRTF_LIB})
  check_c_snippet_compiles("finite(0.0);" "math.h" HAVE_FINITE)
  check_c_snippet_compiles("isfinite(0.0);" "math.h" HAVE_ISFINITE)

  # ****************************************************************
  # *** Checks for working poll() (macosx defines it but doesn't ***
  # *** have it in the library (duh))                            ***
  # ****************************************************************
  check_functions_exists_ext(poll)

  # *************************
  # *** Check for signbit ***
  # *************************
  check_c_snippet_compiles("signbit(1.0);" "math.h" HAVE_SIGNBIT)

  # **********************************
  # *** epoll			   ***
  # **********************************
  if(NOT HAVE_NACL_NACL_DYNCODE_H)
    check_include_files_ext(sys/epoll.h)
    check_functions_exists_ext(epoll_ctl)
    if(HAVE_SYS_EPOLL_H AND HAVE_EPOLL_CTL)
      set(HAVE_EPOLL YES)
    endif()
  endif()

  check_include_files_ext(sys/event.h)
  check_functions_exists_ext(kqueue)

  # **************************************
  # * Darwin has a race that prevents us from using reliably:
  # * http://lists.apple.com/archives/darwin-dev/2011/Jun/msg00016.html
  # * Since kqueue is mostly used for scaling large web servers,
  # * and very few folks run Mono on large web servers on OSX, falling
  # * back
  # **************************************
  if(HAVE_KQUEUE AND HAVE_SYS_EVENT_H)
    if(NOT HOST_DARWIN)
      set(USE_KQUEUE_FOR_THREADPOOL YES)
    endif()
  endif()

  # ******************************
  # *** Checks for SIOCGIFCONF ***
  # ******************************
  check_include_files_ext(sys/ioctl.h)
  check_include_files_ext(net/if.h)
  check_c_snippet_compiles("
      struct ifconf ifc;
      struct ifreq *ifr;
      void *x;
      ifc.ifc_len = 0;
      ifc.ifc_buf = NULL;
      x = (void *) &ifr->ifr_addr;" "sys/ioctl.h;net/if.h" HAVE_SIOCGIFCONF)

  # **********************************
  # ***     Checks for sin_len     ***
  # **********************************
  message(STATUS "Checking for sockaddr_in.sin_len")
  check_c_snippet_compiles("
      struct sockaddr_in saddr;
      saddr.sin_len = sizeof (saddr);" "netinet/in.h" HAVE_SOCKADDR_IN_SIN_LEN)

  # **********************************
  # ***    Checks for sin6_len     ***
  # **********************************
  message(STATUS "Checking for sockaddr_in6.sin6_len")
  check_c_snippet_compiles("
      struct sockaddr_in6 saddr6;
      saddr6.sin6_len = sizeof (saddr6);" "netinet/in.h" HAVE_SOCKADDR_IN6_SIN_LEN)

  # **********************************
  # *** Check for getifaddrs       ***
  # **********************************
  message(STATUS "Checking for getifaddrs")
  check_c_snippet_compiles("getifaddrs(NULL);" "stdio.h;sys/types.h;sys/socket.h;ifaddrs.h" HAVE_GETIFADDRS)

  # **********************************
  # *** Check for if_nametoindex   ***
  # **********************************
  message(STATUS "Checking for if_nametoindex")
  check_c_snippet_compiles("if_nametoindex(NULL);" "stdio.h;sys/types.h;sys/socket.h" HAVE_IF_NAMETOINDEX)

  # **********************************
  # *** Checks for MonoPosixHelper ***
  # **********************************
  check_include_files_ext(checklist.h pathconf.h fstab.h attr/xattr.h sys/extattr.h sys/sendfile.h sys/statvfs.h
    sys/statfs.h sys/vfstab.h sys/xattr.h sys/mman.h sys/param.h sys/time.h dirent.h)
  if(HAVE_SYS_PARAM_H)
    check_c_snippet_compiles("" "sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
  elseif()
    check_include_files_ext(sys/mount.h)
  endif()
  check_functions_exists_ext(confstr seekdir telldir getdomainname setdomainname endgrent getgrent fgetgrent setgrent
    setgroups endpwent getpwent fgetpwent setpwent getfsstat lutimes futimes mremap remap_file_pages posix_fadvise
    posix_fallocate posix_madvise vsnprintf sendfile gethostid sethostid sethostname statfs fstatfs statvfs fstatvfs
    stime strerror_r ttyname_r psignal getlogin_r lockf swab setusershell endusershell futimens utimensat fstatat
    mknodat readlinkat readv writev preadv pwritev setpgid system fork execv execve accept4)
  check_types_size_ext(size_t)
  check_type_size_ext("blksize_t" "sys/types.h;sys/stat.h;unistd.h")
  check_type_size_ext("blkcnt_t" "sys/types.h;sys/stat.h;unistd.h")
  check_type_size_ext("suseconds_t" "sys/time.h")
  check_type_size_ext("struct cmsghdr" "sys/socket.h")
  check_type_size_ext("struct flock" "unistd.h;fcntl.h")
  check_type_size_ext("struct iovec" "sys/uio.h")
  check_type_size_ext("struct linger" "sys/socket.h")
  check_type_size_ext("struct pollfd" "sys/poll.h")
  check_type_size_ext("struct sockaddr" "sys/socket.h")
  check_type_size_ext("struct sockaddr_storage" "sys/socket.h")
  check_type_size_ext("struct sockaddr_in" "netinet/in.h")
  check_type_size_ext("struct sockaddr_in6" "netinet/in.h")
  check_type_size_ext("struct sockaddr_un" "sys/un.h")
  check_type_size_ext("struct stat" "sys/types.h;sys/stat.h;unistd.h")
  check_type_size_ext("struct timespec" "time.h")
  check_type_size_ext("struct timeval" "sys/time.h;sys/types.h;utime.h")
  check_type_size_ext("struct timezone" "sys/time.h")
  check_type_size_ext("struct utimbuf" "sys/types.h;utime.h")
  check_struct_has_member_ext("struct dirent" "d_off;d_reclen;d_type" "sys/types.h;dirent.h")
  check_struct_has_member_ext("struct passwd" "pw_gecos" "sys/types.h;pwd.h")
  check_struct_has_member_ext("struct statfs" "f_flags" "sys/types.h;sys/vfs.h")
  check_struct_has_member_ext("struct stat" "st_atim;st_mtim;st_ctim" c"sys/types.h;sys/stat.h;unistd.h")

  # Favour xattr through glibc, but use libattr if we have to
  check_functions_exists_ext(lsetxattr)
  if(NOT HAVE_LSETXATTR)
    unset(HAVE_LSETXATTR)
    check_library_exists(attr lsetxattr "" HAVE_LSETXATTR)
    if(HAVE_LSETXATTR)
      set(XATTR_LIB "attr")
    endif()
  endif()

  # kinfo_proc.kp_proc works on darwin but fails on other simil-bsds
  check_struct_has_member_ext("struct kinfo_proc" "kp_proc" "sys/types.h;sys/param.h;sys/sysctl.h;sys/proc.h")

  # ******************************************
  # *** Checks for OSX and iOS compilation ***
  # ******************************************
  check_include_files_ext(CommonCrypto/CommonDigest.h)

  # *********************************
  # *** Check for Console 2.0 I/O ***
  # *********************************
  check_include_files_ext(curses.h termios.h)
  if(HAVE_CURSES_H)
    check_include_files_ext(term.h)
  endif()

else()
  # *********************************
  # *** Checks for Windows compilation ***
  # *********************************
  check_include_files_ext(winternl.h)

  set(JDK_HEADERS_FOUND NO)

  set(HAVE_LIBWS2_32 YES)
  set(LIBS ${LIBS} ws2_32)
  set(HAVE_LIBPSAPI YES)
  set(LIBS ${LIBS} psapi)
  set(HAVE_LIBOLE32 YES)
  set(LIBS ${LIBS} ole32)
  set(HAVE_LIBWINMM YES)
  set(LIBS ${LIBS} winmm)
  set(HAVE_LIBOLEAUT32 YES)
  set(LIBS ${LIBS} oleaut32)
  set(HAVE_LIBADVAPI32 YES)
  set(LIBS ${LIBS} advapi32)
  set(HAVE_LIBVERSION YES)
  set(LIBS ${LIBS} version)
  set(HAVE_LIBMSWSOCK YES)
  set(LIBS ${LIBS} mswsock)

  # *********************************
  # *** Check for struct ip_mreqn ***
  # *********************************
  message(STATUS "Checking for struct ip_mreqn")
  set(CMAKE_EXTRA_INCLUDE_FILES "ws2tcpip.h")
  check_types_size_ext("struct ip_mreqn")
  if(NOT HAVE_STRUCT_IP_MREQN)
    message(STATUS "Checking for struct ip_mreq")
    check_types_size_ext("struct ip_mreq")
  endif()
  unset(CMAKE_EXTRA_INCLUDE_FILES)

  set(HAVE_GETADDRINFO YES)
  set(HAVE_GETHOSTBYNAME YES)
  set(HAVE_GETPROTOBYNAME YES)
  set(HAVE_GETNAMEINFO YES)
  set(HAVE_GETADDRINFO YES)
  set(HAVE_INET_NTOP YES)
  set(HAVE_INET_PTON YES)

  check_symbol_exists(InterlockedExchange64 windows.h HAVE_DECL_INTERLOCKEDEXCHANGE64)
  check_symbol_exists(InterlockedCompareExchange64 windows.h HAVE_DECL_INTERLOCKEDCOMPAREEXCHANGE64)
  check_symbol_exists(InterlockedDecrement64 windows.h HAVE_DECL_INTERLOCKEDDECREMENT64)
  check_symbol_exists(InterlockedIncrement64 windows.h HAVE_DECL_INTERLOCKEDINCREMENT64)
  check_symbol_exists(InterlockedAdd windows.h HAVE_DECL_INTERLOCKEDADD)
  check_symbol_exists(InterlockedAdd64 windows.h HAVE_DECL_INTERLOCKEDADD64)
  check_c_snippet_compiles("unsigned long w = __readfsdword(0);" "windows.h" HAVE_DECL___READFSDWORD)
endif()

# socklen_t check
check_type_size_ext("socklen_t" "sys/types.h;sys/socket.h")

message(STATUS "Checking for array element initializer support")
check_c_snippet_compiles("const int array[] = {[1] = 2,};" "" HAVE_ARRAY_ELEM_INIT)

message(STATUS "Checking for trunc in math.h")
# Simply calling trunc (0.0) is no good since gcc will optimize the call away
check_c_snippet_compiles("static void *p = &trunc;" "math.h" HAVE_TRUNC)

if(NOT HAVE_TRUNC)
  check_library_exists(sunmath aintl "" HAVE_AINTL)
  if(HAVE_AINTL)
    set(LIBS ${LIBS} "sunmath")
  endif()
endif()

check_functions_exists_ext(execvp)

# ****************************
# *** Look for /dev/random ***
# ****************************

if(HOST_WINDOWS)
  # Win32 does not have /dev/random, they have their own method...
  set(HAVE_CRYPT_RNG YES)
else()

  message(STATUS "Checking if usage of random device is requested")
  set(DISABLE_DEV_RANDOM NO CACHE BOOL "disable the use of the random device (enabled by default)")

  if(HOST_OPENBSD)
    set(NAME_DEV_RANDOM "/dev/srandom")
  else()
    # Everywhere else, it's /dev/random
    set(NAME_DEV_RANDOM "/dev/random")
  endif()

  # Now check if the device actually exists
  # TODO: This doesn't take crosscompiling into account
  if(NOT DISABLE_DEV_RANDOM)
    if(EXISTS ${NAME_DEV_RANDOM})
      set(HAVE_CRYPT_RNG YES)
    endif()
  endif()

endif()

if(NOT HAVE_CRYPT_RNG)
  message(WARNING
"***
*** A system-provided entropy source was not found on this system.
*** Because of this, the System.Security.Cryptography random number generator
*** will throw a NotImplemented exception.
***
*** If you are seeing this message, and you know your system DOES have an
*** entropy collection in place, please contact <crichton@gimp.org> and
*** provide information about the system and how to access the random device.
***
*** Otherwise you can install either egd or prngd and set the environment
*** variable MONO_EGD_SOCKET to point to the daemon's socket to use that.
***"
  )
endif()

set(DISABLE_BCL_OPT NO CACHE BOOL "BCL is compiled with no optimizations (allows accurate BCL debugging)")

if(HOST_LINUX)
  set(ENABLE_PERF_EVENTS NO CACHE BOOL "Enable using `perf` for profiling on Linux")
endif()

set(ENABLE_BIG_ARRAYS NO CACHE BOOL "Enable the allocation and indexing of arrays greater than Int32.MaxValue")
if(ENABLE_BIG_ARRAYS)
  if(NOT "${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    message(FATAL_ERROR "The allocation and indexing of arrays greater than Int32.MaxValue is not supported on this platform.")
  endif()
  set(MONO_BIG_ARRAYS YES)
endif()

# **************
# *** DTRACE ***
# **************
if(HAS_DTRACE)
  set(ENABLE_DTRACE YES CACHE BOOL "Enable DTrace probes")
  if(ENABLE_DTRACE)
    find_program(DTRACE dtrace "/usr/sbin")
    if(DTRACE)
      execute_process(COMMAND ${DTRACE} -h -s ${CMAKE_SOURCE_DIR}/data/mono.d RESULT_VARIABLE DTRACE_RESULT ERROR_QUIET OUTPUT_QUIET)
      if(NOT DTRACE_RESULT EQUAL 0)
        message(STATUS "dtrace doesn't support -h option, dtrace support disabled.")
        set(ENABLE_DTRACE NO)
      endif()
    else()
      message(STATUS "dtrace utility not found, dtrace support disabled.")
      set(ENABLE_DTRACE NO)
    endif()
  endif()
endif()
if(ENABLE_DTRACE)
  set(DTRACEFLAGS "")
  if(HOST_64BIT)
    if(HOST_DARWIN AND HOST_POWERPC)
      set(DTRACEFLAGS "-arch ppc64")
    elseif(HOST_DARWIN)
      set(DTRACEFLAGS "-arch x86_64")
    else()
      set(DTRACEFLAGS "-64")
    endif()
  else()
    if(HOST_DARWIN AND HOST_POWERPC)
      set(DTRACEFLAGS "-arch ppc")
    elseif(HOST_DARWIN)
      set(DTRACEFLAGS "-arch i386")
    else()
      set(DTRACEFLAGS "-32")
    endif()
  endif()
  if(HOST_SOLARIS)
    set(DTRACE_G_REQUIRED YES)
  endif()
  check_include_files_ext(sys/sdt.h)
endif()

# **************
# ***  NaCl  ***
# **************
set(ENABLE_NACL_CODEGEN NO CACHE BOOL "Enable Native Client code generation")
set(ENABLE_NACL_GC NO CACHE BOOL "Enable Native Client garbage collection")

set(NACL_CODEGEN ${ENABLE_NACL_CODEGEN})

#
# Hack to use system mono for operations in build/install not allowed in NaCl.
#
set(NACL_SELF_HOST "")
if(HAVE_NACL_NACL_DYNCODE_H)
  set(NACL_SELF_HOST "nacl_self_host")
endif()
if(ENABLE_NACL_CODEGEN)
  set(MONO_NACL_ALIGN_MASK_OFF 1)
  set(TARGET_NACL YES)
  set(__native_client_codegen__ YES)
else()
  set(MONO_NACL_ALIGN_MASK_OFF 0)
  set(__default_codegen__ YES)
endif()
if(ENABLE_NACL_GC)
  if("${TARGET}" STREQUAL "AMD64" OR "${TARGET}" STREQUAL "X86")
    set(INSTRUMENT_CFLAG "-finstrument-for-thread-suspension")
  else()
    # Not yet implemented
    set(INSTRUMENT_CFLAG "")
  endif()
  set(CPPFLAGS "${CPPFLAGS} ${INSTRUMENT_CFLAG} -D__native_client_gc__")
endif()

# TODO: NaCl support


# **************************
# *** AOT cross offsets  ***
# **************************
set(MONO_OFFSETS_FILE "" CACHE FILEPATH "Explicit AOT cross offsets file")

# **************
# ***  LLVM  ***
# **************

set(ENABLE_LLVM NO CACHE BOOL "Enable the LLVM back-end")
set(ENABLE_LOADEDLLVM NO CACHE BOOL "Load the LLVM back-end dynamically")
set(ENABLE_LLVM_VERSION_CHECK NO CACHE BOOL "Check that the LLVM matches the version expected by mono")
set(ENABLE_LLVM_RUNTIME NO CACHE BOOL "Enable runtime support for llvmonly code")
set(WITH_LLVM "" CACHE PATH "Path to the LLVM dist")

if(ENABLE_LLVM)
  if(NOT "${WITH_LLVM}" STREQUAL "")
    set(LLVM_CONFIG "${WITH_LLVM}/bin/llvm-config")
    if(HOST_WINDOWS)
      set(LLVM_CONFIG "${LLVM_CONFIG}.exe")
    endif()
    if(NOT EXISTS "${LLVM_CONFIG}")
      message(FATAL_ERROR "LLVM executable ${LLVM_CONFIG} not found.")
    endif()
  else()
    find_program(LLVM_CONFIG llvm-config)
    if(NOT LLVM_CONFIG)
      message(FATAL_ERROR "llvm-config not found.")
    endif()
  endif()

  set(LLVM_CODEGEN "x86codegen")
  if(TARGET_ARM)
    set(LLVM_CODEGEN "armcodegen")
  endif()

  if(NOT HOST_WINDOWS)
    # Should be something like '2.6' or '2.7svn'
    execute_process(COMMAND ${LLVM_CONFIG} --version OUTPUT_VARIABLE LLVM_VERSION ERROR_QUIET)
    string(STRIP "${LLVM_VERSION}" LLVM_VERSION)
    execute_process(COMMAND ${LLVM_CONFIG} --mono-api-version OUTPUT_VARIABLE LLVM_API_VERSION ERROR_QUIET)
    string(STRIP "${LLVM_API_VERSION}" LLVM_API_VERSION)
    message(STATUS "Checking LLVM version ${LLVM_VERSION} ${LLVM_API_VERSION}")
    if(LLVM_VERSION MATCHES "mono")
      if(ENABLE_LLVM_VERSION_CHECK)
        set(EXPECTED_LLVM_VERSION "3.4svn-mono-mono/e656cac")
        if(NOT "${LLVM_VERSION}" STREQUAL "${EXPECTED_LLVM_VERSION}")
          message(FATAL_ERROR "Expected llvm version ${EXPECTED_LLVM_VERSION}, but llvm-config --version returned ${LLVM_VERSION}")
        endif()
      endif()
    else()
      message(FATAL_ERROR "Compiling with stock LLVM is not supported, please use the Mono LLVM repo at https://github.com/mono/llvm, with the GIT branch which matches this version of mono, i.e. 'mono-2-10' for Mono 2.10.")
	  endif()

    # The output of --cflags seems to include optimizations flags too
    if(LLVM_API_VERSION GREATER 100)
      # The --cflags argument includes all kinds of warnings -pendantic etc.
      set(LLVM_CFLAGS "-I${WITH_LLVM}/include -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
      set(LLVM_CXXFLAGS "-I${WITH_LLVM}/include -std=c++11 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
    else()
      execute_process(COMMAND ${LLVM_CONFIG} --cflags OUTPUT_VARIABLE LLVM_CFLAGS ERROR_QUIET)
      string(STRIP "${LLVM_CFLAGS}" LLVM_CFLAGS)
      string(REGEX REPLACE "-O2|-O0|-fomit-frame-pointer|-fPIC" "" LLVM_CFLAGS "${LLVM_CFLAGS}")
      execute_process(COMMAND ${LLVM_CONFIG} --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS ERROR_QUIET)
      string(STRIP "${LLVM_CXXFLAGS}" LLVM_CXXFLAGS)
      string(REGEX REPLACE "-O2|-O0|-fomit-frame-pointer|-fPIC" "" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
    endif()
    # LLVM is compiled with -fno-rtti, so we need this too, since our classes inherit
    # from LLVM classes.
    set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} -fno-rtti -fexceptions")
    execute_process(COMMAND ${LLVM_CONFIG} --ldflags OUTPUT_VARIABLE LLVM_LDFLAGS ERROR_QUIET)
    string(STRIP "${LLVM_LDFLAGS}" LLVM_LDFLAGS)
    # TODO: This might include empty lines
    execute_process(COMMAND ${LLVM_CONFIG} --system-libs OUTPUT_VARIABLE LLVM_SYSTEM_LIBS ERROR_QUIET)
    string(STRIP "${LLVM_SYSTEM_LIBS}" LLVM_SYSTEM_LIBS)
    set(LLVM_JIT_SUPPORTED YES)
    set(LLVM_JIT_LIBS "jit mcjit ${LLVM_CODEGEN}")
    if(LLVM_API_VERSION GREATER 100)
      # Based on llvm 3.9, only aot is currently supported
      set(LLVM_JIT_LIBS "orcjit ${LLVM_CODEGEN}")
    elseif(MONO_CROSS_COMPILE)
      # No need for jit libs
      set(LLVM_JIT_SUPPORTED NO)
      set(LLVM_JIT_LIBS "")
    endif()
    execute_process(COMMAND ${LLVM_CONFIG} --libs analysis core bitwriter ${LLVM_JIT_LIBS} OUTPUT_VARIABLE LLVM_LIBS ERROR_QUIET)
    string(STRIP "${LLVM_LIBS}" LLVM_LIBS)
    if(NOT LLVM_LIBS)
      message(STATUS "${LLVM_CONFIG} --libs failed")
    endif()
    set(LLVM_LIBS "${LLVM_LIBS} ${LLVM_LDFLAGS} ${LLVM_SYSTEM_LIBS}")
    # The c++ standard library used by llvm doesn't show up in $LLVM_SYSTEM_LIBS so add it manually
    if(LLVM_CXXFLAGS MATCHES "-stdlib=libc\\+\\+")
      set(LLVM_LIBS "${LLVM_LIBS} -lc++")
    else()
      set(LLVM_LIBS "${LLVM_LIBS} -lstdc++")
    endif()

  else()
    # Windows
    set(LLVM_CFLAGS "-I${WITH_LLVM}/include -DNDEBUG -D__NO_CTYPE_INLINE -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
    set(LLVM_CXXFLAGS "${LLVM_CFLAGS} -std=gnu++11 -fvisibility-inlines-hidden -fno-rtti -Woverloaded-virtual -Wcast-qual")
    set(LLVM_LDFLAGS "-L${WITH_LLVM}/lib")
    set(LLVM_SYSTEM_LIBS "-lshell32 -lpsapi -limagehlp -ldbghelp -lm")
    set(LLVM_LIBS "-lLLVMLTO -lLLVMObjCARCOpts -lLLVMLinker -lLLVMipo -lLLVMVectorize -lLLVMBitWriter \
      -lLLVMARMDisassembler -lLLVMARMCodeGen -lLLVMARMAsmParser -lLLVMARMDesc -lLLVMARMInfo \
      -lLLVMARMAsmPrinter -lLLVMTableGen -lLLVMDebugInfo -lLLVMOption -lLLVMX86Disassembler \
      -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMX86Desc \
      -lLLVMMCDisassembler -lLLVMX86Info -lLLVMX86AsmPrinter -lLLVMX86Utils -lLLVMJIT \
      -lLLVMAnalysis -lLLVMTarget \
      -lLLVMIRReader -lLLVMAsmParser -lLLVMLineEditor -lLLVMMCAnalysis -lLLVMInstrumentation \
      -lLLVMInterpreter -lLLVMCodeGen -lLLVMScalarOpts -lLLVMInstCombine -lLLVMTransformUtils \
      -lLLVMipa -lLLVMAnalysis -lLLVMProfileData -lLLVMMCJIT -lLLVMTarget -lLLVMRuntimeDyld \
      -lLLVMObject -lLLVMMCParser -lLLVMBitReader -lLLVMExecutionEngine -lLLVMMC -lLLVMCore \
      -lLLVMSupport -lstdc++")
    set(LLVM_LIBS "${LLVM_LIBS} ${LLVM_SYSTEM_LIBS}")

    set(LLVM_CONFIG_PATH "${WITH_LLVM}/include/llvm/Config/llvm-config.h")
    file(READ "${LLVM_CONFIG_PATH}" LLVM_API_VERSION)
    string(REGEX REPLACE ".*MONO_API_VERSION ([0-9]+).*" "\\1" LLVM_API_VERSION "${LLVM_API_VERSION}")
    message(STATUS "LLVM_API_VERSION = ${LLVM_API_VERSION}")
  endif()

  if(NOT LLVM_API_VERSION)
    set(LLVM_CFLAGS "${LLVM_CFLAGS} -DLLVM_API_VERSION=0")
    set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} -DLLVM_API_VERSION=0")
  else()
    set(LLVM_CFLAGS "${LLVM_CFLAGS} -DLLVM_API_VERSION=${LLVM_API_VERSION}")
    set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} -DLLVM_API_VERSION=${LLVM_API_VERSION}")
  endif()

endif()

set(MONO_LLVM_LOADED ${ENABLE_LOADEDLLVM})
set(LLVM_LOADED ${ENABLE_LOADEDLLVM})
set(ENABLE_LLVM_RUNTIME ${ENABLE_LLVM})

set(TARGET "unknown")
set(ACCESS_UNALIGNED YES)

set(LIBC "libc.so.6")
set(INTL "libc.so.6")
set(SQLITE "libsqlite.so.0")
set(SQLITE3 "libsqlite3.so.0")
set(X11 "libX11.so")
set(GDKX11 "libgdk-x11-2.0.so.0")
set(GTKX11 "libgtk-x11-2.0.so.0")
set(XINERAMA "libXinerama.so.1")

set(SIZEOF_REGISTER "SIZEOF_VOID_P")
set(TARGET_BYTE_ORDER "G_BYTE_ORDER")

set(JIT_WANTED YES)
set(BOEHM_SUPPORTED YES)
if(HOST_TRIPLE MATCHES "^mips-.*")
  # TODO: Add MIPS support
  message(FATAL_ERROR "TODO: Add MIPS support")
elseif(HOST_TRIPLE MATCHES "^x86-.*")
  set(TARGET "X86")
  set(ARCH_TARGET "x86")
  if(HOST_SOLARIS)
    set(LIBC "libc.so")
    set(INTL "libintl.so")

    # On solaris 10 x86, gcc prints a warning saying 'visibility attribute not supported on this configuration; ignored', but linking fails. A test case:
    # int astruct __attribute__ ((visibility ("hidden")));
    # void foo ()
    # {
    #	void *p = &astruct;
    # }
    # gcc -fPIC --shared -o libfoo.so foo.c
    # yields:
    # foo.c:6: warning: visibility attribute not supported in this configuration; ignored
    # ld: fatal: relocation error: R_386_GOTOFF: file /var/tmp//ccxYR96k.o: symbol astruct: relocation must bind locally
    set(HAVE_VISIBILITY_HIDDEN NO)
  elseif(HOST_WINDOWS)
    set(HAVE_VISIBILITY_HIDDEN NO)
  elseif(HOST_HAIKU)
    set(LIBC libroot.so)
  elseif(HOST_LINUX)
    set(AOT_SUPPORTED YES)
  elseif(HOST_DARWIN)
    set(AOT_SUPPORTED YES)
  endif()
elseif(HOST_TRIPLE MATCHES "^x86_64-.*")
  set(TARGET "AMD64")
  set(ARCH_TARGET "amd64")
  if(HOST_32BIT)
    set(__mono_ilp32__ YES)
    set(SIZEOF_REGISTER 8)
  endif()
  if(HOST_LINUX)
    set(AOT_SUPPORTED YES)
  elseif(HOST_DARWIN)
    set(AOT_SUPPORTED YES)
  endif()
  # TODO: NaCl
elseif(HOST_TRIPLE MATCHES "^ia64-.*")
    # TODO: Add ia64 support
    message(FATAL_ERROR "TODO: Add ia64 support")
elseif(HOST_TRIPLE MATCHES "^sparc.*")
  # TODO: Add sparc support
  message(FATAL_ERROR "TODO: Add sparc support")
elseif(HOST_WINDOWS)
  # When this is enabled, it leads to very strange crashes at runtime (gcc-3.4.4)
  set(HAVE_VISIBILITY_HIDDEN NO)
  set(INTL "intl")
else()
  # TODO: Port all triples support by configure.ac
  message(FATAL_ERROR "TODO: Add ${HOST_TRIPLE} support")
endif()

if(MONO_CROSS_COMPILE)
  # TODO: Add cross compile support
  message(FATAL_ERROR "Cross compiling is not supported for target ${TARGET_TRIPLE}")
endif()

# *************
# *** VTUNE ***
# *************
set(ENABLE_VTUNE NO CACHE BOOL "Enable the VTUNE back-end")
set(WITH_VTUNE "" CACHE FILEPATH "Enable jit vtune profiling")
if(WITH_VTUNE)
  set(ENABLE_VTUNE YES)
endif()

if(ENABLE_VTUNE)
  set(HAVE_VTUNE YES)
  if(NOT WITH_VTUNE)
    set(VTUNE_PATH /opt/intel/vtune_amplifier_xe)
  else()
    set(VTUNE_PATH "${WITH_VTUNE}")
  endif()
  set(VTUNE_INCLUDE "${VTUNE_PATH}/include")
  if(TARGET_X86)
    set(VTUNE_LIB "${VTUNE_PATH}/lib32")
  elseif(TARGET_X86_64)
    set(VTUNE_LIB "${VTUNE_PATH}/lib64")
  else()
    message(FATAL_ERROR "Unsupported target ${TARGET} for VTUNE.")
  endif()
  if(NOT EXISTS "${VTUNE_INCLUDE}/jitprofiling.h")
    message(FATAL_ERROR "VTUNE ${VTUNE_INCLUDE}/jitprofiling.h not found.")
  endif()
  if(NOT EXISTS "${VTUNE_LIB}/libjitprofiling.a")
    message(FATAL_ERROR "VTUNE ${VTUNE_LIB}/libjitprofiling.a not found.")
  endif()

  set(VTUNE_CFLAGS "-I${VTUNE_INCLUDE}")
  set(VTUNE_LIBS "-L/${VTUNE_LIB}/ -ljitprofiling")
endif()

# Use GCC atomic ops if they work on the target.
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  if("${TARGET}" MATCHES "^(X86|AMD64|ARM|ARM64|POWERPC|POWERPC64|MIPS|S390X|SPARC|SPARC64)$")
    set(USE_GCC_ATOMIC_OPS YES)
  endif()
endif()

if(TARGET_MACH)
  if(TARGET_SYS STREQUAL "WATCHOS")
    set(TARGET_WATCHOS YES)
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DTARGET_WATCHOS)
  elseif("${TARGET}" STREQUAL "ARM" OR "${TARGET}" STREQUAL "ARM64")
    set(TARGET_IOS YES)
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DTARGET_IOS)
  else()
    check_c_snippet_compiles("
      #if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
        #error fail this for ios
      #endif
      " "TargetConditionals.h" CHECK_TARGET_OSX)
    if(CHECK_TARGET_OSX)
      set(TARGET_OSX YES)
      set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DTARGET_OSX)
    else()
      set(TARGET_IOS YES)
      set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DTARGET_IOS)
    endif()
  endif()
endif()

#
# Simple Generational checks (sgen)
#
set(SGEN_DEFINES "")
set(ENABLE_SGEN YES CACHE BOOL "Extra Generational GC, default=yes")
if(ENABLE_SGEN)
  set(HAVE_MOVING_COLLECTOR YES)
  set(SGEN_DEFINES "-DHAVE_SGEN_GC")
  set(GC_MSG "sgen and ${GC_MSG}")
endif()
set(SUPPORT_SGEN ${ENABLE_SGEN})

set(JIT_STATUS "Building and using the JIT")

set(LIBSUFFIX ".so")

if(HOST_DARWIN)
  set(LIBSUFFIX ".dylib")
  set(LIBC "libc.dylib")
  set(INTL "libintl.dylib")
  set(SQLITE "libsqlite.0.dylib")
  set(SQLITE3 "libsqlite3.0.dylib")
  set(X11 "libX11.dylib")
  set(GDKX11 "libgdk-x11-2.0.dylib")
  set(GTKX11 "libgtk-x11-2.0.dylib")
elseif(HOST_WINDOWS)
  # Not applicable
else()
  message(FATAL_ERROR "TODO: Port support for ${HOST_TRIPLE}")
endif()

# TODO: libgdiplus support

set(ENABLE_ICALL_SYMBOL_MAP NO CACHE BOOL "Generate tables which map icall functions to their C symbols")
set(ENABLE_ICALL_EXPORT NO CACHE BOOL "Export icall functions")
set(DISABLE_ICALL_TABLES NO CACHE BOOL "Disable the runtime lookup of icalls")

if(WITH_TLS STREQUAL "__thread")
  set(HAVE_KW_THREAD YES)
  # Pass the information to libgc
  set(CPPFLAGS "${CPPFLAGS} -DUSE_COMPILER_TLS")
  message(STATUS "Checking if the tls_model attribute is supported")
  check_c_snippet_compiles("
        static __thread int foo __attribute__((tls_model(\"initial-exec\")));
      " "" HAVE_TLS_MODEL_ATTR)
endif()

if(${TARGET} STREQUAL "ARM")
  # ******************************************
  # *** Check to see what FPU is available ***
  # ******************************************
  message("Checking which FPU to use")

  #
  # This is a bit tricky:
  #
  # if (__ARM_PCS_VFP) {
  # 	/* mfloat-abi=hard == VFP with hard ABI */
  # } elif (!__SOFTFP__) {
  # 	/* mfloat-abi=softfp == VFP with soft ABI */
  # } else {
  # 	/* mfloat-abi=soft == no VFP */
  # }
  #
  # The exception is iOS (w/ GCC) where none of the above
  # are defined (but iOS always uses the 'softfp' ABI).
  #
  # No support for FPA.
  #

  set(FPU "")

  # iOS GCC always uses the 'softfp' ABI.
  #if test x"$GCC" = xyes && test x$host_darwin = xyes; then
  #  fpu=VFP
  #fi

  # Are we using the 'hard' ABI?
  if(NOT FPU)
    check_c_snippet_compiles("
      #ifndef __ARM_PCS_VFP
        #error \"Float ABI is not 'hard'\"
      #endif" "" HAVE_VFP_HARD)
    if(HAVE_VFP_HARD)
      set(FPU VFP_HARD)
    endif()
  endif()

  # No 'hard' ABI. 'soft' or 'softfp'?
  if(NOT FPU)
    check_c_snippet_compiles("
      #ifdef __SOFTFP__
        #error \"Float ABI is not 'softfp'\"
      #endif" "" HAVE_VFP)
    if(HAVE_VFP)
      set(FPU VFP)
    endif()
  endif()

  if(NOT FPU)
    set(FPU NONE)
  endif()

  message(STATUS "FPU is ${FPU}")
  set(CPPFLAGS "${CPPFLAGS} -DARM_FPU_${FPU}=1")

  # *********************************************
  # *** Check which ARM version(s) we can use ***
  # *********************************************
  message(STATUS "Checking which ARM version to use")

  check_c_snippet_compiles("
    #if !defined(__ARM_ARCH_5T__) && !defined(__ARM_ARCH_5TE__) && !defined(__ARM_ARCH_5TEJ__)
		  #error Not on ARM v5.
		#endif" "" HAVE_ARMV5)
  check_c_snippet_compiles("
    #if !defined(__ARM_ARCH_6J__) && !defined(__ARM_ARCH_6ZK__) && !defined(__ARM_ARCH_6K__) && !defined(__ARM_ARCH_6T2__) && !defined(__ARM_ARCH_6M__)
		  #error Not on ARM v6.
		#endif" "" HAVE_ARMV6)
  check_c_snippet_compiles("
    #if !defined(__ARM_ARCH_7A__) && !defined(__ARM_ARCH_7R__) && !defined(__ARM_ARCH_7EM__) && !defined(__ARM_ARCH_7M__) && !defined(__ARM_ARCH_7S__)
		  #error Not on ARM v7.
		#endif" "" HAVE_ARMV7)

  if(HAVE_ARMV7)
    message(STATUS "Found ARMv7")
    set(HAVE_ARMV5 YES)
    set(HAVE_ARMV6 YES)
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DHAVE_ARMV5=1 -DHAVE_ARMV6=1 -DHAVE_ARMV7=1)
  elseif(HAVE_ARMV6)
    message(STATUS "Found ARMv6")
    set(HAVE_ARMV5 YES)
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DHAVE_ARMV5=1 -DHAVE_ARMV6=1)
  elseif(HAVE_ARMV5)
    message(STATUS "Found ARMv5")
    set(LIBGC_COMPILE_DEFINITIONS ${LIBGC_COMPILE_DEFINITIONS} -DHAVE_ARMV5=1)
  endif()
endif()

if("${TARGET}" STREQUAL "ARM")
  if(JUMPTABLES)
    set(USE_JUMP_TABLES YES)
  endif()
endif()

if(NOT ACCESS_UNALIGNED)
  set(CPPFLAGS "${CPPFLAGS} -DNO_UNALIGNED_ACCESS")
endif()

if("${TARGET}" STREQUAL "unknown")
  set(CPPFLAGS "${CPPFLAGS} -DNO_PORT")
  message(STATUS "mono has not been ported to ${HOST_TRIPLE}: some things may not work.")
endif()

set(WITH_PROFILE4_X YES CACHE BOOL "If you want to install the 4.6 FX (defaults to yes)")
set(WITH_MONODROID NO CACHE BOOL "If you want to build the MonoDroid assemblies (defaults to no)")
set(WITH_MONOTOUCH NO CACHE BOOL "If you want to build the Xamarin.iOS assemblies (defaults to no)")
set(WITH_MONOTOUCH_WATCH NO CACHE BOOL "If you want to build the Xamarin.WatchOS assemblies (defaults to no)")
set(WITH_MONOTOUCH_TV NO CACHE BOOL "If you want to build the Xamarin.TVOS assemblies (defaults to no)")
set(WITH_BITCODE NO CACHE BOOL "If bitcode is enabled (defaults to no)")
set(WITH_XAMMAC NO CACHE BOOL "If you want to build the Xamarin.Mac assemblies (defaults to no)")
set(WITH_MOBILE_STATIC NO CACHE BOOL "If you want to build the mobile_static assemblies (defaults to no)")

set(OPROFILE NO)
set(WITH_OPROFILE "" CACHE PATH "Enable oprofile support (defaults to no)")
if(WITH_OPROFILE)
  set(OPROFILE_INCLUDE "${WITH_OPROFILE}/include")
  if(NOT EXISTS "${OPROFILE_INCLUDE}/opagent.h")
    message(STATUS "oprofile include file not found at ${OPROFILE_INCLUDE}/opagent.h")
  endif()
  set(OPROFILE YES)
  set(OPROFILE_CFLAGS "-I${OPROFILE_INCLUDE}")
  set(OPROFILE_LIBS "-L${WITH_OPROFILE}/lib/oprofile -lopagent")
  set(HAVE_OPROFILE YES)
endif()

set(USE_MALLOC_FOR_MEMPOOLS NO CACHE BOOL "Use malloc for each single mempool allocation (only for runtime debugging, defaults to no)")

set(DISABLE_MCS_DOCS NO CACHE BOOL "If you want to build the documentation under mcs (defaults to yes)")
if(NOT WITH_PROFILE4_X)
  set(DISABLE_MCS_DOCS YES)
endif()

set(LAZY_GC_THREAD_CREATION NO CACHE BOOL "Enable lazy runtime thread creation, embedding host must do it explicitly (defaults to no)")

set(USE_COOP_GC NO CACHE BOOL "Enable cooperative stop-the-world garbage collection (sgen only) (defaults to no)")

set(CHECKED_BUILD "" CACHE STRING "To enable checked build (expensive asserts), configure with a comma-separated LIST of checked build modules and then include that same list in the environment variable MONO_CHECK_MODE at runtime. Recognized checked build modules: all, gc, metadata, thread")
if(CHECKED_BUILD)
  string(REGEX REPLACE "[ ]*,[ ]*" ";" CHECKED_BUILD "${CHECKED_BUILD}")
  foreach(F ${CHECKED_BUILD})
    if("${F}" STREQUAL "all")
      set(ENABLE_CHECKED_BUILD_GC YES)
      set(ENABLE_CHECKED_BUILD_METADATA YES)
      set(ENABLE_CHECKED_BUILD_THREAD YES)
    elseif("${F}" STREQUAL "gc")
      set(ENABLE_CHECKED_BUILD_GC YES)
    elseif("${F}" STREQUAL "metadata")
      set(ENABLE_CHECKED_BUILD_METADATA YES)
    elseif("${F}" STREQUAL "thread")
      set(ENABLE_CHECKED_BUILD_THREAD YES)
    endif()
  endforeach()
endif()
if(ENABLE_CHECKED_BUILD_GC)
  set(CPPFLAGS "${CPPFLAGS} -DENABLE_CHECKED_BUILD_GC")
endif()
if(ENABLE_CHECKED_BUILD_METADATA)
  set(CPPFLAGS "${CPPFLAGS} -DENABLE_CHECKED_BUILD_METADATA")
endif()
if(ENABLE_CHECKED_BUILD_GC)
  set(CPPFLAGS "${CPPFLAGS} -DENABLE_CHECKED_BUILD_THREAD")
endif()

check_include_files(malloc.h HAVE_USR_INCLUDE_MALLOC_H)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  # Implicit function declarations are not 64 bit safe
  # Do this late, since this causes lots of configure tests to fail
  set(CFLAGS "${CFLAGS} -Werror-implicit-function-declaration")
  # jay has a lot of implicit declarations
  set(JAY_CFLAGS "-Wno-implicit-function-declaration")
endif()

#
# Consistency settings
#
if(CMAKE_CROSSCOMPILING OR NOT ENABLE_MCS_BUILD)
  set(DISABLE_MCS_DOCS YES)
  set(WITH_PROFILE4_X NO)
  set(WITH_MONODROID NO)
  set(WITH_MONOTOUCH NO)
  set(WITH_MONOTOUCH_WATCH NO)
  set(WITH_MONOTOUCH_TV NO)
  set(WITH_XAMMAC NO)
  set(WITH_MOBILE_STATIC NO)
endif()

# TODO: Add docs
#if test x$DISABLE_MCS_DOCS = xyes; then
#  docs_dir=""
#else
#  docs_dir=docs
#fi
#AC_SUBST(docs_dir)

## Maybe should also disable if mcsdir is invalid.  Let's punt the issue for now.
if(NOT CMAKE_CROSSCOMPILING AND ENABLE_MCS_BUILD)
  set(BUILD_MCS YES)
endif()

set(INSTALL_4_X ${WITH_PROFILE4_X})
set(INSTALL_MONODROID ${WITH_MONODROID})
set(INSTALL_MONOTOUCH ${WITH_MONOTOUCH})
set(INSTALL_MONOTOUCH_WATCH ${WITH_MONOTOUCH_WATCH})
set(INSTALL_MONOTOUCH_TV ${WITH_MONOTOUCH_TV})
set(BITCODE ${WITH_BITCODE})
set(INSTALL_XAMMAC ${WITH_XAMMAC})
set(INSTALL_MOBILE_STATIC ${WITH_MOBILE_STATIC})

#AM_CONDITIONAL(MIPS_GCC, test ${TARGET}${ac_cv_prog_gcc} = MIPSyes)
#AM_CONDITIONAL(MIPS_SGI, test ${TARGET}${ac_cv_prog_gcc} = MIPSno)
set(SPARC ${TARGET_SPARC})
set(SPARC64 ${TARGET_SPARC64})
set(X86 ${TARGET_X86})
set(AMD64 ${TARGET_AMD64})
set(IA64 ${TARGET_IA64})
set(MIPS ${TARGET_MIPS})
set(POWERPC ${TARGET_POWERPC})
set(POWERPC64 ${TARGET_POWERPC64})
set(ARM ${TARGET_ARM})
set(ARM64 ${TARGET_ARM64})
set(S390X ${TARGET_S390X})
set(CROSS_COMPILE ${CMAKE_CROSSCOMPILING})

set(MONO_ARCHITECTURE "${ARCH_TARGET}")

#This must always be defined when building the runtime
set(MONO_INSIDE_RUNTIME YES) # Disable banned functions from being used by the runtime

set(MONO_BUILD_ROOT ${CMAKE_SOURCE_DIR})
set(MONO_RUNTIME mono/mini/mono)
set(MONO_CFG_ROOT ${MONO_BUILD_ROOT}/runtime)
set(MONO_CFG_DIR ${MONO_CFG_ROOT}/etc)

# TODO: DEFAULT_PROFILE ?

# TODO: Do we need to set up those symbolic links?
# TODO: Do we need to port all those AC_CONFIG_COMMANDS ?

# TODO: Make this work on Windows outside of mingw/cygwin
execute_process(COMMAND "${CMAKE_SOURCE_DIR}/scripts/update_submodules")

set(LIBGDIPLUS_MSG "assumed to be installed")
if(LIBGDIPLUS_LOC)
  set(LIBGDIPLUS_MSG "${LIBGDIPLUS_LOC}")
endif()

# Generate top level config.h

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-cmake.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Compile libgc BEFORE we set global C/CXX/LD flags
if(SUPPORT_BOEHM)
  add_subdirectory(libgc)
  #  ExternalProject_Add(extlibgc
  #          URL file://${CMAKE_SOURCE_DIR}/libgc
  #          UPDATE_COMMAND ""
  #          CONFIGURE_COMMAND bash -c "CFLAGS='${CFLAGS_FOR_LIBGC} -I${CMAKE_SOURCE_DIR} -I${CMAKE_BINARY_DIR}' CPPFLAGS='${CPPFLAGS_FOR_LIBGC} -I${CMAKE_SOURCE_DIR} -I${CMAKE_BINARY_DIR}' ./configure --disable-embed-check --with-libgc-threads=${LIBGC_THREADS} ${LIBGC_CONFIGURE_ARGS} --prefix=${CMAKE_BINARY_DIR}/gc"
  #          INSTALL_COMMAND ""
  #          BUILD_IN_SOURCE 1
  #          )
  #  add_library(monogc STATIC IMPORTED)
  #  set_target_properties(monogc PROPERTIES IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/extlibgc-prefix/src/extlibgc/.libs/libmonogc.a")
  #  add_dependencies(monogc extlibgc)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CONFIG_H ${CFLAGS} ${CPPFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_CONFIG_H ${CXXFLAGS} ${CPPFLAGS}")
foreach(LIB ${LIBS})
  if(MSVC)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LIB}.lib")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIB}.lib")
  else()
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -l${LIB}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -l${LIB}")
  endif()
endforeach()

message(STATUS "CMAKE_C_FLAGS = ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_C_FLAGS_DEBUG = ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_C_FLAGS_RELEASE = ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS = ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")

#SUBDIRS = po $(libgc_dir) eglib mono $(ikvm_native_dir) support data runtime scripts man samples msvc $(docs_dir) acceptance-tests
#add_subdirectory(po)
#add_subdirectory(libgc)
add_subdirectory(eglib)
add_subdirectory(mono)

#add_executable(mono ${SOURCE_FILES})

message(STATUS "
        mcs source:  ${MCSDIR}

  Engine:
    Host:            ${HOST_TRIPLE}
    Target:          ${TARGET}
    GC:	             ${GC_MSG}
    TLS:             ${WITH_TLS}
    SIGALTSTACK:     ${WITH_SIGALTSTACK}
    Engine:          ${JIT_STATUS}
    oprofile:        ${OPROFILE}
    BigArrays:       ${ENABLE_BIG_ARRAYS}
    DTrace:          ${ENABLE_DTRACE}
    LLVM Back End:   ${ENABLE_LLVM} (dynamically loaded: ${ENABLE_LOADEDLLVM})

  Libraries:
    .NET 4.6:        ${WITH_PROFILE4_X}
    Xamarin.Android: ${WITH_MONODROID}
    Xamarin.iOS:     ${WITH_MONOTOUCH}
    Xamarin.WatchOS: ${WITH_MONOTOUCH_WATCH}
    Xamarin.TVOS:    ${WITH_MONOTOUCH_TV}
    Xamarin.Mac:     ${WITH_XAMMAC}
    mobile_static:   ${WITH_MOBILE_STATIC}
    JNI support:     ${JDK_HEADERS_FOUND}
    libgdiplus:      ${LIBGDIPLUS_MSG}
    zlib:            ${ZLIB_MSG}
    Disabled:        ${DISABLED_FEATURES}
")

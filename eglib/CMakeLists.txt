cmake_minimum_required(VERSION 2.8.11)
project(eglib)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(TestBigEndian)

# Package info
set(PACKAGE_NAME eglib)
set(PACKAGE "${PACKAGE_NAME}")
set(PACKAGE_VERSION 0.3)
set(PACKAGE_BUGREPORT "http://bugzilla.xamarin.com/enter_bug.cgi?classification=Mono")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_URL "")

# FIXME: replace with add_compile_options
add_definitions(-g -Wall)

if(CFLAGS_FOR_EGLIB)
  # FIXME: move logic from ../configure.ac to here
  add_definitions(${CFLAGS_FOR_EGLIB})
endif()

# FIXME: replace with target_link_libraries + figure out if both src and test need linking against textencoding
if(TARGET_HAIKU)
  add_definitions(-ltextencoding)
endif()

set(HAVE_CLASSIC_WINAPI_SUPPORT 1)
set(HAVE_UWP_WINAPI_SUPPORT 0)

check_type_size("void*" SIZEOF_VOID_P)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)

check_function_exists(strlcpy HAVE_STRLCPY)
check_function_exists(stpcpy HAVE_STPCPY)
check_function_exists(strtok_r HAVE_STRTOK_R)
check_function_exists(rewinddir HAVE_REWINDDIR)
check_function_exists(vasprintf HAVE_VASPRINTF)
check_function_exists(strerror_r HAVE_STRERROR_R)
check_function_exists(getrlimit HAVE_GETRLIMIT)
check_function_exists(fork HAVE_FORK)
check_function_exists(execv HAVE_EXECV)
check_function_exists(execve HAVE_EXECVE)

# Mono currently supports 10.6, but strndup is not available prior to 10.7; avoiding
# the detection of strndup on OS X so Mono built on 10.7+ still runs on 10.6. This can be
# removed once support for 10.6 is dropped.
if(TARGET_OSX)
  check_function_exists(getpwuid_r HAVE_GETPWUID_R)
elseif(NOT TARGET_IOS)
  check_function_exists(strndup HAVE_STRNDUP)
  check_function_exists(getpwuid_r HAVE_GETPWUID_R)
endif()

check_include_file(alloca.h HAVE_ALLOCA_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(iconv.h HAVE_ICONV_H)
check_include_file(localcharset.h HAVE_LOCALCHARSET_H)
check_include_file(pwd.h HAVE_PWD_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)

if(NOT HOST_WIN32 AND NOT HOST_DARWIN AND NOT CROSS_COMPILING)
  # add_subdirectory(test)
endif()

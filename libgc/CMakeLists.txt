# Package info
set(PACKAGE_NAME libgc-mono)
set(PACKAGE "${PACKAGE_NAME}")
set(PACKAGE_VERSION 6.6)
set(PACKAGE_BUGREPORT "Hans_Boehm@hp.co")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_URL "")

set(LIBGC_ENABLE_WIN32_DLLMAIN NO CACHE BOOL "Define the DllMain function in win32_threads.c even if the collector is not built as a dll")
set(LIBGC_ENABLE_PARALLEL_MARK NO CACHE BOOL "Parallelize marking and free list construction")

# Top-level CMakeLists.txt checks for threading lib
if(PTHREAD_LIB)
  set(THREADDLLIBS ${THREADDLLIBS} ${PTHREAD_LIB})
elseif()
  set(THREADDLLIBS ${THREADDLLIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

if(HOST_LINUX AND HOST_CPU MATCHES "^(x86|alpha|s390|sparc|powerpc)")
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DGC_LINUX_THREADS=1)
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -D_REENTRANT=1)
  if(LIBGC_ENABLE_PARALLEL_MARK)
    set(LIBGC_DEFINES ${LIBGC_DEFINES} -DPARALLEL_MARK=1)
  endif()
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DTHREAD_LOCAL_ALLOC=1)
elseif(HOST_LINUX OR HOST_NACL)
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DGC_LINUX_THREADS=1)
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -D_REENTRANT=1)
elseif(HOST_DARWIN)
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DGC_DARWIN_THREADS=1)
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DTHREAD_LOCAL_ALLOC=1)
  if(LIBGC_ENABLE_PARALLEL_MARK)
    set(LIBGC_DEFINES ${LIBGC_DEFINES} -DPARALLEL_MARK=1)
  endif()
elseif(HOST_WINDOWS)
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DGC_WIN32_THREADS=1)
  # Wine getenv may not return NULL for missing entry
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DNO_GETENV=1)
  if(LIBGC_ENABLE_WIN32_DLLMAIN)
    set(LIBGC_DEFINES ${LIBGC_DEFINES} -DGC_INSIDE_DLL=1)
  endif()
else()
  # TODO: Port other supported platforms
  message(FATAL_ERROR "Unsupported platform ${HOST_TRIPLE}")
endif()

if(HOST_DARWIN AND HOST_POWERPC)
  set(POWERPC_DARWIN YES)
endif()

# Check if the GCC builtin __sync_bool_compare_and_swap is available.
# It is preferred in gc_locks.h for PPC as GCC 4.4 has a problem with the inline assembly there.
message(STATUS "Checking for __sync_bool_compare_and_swap")
check_c_source_compiles("
        volatile unsigned int foo = 0;
        int main(int argc, char** argv) {
          unsigned int r1 = __sync_bool_compare_and_swap(&foo, 0, 1);
          return 0;
        }
      " HAS___SYNC_BOOL_COMPARE_AND_SWAP)
if(HAS___SYNC_BOOL_COMPARE_AND_SWAP)
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DHAS___SYNC_BOOL_COMPARE_AND_SWAP=1)
endif()

message(STATUS "Checking for xlc")
check_c_snippet_compiles("
        #ifndef __xlC__
        # error
        #endif
      " "" COMPILER_XLC)
if(COMPILER_XLC AND POWERPC_DARWIN)
  # the darwin stack-frame-walking code is completely broken on xlc
  set(LIBGC_DEFINES ${LIBGC_DEFINES} -DDARWIN_DONT_PARSE_STACK=1)
endif()

# Top-level CMakeLists.txt checks for lib containing dlopen
# We never want libdl on darwin. It is a fake libdl that just ends up making
# dyld calls anyway
if(DLOPEN_LIB AND NOT HOST_DARWIN)
  set(THREADDLLIBS ${THREADDLLIBS} ${DLOPEN_LIB})
endif()

# TODO: Configuration of machine-dependent code
set(MACHDEP mach_dep.c)

# TODO: Solaris, dgux checks?

# Include defines that have become de facto standard.
# ALL_INTERIOR_POINTERS can be overridden in startup code.
set(LIBGC_DEFINES ${LIBGC_DEFINES} -DSILENT=1)
set(LIBGC_DEFINES ${LIBGC_DEFINES} -DNO_SIGNALS=1)
set(LIBGC_DEFINES ${LIBGC_DEFINES} -DNO_EXECUTE_PERMISSION=1)
#set(LIBGC_DEFINES ${LIBGC_DEFINES} -DALL_INTERIOR_POINTERS=1)

# By default, make the library as general as possible.
set(LIBGC_DEFINES ${LIBGC_DEFINES} -DJAVA_FINALIZATION=1)
set(LIBGC_DEFINES ${LIBGC_DEFINES} -DGC_GCJ_SUPPORT=1)
set(LIBGC_DEFINES ${LIBGC_DEFINES} -DATOMIC_UNCOLLECTABLE=1)

# TODO: Don't think the with_cross_host, full-debug, reidrect-malloc checks are ever used with mono.

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES allchblk.c alloc.c blacklst.c checksums.c dbg_mlc.c
  dyn_load.c finalize.c gc_dlopen.c gcj_mlc.c headers.c
  malloc.c mallocx.c mark.c mark_rts.c misc.c new_hblk.c
  obj_map.c os_dep.c pcr_interface.c ptr_chck.c real_malloc.c reclaim.c
  solaris_pthreads.c solaris_threads.c specific.c stubborn.c typd_mlc.c
  backgraph.c win32_threads.c
  pthread_support.c pthread_stop_world.c darwin_stop_world.c
  openbsd_stop_world.c mach_dep.c ${MACHDEP}
)

if(POWERPC_DARWIN)
  set(SOURCES ${SOURCES} powerpc_darwin_mach_dep.s)
endif()
if(TARGET_IA64)
  set(SOURCES ${SOURCES} ia64_save_regs_in_stack.s)
endif()

add_library(monogc_objects OBJECT ${SOURCES})
target_compile_definitions(monogc_objects PRIVATE ${LIBGC_DEFINES} ${LIBGC_COMPILE_DEFINITIONS})
target_compile_options(monogc_objects PRIVATE ${LIBGC_COMPILE_OPTIONS})
add_library(monogc INTERFACE)
target_sources(monogc INTERFACE $<TARGET_OBJECTS:monogc_objects>)
target_link_libraries(monogc INTERFACE ${THREADDLLIBS} ${UNWINDLIBS})

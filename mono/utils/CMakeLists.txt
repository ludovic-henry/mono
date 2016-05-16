
#AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/mono $(LIBGC_CPPFLAGS) $(GLIB_CFLAGS) $(SHARED_CFLAGS)
include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/mono
  ${CMAKE_SOURCE_DIR}/mono
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GLIB_INCLUDE_DIRECTORIES}
)

if(ENABLE_DTRACE)
  set(BUILT_SOURCES mono-dtrace.h)
endif()

set(SOURCES
  mono-md5.c
  mono-sha1.c
  mono-logger.c
  mono-logger-internals.h
  mono-codeman.c
  dlmalloc.h
  dlmalloc.c
  mono-counters.c
  mono-compiler.h
  mono-complex.h
  mono-dl.c
  mono-dl-windows.c
  mono-dl-darwin.c
  mono-dl-posix.c
  mono-dl.h
  mono-internal-hash.c
  mono-internal-hash.h
  mono-io-portability.c
  mono-io-portability.h
  monobitset.c
  mono-filemap.c
  mono-math.c
  mono-mmap.c
  mono-mmap.h
  mono-mmap-internals.h
  mono-os-mutex.h
  mono-coop-mutex.h
  mono-once.h
  mono-lazy-init.h
  mono-networkinterfaces.c
  mono-networkinterfaces.h
  mono-proclib.c
  mono-proclib.h
  mono-publib.c
  mono-string.h
  mono-time.c
  mono-time.h
  strenc.h
  strenc.c
  mono-uri.c
  mono-poll.c
  mono-path.c
  mono-os-semaphore.h
  mono-coop-semaphore.h
  mono-sigcontext.h
  mono-stdlib.c
  mono-property-hash.h
  mono-property-hash.c
  mono-value-hash.h
  mono-value-hash.c
  freebsd-elf_common.h
  freebsd-elf32.h
  freebsd-elf64.h
  freebsd-dwarf.h
  dtrace.h
  gc_wrapper.h
  mono-error.c
  mono-error-internals.h
  monobitset.h
  mono-codeman.h
  mono-counters.h
  mono-digest.h
  mono-error.h
  mono-machine.h
  mono-math.h
  mono-membar.h
  mono-path.h
  mono-poll.h
  mono-uri.h
  mono-stdlib.h
  valgrind.h
  mach-support.c
  mach-support.h
  memcheck.h
  mono-context.c
  mono-context.h
  mono-stack-unwinding.h
  hazard-pointer.c
  hazard-pointer.h
  lock-free-queue.c
  lock-free-queue.h
  lock-free-alloc.c
  lock-free-alloc.h
  lock-free-array-queue.c
  lock-free-array-queue.h
  mono-linked-list-set.c
  mono-linked-list-set.h
  mono-threads.c
  mono-threads-state-machine.c
  mono-threads-posix.c
  mono-threads-posix-signals.c
  mono-threads-posix-signals.h
  mono-threads-mach.c
  mono-threads-mach-helper.c
  mono-threads-windows.c
  mono-threads-linux.c
  mono-threads-freebsd.c
  mono-threads-openbsd.c
  mono-threads-android.c
  mono-threads.h
  mono-threads-api.h
  mono-threads-coop.c
  mono-threads-coop.h
  mono-threads-mach-abort-syscall.c
  mono-threads-posix-abort-syscall.c
  mono-threads-windows-abort-syscall.c
  mono-tls.h
  mono-tls.c
  linux_magic.h
  mono-memory-model.h
  atomic.h
  atomic.c
  mono-hwcap.h
  mono-hwcap.c
  bsearch.h
  bsearch.c
  mono-signal-handler.h
  mono-conc-hashtable.h
  mono-conc-hashtable.c
  sha1.h
  sha1.c
  json.h
  json.c
  networking.c
  networking-posix.c
  networking-fallback.c
  networking-missing.c
  networking-windows.c
  networking.h
  mono-rand.c
  mono-rand.h
  memfuncs.c
  memfuncs.h
  parse.c
  parse.h
  checked-build.c
  checked-build.h
)

if(NOT CMAKE_CROSSCOMPILE)
  if(TARGET_X86)
    list(APPEND SOURCES mach-support-x86.c)
  endif()

  if(TARGET_AMD64)
    list(APPEND SOURCES mach-support-amd64.c)
    if(MSVC)
      list(APPEND SOURCES win64.asm)
    endif()
  endif()

  if(TARGET_ARM)
    list(APPEND SOURCES mach-support-arm.c)
  endif()

  if(TARGET_ARM64)
    list(APPEND SOURCES mach-support-arm64.c)
  endif()
else()
  list(APPEND SOURCES mach-support-unknown.c)
endif()

if(TARGET_X86)
  list(APPEND SOURCES mono-hwcap-x86.c mono-hwcap-x86.h)
endif()

if(TARGET_AMD64)
  list(APPEND SOURCES mono-hwcap-x86.c mono-hwcap-x86.h)
endif()

if(TARGET_ARM)
  list(APPEND SOURCES mono-hwcap-arm.c mono-hwcap-arm.h)
endif()

if(TARGET_ARM64)
  list(APPEND SOURCES mono-hwcap-arm64.c mono-hwcap-arm64.h)
endif()

if(TARGET_MIPS)
  list(APPEND SOURCES mono-hwcap-mips.c mono-hwcap-mips.h)
endif()

if(TARGET_POWERPC)
  list(APPEND SOURCES mono-hwcap-ppc.c mono-hwcap-ppc.h)
endif()

if(TARGET_POWERPC64)
  list(APPEND SOURCES mono-hwcap-ppc.c mono-hwcap-ppc.h)
endif()

if(TARGET_SPARC)
  list(APPEND SOURCES mono-hwcap-sparc.c mono-hwcap-sparc.h)
endif()

if(TARGET_SPARC64)
  list(APPEND SOURCES mono-hwcap-sparc.c mono-hwcap-sparc.h)
endif()

if(TARGET_IA64)
  list(APPEND SOURCES mono-hwcap-ia64.c mono-hwcap-ia64.h)
endif()

if(TARGET_S390X)
  list(APPEND SOURCES mono-hwcap-s390x.c mono-hwcap-s390x.h)
endif()

add_library(monoutils_objects OBJECT ${SOURCES})
add_library(monoutils INTERFACE)
target_sources(monoutils INTERFACE $<TARGET_OBJECTS:monoutils_objects> $<TARGET_OBJECTS:monoutils_objects>)
target_link_object_libraries(monoutils eglib)

if(ENABLE_DTRACE)
  add_custom_command(OUTPUT mono-dtrace.h COMMAND ${DTRACE} ${DTRACEFLAGS} -h -s ${CMAKE_SOURCE_DIR}/data/mono.d -o mono-dtrace.h)
  add_custom_target(mono-dtrace.h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mono-dtrace.h)
  add_dependencies(monoutils_objects mono-dtrace.h)
endif()

# Also create a static library which is used when linking genmdesc in ../mini/. If we link it against monoutils we will
# end up with unresolved symbols since all object files will be linked eagerly even though genmdesc only uses symbols
# in a few of them. With a static lib only object files with referenced symbols will be linked in.
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty.c" "")
endif()
add_library(monoutils-static STATIC "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
target_link_libraries(monoutils-static PRIVATE monoutils)

#libmonoutilsincludedir = $(includedir)/mono-$(API_VER)/mono/utils

#libmonoutilsinclude_HEADERS = 	\
#mono-logger.h		\
#mono-error.h		\
#mono-publib.h		\
#mono-dl-fallback.h	\
#mono-counters.h

#EXTRA_DIST = mono-embed.h mono-embed.c

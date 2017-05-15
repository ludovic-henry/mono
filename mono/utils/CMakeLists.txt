
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
  mono-dl-windows-internals.h
  mono-log-windows.c
  mono-log-common.c
  mono-log-posix.c
  mono-log-android.c
  mono-log-darwin.c
  mono-internal-hash.c
  mono-internal-hash.h
  mono-io-portability.c
  mono-io-portability.h
  monobitset.c
  mono-filemap.c
  mono-math.c
  mono-mmap.c
  mono-mmap-windows.c
  mono-mmap.h
  mono-mmap-internals.h
  mono-mmap-windows-internals.h
  mono-os-mutex.h
  mono-coop-mutex.h
  mono-once.h
  mono-lazy-init.h
  mono-networkinterfaces.c
  mono-networkinterfaces.h
  mono-proclib.c
  mono-proclib-windows.c
  mono-proclib.h
  mono-proclib-windows-internals.h
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
  mono-threads-mach.c
  mono-threads-mach-helper.c
  mono-threads-windows.c
  mono-threads-linux.c
  mono-threads-freebsd.c
  mono-threads-netbsd.c
  mono-threads-openbsd.c
  mono-threads-android.c
  mono-threads-haiku.c
  mono-threads.h
  mono-threads-debug.h
  mono-threads-api.h
  mono-threads-coop.c
  mono-threads-coop.h
  mono-tls.h
  mono-tls.c
  linux_magic.h
  mono-memory-model.h
  atomic.h
  atomic.c
  mono-hwcap.h
  mono-hwcap.c
  mono-hwcap-vars.h
  bsearch.h
  bsearch.c
  mono-signal-handler.h
  mono-conc-hashtable.h
  mono-conc-hashtable.c
  json.h
  json.c
  networking.c
  networking-posix.c
  networking-fallback.c
  networking-missing.c
  networking-windows.c
  networking.h
  mono-rand.c
  mono-rand-windows.c
  mono-rand.h
  mono-rand-windows-internals.h
  memfuncs.c
  memfuncs.h
  parse.c
  parse.h
  checked-build.c
  checked-build.h
  os-event.h
  refcount.h
  w32api.h
)

if(HOST_WIN32)
  list(APPEND SOURCES os-event-win32.c)
else()
  list(APPEND SOURCES os-event-unix.c)
endif()

if(NOT CMAKE_CROSSCOMPILE)
  if(TARGET_X86)
    list(APPEND SOURCES mach-support-x86.c)
  elseif(TARGET_AMD64)
    list(APPEND SOURCES mach-support-amd64.c)
    if(MSVC)
      list(APPEND SOURCES win64.asm)
    endif()
  elseif(TARGET_ARM)
    list(APPEND SOURCES mach-support-arm.c)
  elseif(TARGET_ARM64)
    list(APPEND SOURCES mach-support-arm64.c)
  endif()
else()
  list(APPEND SOURCES mach-support-unknown.c)
endif()

if(NOT CMAKE_CROSSCOMPILE)
  if(TARGET_X86 OR TARGET_AMD64)
    list(APPEND SOURCES mono-hwcap-x86.c)
  elseif(TARGET_ARM)
    list(APPEND SOURCES mono-hwcap-arm.c)
  elseif(TARGET_ARM64)
    list(APPEND SOURCES mono-hwcap-arm64.c)
  elseif(TARGET_MIPS)
    list(APPEND SOURCES mono-hwcap-mips.c)
  elseif(TARGET_POWERPC OR TARGET_POWERPC64)
    list(APPEND SOURCES mono-hwcap-ppc.c)
  elseif(TARGET_SPARC OR TARGET_SPARC64)
    list(APPEND SOURCES mono-hwcap-sparc.c)
  elseif(TARGET_IA64)
    list(APPEND SOURCES mono-hwcap-ia64.c)
  elseif(TARGET_S390X)
    list(APPEND SOURCES mono-hwcap-s390x.c)
  endif()
else()
  list(APPEND SOURCES mono-hwcap-cross.c)
endif()

if(ENABLE_DTRACE)
  list(APPEND SOURCES mono-dtrace.h)
  add_custom_command(OUTPUT mono-dtrace.h
    COMMAND ${DTRACE} ${DTRACEFLAGS} -h -s ${CMAKE_SOURCE_DIR}/data/mono.d -o mono-dtrace.h)
endif()

add_library(monoutils-objects OBJECT ${SOURCES})
set_property(TARGET monoutils_objects PROPERTY FOLDER "Libraries")
set_property(TARGET monoutils_objects PROPERTY PROJECT_LABEL "monoutils (objects)")
add_library(monoutils INTERFACE)
target_sources(monoutils INTERFACE $<TARGET_OBJECTS:monoutils_objects>)
target_link_object_libraries(monoutils eglib)

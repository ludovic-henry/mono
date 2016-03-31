# TODO: Set ASSEMBLIESDIR, CONFDIR properly

if(HOST_WIN32)
  set(WIN32_SOURCES console-win32.c)

  set(PLATFORM_SOURCES ${WIN32_SOURCES})

  # Use -m here. This will use / as directory separator (C:/WINNT).
  # The files that use MONO_ASSEMBLIES and/or MONO_CFG_DIR replace the
  # / by \ if running under WIN32.
  if(CROSS_COMPILING)
    # TODO: ${LIBDIR}
    set(ASSEMBLIESDIR ${LIBDIR})
    # TODO: ${sysconfdir}
    set(CONFDIR ${SYSCONFDIR})
  else()
    #assembliesdir = `cygpath -m "${libdir}"`
    #confdir = `cygpath -m "${sysconfdir}"`
  endif()
  #export HOST_CC
  # The mingw math.h has "extern inline" functions that dont appear in libs, so
  # optimisation is required to actually inline them
  set(CFLAGS "-O")
else()

#  assembliesdir = $(exec_prefix)/lib
#  confdir = $(sysconfdir)
  set(UNIX_SOURCES console-unix.c)

  set(PLATFORM_SOURCES ${UNIX_SOURCES})
endif()

if(PLATFORM_ANDROID)
  set(PLATFORM_SOURCES ${PLATFORM_SOURCES}  ../../support/libm/complex.c)
endif()

#AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/mono $(LIBGC_CPPFLAGS) $(GLIB_CFLAGS) $(SHARED_CFLAGS)
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/mono
  ${CMAKE_BINARY_DIR}/mono
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBGC_INCLUDE_DIRECTORIES}
  ${GLIB_INCLUDE_DIRECTORIES}
)

#
# This library is used to localize the usage of MONO_BINDIR etc. to just one source file, thus enabling
# ccache to work even if the value of these defines change. We need to use a convenience library since automake
# doesn't support per file cflags.
#
add_library(monoruntime-config STATIC mono-config-dirs.h mono-config-dirs.c)
target_compile_definitions(monoruntime-config PUBLIC -DMONO_BINDIR="${bindir}" -DMONO_ASSEMBLIES="${assembliesdir}" -DMONO_CFG_DIR="${confdir}")

set(NULL_SOURCES  console-null.c)

set(NULL_GC_SOURCES null-gc.c)

set(COMMON_SOURCES
  ${PLATFORM_SOURCES}
  assembly.c
  attach.h
  attach.c
  cil-coff.h
  class.c
  class-internals.h
  cominterop.c
  cominterop.h
  console-io.h
  coree.c
  coree.h
  culture-info.h
  culture-info-tables.h
  debug-helpers.c
  debug-mono-symfile.h
  debug-mono-symfile.c
  debug-mono-ppdb.h
  debug-mono-ppdb.c
  decimal-ms.c
  decimal-ms.h
  domain-internals.h
  environment.c
  environment.h
  exception.c
  exception.h
  exception-internals.h
  file-io.c
  file-io.h
  filewatcher.c
  filewatcher.h
  gc-internals.h
  icall.c
  icall-def.h
  image.c
  image-internals.h
  jit-info.c
  loader.c
  locales.c
  locales.h
  lock-tracer.c
  lock-tracer.h
  marshal.c
  marshal.h
  mempool.c
  mempool.h
  mempool-internals.h
  metadata.c
  metadata-verify.c
  metadata-internals.h
  method-builder.h
  method-builder.c
  mono-basic-block.c
  mono-basic-block.h
  mono-config.c
  mono-debug.h
  mono-debug.c
  mono-debug-debugger.h
  mono-endian.c
  mono-endian.h
  mono-hash.h
  mono-mlist.c
  mono-mlist.h
  mono-perfcounters.c
  mono-perfcounters.h
  mono-perfcounters-def.h
  mono-ptr-array.h
  mono-route.c
  mono-route.h
  monitor.h
  nacl-stub.c
  normalization-tables.h
  number-formatter.h
  number-ms.c
  number-ms.h
  object-internals.h
  opcodes.c
  socket-io.c
  socket-io.h
  process.c
  process.h
  profiler.c
  profiler-private.h
  rand.h
  rand.c
  remoting.h
  remoting.c
  runtime.c
  mono-security.c
  security.h
  security-core-clr.c
  security-core-clr.h
  security-manager.c
  security-manager.h
  string-icalls.c
  string-icalls.h
  sysmath.h
  sysmath.c
  tabledefs.h
  threads.c
  threads-types.h
  threadpool-ms.c
  threadpool-ms.h
  threadpool-ms-io.c
  threadpool-ms-io.h
  verify.c
  verify-internals.h
  wrapper-types.h
  reflection-internals.h
  file-mmap-posix.c
  file-mmap-windows.c
  file-mmap.h
  object-offsets.h
  abi-details.h
  metadata-cross-helpers.c
  seq-points-data.h
  seq-points-data.c
  handle.c
  handle.h
)

# These source files have compile time dependencies on GC code
set(GC_DEPENDENT_SOURCES
  appdomain.c
  domain.c
  gc-stats.c
  gc.c
  monitor.c
  mono-hash.c
  object.c
  reflection.c
)

set(BOEHM_SOURCES boehm-gc.c)

set(SGEN_SOURCES
  sgen-os-posix.c
  sgen-os-mach.c
  sgen-os-win32.c
  sgen-os-coop.c
  sgen-bridge.c
  sgen-bridge.h
  sgen-bridge-internals.h
  sgen-old-bridge.c
  sgen-new-bridge.c
  sgen-tarjan-bridge.c
  sgen-toggleref.c
  sgen-toggleref.h
  sgen-stw.c
  sgen-mono.c
  sgen-client-mono.h
)

add_library(monoruntime SHARED ${COMMON_SOURCES} ${GC_DEPENDENT_SOURCES} ${NULL_GC_SOURCES} ${BOEHM_SOURCES})
target_compile_definitions(monoruntime PRIVATE ${BOEHM_DEFINES})
target_link_libraries(monoruntime monoruntime-config monoutils wapi eglib monogc-static)

add_library(monoruntime-static STATIC ${COMMON_SOURCES} ${GC_DEPENDENT_SOURCES} ${NULL_GC_SOURCES} ${BOEHM_SOURCES})
target_compile_definitions(monoruntime-static PRIVATE ${BOEHM_DEFINES})
target_link_libraries(monoruntime-static monoruntime-config monoutils wapi eglib monogc-static)

if(PLATFORM_DARWIN)
  target_link_libraries(monoruntime "-framework CoreFoundation" "-framework Foundation")
  target_link_libraries(monoruntime-static "-framework CoreFoundation" "-framework Foundation")
endif()

if(SUPPORT_SGEN)
  if(DISABLE_EXECUTABLES OR SHARED_MONO)
    add_library(monoruntimesgen SHARED ${COMMON_SOURCES} ${GC_DEPENDENT_SOURCES} ${SGEN_SOURCES})
    target_compile_definitions(monoruntimesgen PRIVATE ${SGEN_DEFINES})
    target_link_libraries(monoruntimesgen monoruntime-config monoutils wapi eglib monosgensgen)
    if(PLATFORM_DARWIN)
      target_link_libraries(monoruntimesgen "-framework CoreFoundation" "-framework Foundation")
    endif()
  endif()
  add_library(monoruntimesgen-static STATIC ${COMMON_SOURCES} ${GC_DEPENDENT_SOURCES} ${SGEN_SOURCES})
  target_compile_definitions(monoruntimesgen-static PRIVATE ${SGEN_DEFINES})
  target_link_libraries(monoruntimesgen-static monoruntime-config monoutils wapi eglib monosgensgen)
  if(PLATFORM_DARWIN)
    target_link_libraries(monoruntimesgen-static "-framework CoreFoundation" "-framework Foundation")
  endif()
endif()

#libmonoruntimeincludedir = $(includedir)/mono-$(API_VER)/mono/metadata

if(DTRACE_G_REQUIRED)
  # TODO: PEDUMP_DTRACE_OBJECT
  message(FATAL_ERROR "DTRACE_G_REQUIRED not yet supported")
#  add_library(<name> OBJECT <src>...)
#  set(PEDUMP_DTRACE_OBJECT pedump-dtrace.o)

#  pedump-dtrace.$(OBJEXT): $(top_srcdir)/data/mono.d libmonoruntime.la ../io-layer/libwapi.la ../utils/libmonoutils.la
#  DTRACE="$(DTRACE)" DTRACEFLAGS="$(DTRACEFLAGS)" AR="$(AR)" $(SHELL) $(top_srcdir)/data/dtrace-prelink.sh \
#  --pic pedump-dtrace.$(OBJEXT) $(top_srcdir)/data/mono.d libmonoruntime.la ../io-layer/libwapi.la ../utils/libmonoutils.la
endif()

if(SHARED_MONO AND SUPPORT_BOEHM)
  add_executable(pedump pedump.c)
  target_link_libraries(pedump monoruntime-static)
  if(PLATFORM_DARWIN)
    target_link_libraries(pedump "-framework CoreFoundation" "-framework Foundation")
  endif()
endif()

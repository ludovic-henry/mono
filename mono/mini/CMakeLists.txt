include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LIBGC_INCLUDE_DIRECTORIES}
  ${GLIB_INCLUDE_DIRECTORIES}
)

if(HOST_WIN32)
  set(LIBMONOLDFLAGS -no-undefined -avoid-version -Wl,--kill-at ${MONOLDFLAGS})
elseif(PLATFORM_ANDROID)
  set(LIBMONOLDFLAGS -avoid-version ${MONOLDFLAGS})
elseif(NOT PLATFORM_DARWIN)
  set(LIBMONOLDFLAGS ${MONOLDFLAGS} "-version-info 1:0:0")
endif()

if(LOADED_LLVM)
  add_library(mono-llvm mini-llvm.c mini-llvm-cpp.cpp llvm-jit.cpp)
  target_link_libraries(mono-llvm eglib ${LLVM_LIBS} ${LLVM_LDFLAGS})
  if(PLATFORM_DARWIN)
    target_link_libraries(mono-llvm -Wl,-undefined -Wl,suppress -Wl,-flat_namespace)
  else()
    target_link_libraries(mono-llvm monoboehm)
  endif()
endif()

set(MONO_BOEHM_SOURCES main.c)
set(MONO_SGEN_SOURCES main-sgen.c)

# We build this after libmono was built so it contains the date when the final link was done
if(SUPPORT_BOEHM)
  if(DISABLE_EXECUTABLES)
    set(DEPENDS mini-static monoruntime)
  else()
    set(DEPENDS mini-static monoruntime-static)
  endif()
  add_custom_command(
    OUTPUT buildver-boehm.h
    COMMAND ${CMAKE_COMMAND} -DFILE="${CMAKE_CURRENT_BINARY_DIR}/buildver-boehm.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVersionHeader.cmake"
    DEPENDS ${DEPENDS}
  )
  add_custom_target(generate_buildver_boehm_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/buildver-boehm.h)
endif()

if(DISABLE_EXECUTABLES)
  set(DEPENDS mini-static monoruntimesgen monosgen)
else()
  set(DEPENDS mini-static monoruntimesgen-static monosgen-static)
endif()
add_custom_command(
  OUTPUT buildver-sgen.h
  COMMAND ${CMAKE_COMMAND} -DFILE="${CMAKE_CURRENT_BINARY_DIR}/buildver-sgen.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVersionHeader.cmake"
  DEPENDS ${DEPENDS}
)
add_custom_target(generate_buildver_sgen_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/buildver-sgen.h)

if(DTRACE_G_REQUIRED)
  # TODO: Support DTRACE_G_REQUIRED
  message(FATAL_ERROR "DTRACE_G_REQUIRED not yet supported")
  #LIBMONO_DTRACE_OBJECT = .libs/mono-dtrace.$(OBJEXT)
  #if STATIC_MONO
  #MONO_DTRACE_OBJECT = mono-dtrace.$(OBJEXT)
  #else
  #MONO_DTRACE_OBJECT =
  #endif
  #else
  #MONO_DTRACE_OBJECT =
  #LIBMONO_DTRACE_OBJECT =
endif()

if(STATIC_MONO)
  # Link libmono into mono statically
  # This leads to higher performance, especially with TLS
  set(MONO_LIB monoboehm-static)
  set(MONO_SGEN_LIB monosgen-static)
else()
  set(MONO_LIB monoboehm)
  set(MONO_SGEN_LIB monosgen)
endif()

if(NOT LOADED_LLVM)
  set(LLVMMONOF ${LLVM_LIBS} ${LLVM_LDFLAGS})
endif()

if(SUPPORT_BOEHM)
  add_executable(mono-boehm ${MONO_BOEHM_SOURCES})
  target_link_libraries(mono-boehm ${MONO_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
  add_dependencies(mono-boehm generate_buildver_boehm_h)
endif()

add_executable(mono-sgen ${MONO_SGEN_SOURCES})
target_link_libraries(mono-sgen ${MONO_SGEN_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
add_dependencies(mono-sgen generate_buildver_sgen_h)

if(BITCODE)
  set(LIBMONOLDFLAGS ${LIBMONOLDFLAGS} -no-undefined)
endif()

# if SUPPORT_SGEN
#
# mono_LDADD = $(mono_sgen_LDADD)
# mono_LDFLAGS = $(mono_sgen_LDFLAGS)
#
# endif


if(DTRACE_G_REQUIRED)
  # TODO: Support DTRACE_G_REQUIRED
  message(FATAL_ERROR "DTRACE_G_REQUIRED not yet supported")

  #mono-dtrace.$(OBJEXT): $(top_srcdir)/data/mono.d mini.lo $(monodir)/mono/metadata/libmonoruntime-static.la
  #DTRACE="$(DTRACE)" DTRACEFLAGS="$(DTRACEFLAGS)" AR="$(AR)" $(SHELL) $(top_srcdir)/data/dtrace-prelink.sh \
  #$@ $(top_srcdir)/data/mono.d $(monodir)/mono/metadata/libmonoruntime-static.la mini.lo

  #.libs/mono-dtrace.$(OBJEXT): $(top_srcdir)/data/mono.d mini.lo $(monodir)/mono/metadata/libmonoruntime.la
  #DTRACE="$(DTRACE)" DTRACEFLAGS="$(DTRACEFLAGS)" AR="$(AR)" $(SHELL) $(top_srcdir)/data/dtrace-prelink.sh \
  #--pic $@ $(top_srcdir)/data/mono.d $(monodir)/mono/metadata/libmonoruntime.la mini.lo

endif()

# Create monow.exe, linked for the 'windows' subsystem
if(HOST_WIN32)
  if(SUPPORT_BOEHM)
    add_executable(monow ${MONO_BOEHM_SOURCES})
    target_link_libraries(monow ${MONO_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF} -mwindows)
  else()
    add_executable(monow ${MONO_SGEN_SOURCES})
    target_link_libraries(monow ${MONO_SGEN_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF} -mwindows)
  endif()
endif()



set(GENMDESC_SOURCES
  mini.h
  seq-points.h
  genmdesc.c
  helpers.c
  ../metadata/opcodes.c
)
add_executable(genmdesc ${GENMDESC_SOURCES})
target_link_libraries(genmdesc monoutils)

# Don't link this against libmonoruntime to speed up rebuilds
#set(genmdesc_LDADD = \
#$(monodir)/mono/utils/libmonoutils.la -lm	\
#$(GLIB_LIBS)					\
#$(LIBICONV)


set(X86_SOURCES
  mini-x86.c
  mini-x86.h
  exceptions-x86.c
  tramp-x86.c
)

set(AMD64_SOURCES
  mini-amd64.c
  mini-amd64.h
  exceptions-amd64.c
  tramp-amd64.c
)

set(PPC_SOURCES
  mini-ppc.c
  mini-ppc.h
  exceptions-ppc.c
  tramp-ppc.c
)

set(ARM_SOURCES
  mini-arm.c
  mini-arm-tls.S
  mini-arm.h
  mini-arm-tls.h
  exceptions-arm.c
  tramp-arm.c
)

set(ARM64_SOURCES
  mini-arm64.c
  mini-arm64.h
  exceptions-arm64.c
  tramp-arm64.c
)

set(MIPS_SOURCES
  mini-mips.c
  mini-mips.h
  exceptions-mips.c
  tramp-mips.c
)

set(SPARC_SOURCES
  mini-sparc.c
  mini-sparc.h
  exceptions-sparc.c
  tramp-sparc.c
)

set(S390X_SOURCES
  mini-s390x.c
  mini-s390x.h
  support-s390x.h
  exceptions-s390x.c
  tramp-s390x.c
)

set(IA64_SOURCES
  mini-ia64.c
  mini-ia64.h
  exceptions-ia64.c
  tramp-ia64.c
)

set(DARWIN_SOURCES
  mini-darwin.c
)

set(WINDOWS_SOURCES
  mini-windows.c
)

set(POSIX_SOURCES
  mini-posix.c
)

if(ENABLE_LLVM)
  if(LOADED_LLVM)
    set(LLVM_SOURCES mini-llvm-loaded.c)
  else()
    set(LLVM_SOURCES
      mini-llvm.c
      mini-llvm-loaded.c
      mini-llvm-cpp.cpp
      llvm-jit.cpp
    )
  endif()
endif()

if(ENABLE_LLVM OR ENABLE_LLVM_RUNTIME)
  set(LLVM_RUNTIME_SOURCES llvm-runtime.cpp)
endif()

set(COMMON_SOURCES
  mini.c
  mini-runtime.c
  seq-points.c
  seq-points.h
  ir-emit.h
  method-to-ir.c
  decompose.c
  mini.h
  version.h
  optflags-def.h
  jit-icalls.h
  jit-icalls.c
  trace.c
  trace.h
  patch-info.h
  mini-ops.h
  mini-arch.h
  dominators.c
  cfold.c
  regalloc.h
  helpers.c
  liveness.c
  ssa.c
  abcremoval.c
  abcremoval.h
  local-propagation.c
  driver.c
  debug-mini.c
  linear-scan.c
  aot-compiler.h
  aot-compiler.c
  aot-runtime.c
  graph.c
  mini-codegen.c
  mini-exceptions.c
  mini-trampolines.c
  branch-opts.c
  mini-generic-sharing.c
  simd-methods.h
  tasklets.c
  tasklets.h
  simd-intrinsics.c
  mini-native-types.c
  mini-unwind.h
  unwind.c
  image-writer.h
  image-writer.c
  dwarfwriter.h
  dwarfwriter.c
  mini-gc.h
  mini-gc.c
  debugger-agent.h
  debugger-agent.c
  xdebug.c
  mini-llvm.h
  mini-llvm-cpp.h
  alias-analysis.c
  mini-cross-helpers.c
  arch-stubs.c
  llvm-runtime.h
)

set(TEST_SOURCES
  basic-calls.cs
  basic-long.cs
  bench.cs
  objects.cs
  arrays.cs
  basic-float.cs
  basic-math.cs
  basic.cs
  exceptions.cs
  devirtualization.cs
  iltests.il
  test.cs
  generics.cs
  generics-variant-types.il
  basic-simd.cs
  aot-tests.cs
  gc-test.cs
  gshared.cs
)

set(REGTESTS basic.exe basic-float.exe basic-long.exe basic-calls.exe objects.exe arrays.exe basic-math.exe exceptions.exe iltests.exe devirtualization.exe generics.exe basic-simd.exe)
if(NACL_CODEGEN)
  set(TEST_SOURCES ${TEST_SOURCES} nacl.cs)
  set(REGTESTS ${REGTESTS} nacl.exe)
endif()

if(X86)
  set(ARCH_SOURCES ${X86_SOURCES})
  set(ARCH_BUILT cpu-x86.h)
  set(ARCH_DEFINE __i386__)
endif()

if(AMD64)
  set(ARCH_SOURCES ${AMD64_SOURCES})
  set(ARCH_BUILT cpu-amd64.h)
  set(ARCH_DEFINE __x86_64__)
  set(ARCH_FULLAOT_EXCLUDE "--exclude DYNCALL")
endif()

if(POWERPC)
  set(ARCH_SOURCES ${PPC_SOURCES})
  set(ARCH_BUILT cpu-ppc.h)
  set(ARCH_DEFINE __ppc__)
endif()

if(POWERPC64)
  set(ARCH_SOURCES ${PPC_SOURCES})
  set(ARCH_BUILT cpu-ppc64.h)
  set(ARCH_DEFINE __ppc64__)
endif()

if(MIPS)
  set(ARCH_SOURCES ${MIPS_SOURCES})
  set(ARCH_BUILT cpu-mips.h)
  set(ARCH_DEFINE __mips__)
endif()

if(ARM)
  # pick up arm_dpimacros.h
  include_directories(../arch/arm)
  set(ARCH_SOURCES ${ARM_SOURCES})
  set(ARCH_BUILT cpu-arm.h)
  set(ARCH_DEFINE __arm__)
endif()

if(ARM64)
  set(ARCH_SOURCES ${ARM64_SOURCES})
  set(ARCH_BUILT cpu-arm64.h)
  set(ARCH_DEFINE __aarch64__)
endif()

if(SPARC)
  set(ARCH_SOURCES ${SPARC_SOURCES})
  set(ARCH_BUILT cpu-sparc.h)
  set(ARCH_DEFINE __sparc__)
endif()

if(SPARC64)
  set(ARCH_SOURCES ${SPARC_SOURCES})
  set(ARCH_BUILT cpu-sparc.h)
  set(ARCH_DEFINE __sparc__)
endif()

if(S390X)
  set(ARCH_SOURCES ${S390X_SOURCES})
  set(ARCH_BUILT cpu-s390x.h)
  set(ARCH_DEFINE __s390__)
endif()

if(IA64)
  set(ARCH_SOURCES ${IA64_SOURCES})
  set(ARCH_BUILT cpu-ia64.h)
  set(ARCH_DEFINE __ia64__)
endif()

if(HOST_WIN32)
  set(OS_SOURCES ${WINDOWS_SOURCES})
  set(monobin_platform_ldflags "")
endif()

if(PLATFORM_SIGPOSIX)
  set(OS_SOURCES ${POSIX_SOURCES})
  set(monobin_platform_ldflags "")
endif()

if(PLATFORM_DARWIN)
  set(OS_SOURCES ${DARWIN_SOURCES} ${POSIX_SOURCES})
  #monobin_platform_ldflags=-sectcreate __TEXT __info_plist $(top_srcdir)/mono/mini/Info.plist -framework CoreFoundation -framework Foundation
  set(monobin_platform_ldflags "=-framework CoreFoundation" "-framework Foundation")
endif()

#AM_CFLAGS = \
#-I$(top_srcdir) 	\
#$(GLIB_CFLAGS)		\
#$(LLVM_CFLAGS)		\
#$(PLATFORM_CFLAGS) $(ARCH_CFLAGS) $(SHARED_CFLAGS)
#AM_CXXFLAGS = $(LLVM_CXXFLAGS) $(GLIB_CFLAGS)

add_library(mini SHARED ${COMMON_SOURCES} ${LLVM_SOURCES} ${LLVM_RUNTIME_SOURCES} ${ARCH_SOURCES} ${OS_SOURCES})
target_link_libraries(mini PUBLIC monoutils)
target_link_libraries(mini PRIVATE monoruntime)

# This library is shared between mono and mono-sgen, since the code in mini/ doesn't contain
# compile time dependencies on boehm/sgen.
#
add_library(mini-static STATIC ${COMMON_SOURCES} ${LLVM_SOURCES} ${LLVM_RUNTIME_SOURCES} ${ARCH_SOURCES} ${OS_SOURCES})
target_link_libraries(mini-static monoutils)

# add_library() needs at least one source file
add_custom_command(
  OUTPUT empty.c
  COMMAND ${CMAKE_COMMAND} -E touch empty.c
)

add_library(monoboehm SHARED empty.c)
target_link_libraries(monoboehm mini monoruntime ${LLVMMONOF} ${LIBMONOLDFLAGS})
add_library(monoboehm-static STATIC empty.c)
target_link_libraries(monoboehm-static mini monoruntime ${LLVMMONOF})
add_library(monosgen SHARED empty.c)
target_link_libraries(monosgen mini monoruntimesgen ${LLVMMONOF} ${LIBMONOLDFLAGS})
add_library(monosgen-static STATIC empty.c)
target_link_libraries(monosgen-static mini monoruntimesgen ${LLVMMONOF})

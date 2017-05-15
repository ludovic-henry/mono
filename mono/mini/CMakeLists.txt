include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBGC_INCLUDE_DIRECTORIES}
  ${GLIB_INCLUDE_DIRECTORIES}
)

if(HOST_WIN32)
  if(NOT MSVC)
    set(LIBMONOLDFLAGS -Wl,--no-undefined -Wl,--kill-at ${MONOLDFLAGS})
  endif()
elseif(PLATFORM_ANDROID)
  set(LIBMONOLDFLAGS -avoid-version ${MONOLDFLAGS})
elseif(NOT PLATFORM_DARWIN)
  set(LIBMONOLDFLAGS ${MONOLDFLAGS} "-version-info 1:0:0")
endif()

if(LOADED_LLVM)
  add_library(mono-llvm mini-llvm.c mini-llvm-cpp.cpp llvm-jit.cpp)
  set_property(TARGET mono-llvm PROPERTY FOLDER "Libraries")
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
  add_custom_command(
    OUTPUT buildver-boehm.h
    COMMAND ${CMAKE_COMMAND} -DFILE="${CMAKE_CURRENT_BINARY_DIR}/buildver-boehm.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVersionHeader.cmake"
    DEPENDS mini monoruntimeboehm monoboehm-static
  )
endif()

if(SUPPORT_SGEN)
  add_custom_command(
    OUTPUT buildver-sgen.h
    COMMAND ${CMAKE_COMMAND} -DFILE="${CMAKE_CURRENT_BINARY_DIR}/buildver-sgen.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVersionHeader.cmake"
    DEPENDS mini monoruntimesgen monosgen-static
  )
endif()

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
  add_executable(mono-boehm ${MONO_BOEHM_SOURCES} buildver-boehm.h)
  set_property(TARGET mono-boehm PROPERTY FOLDER "Executables")
  target_link_libraries(mono-boehm ${MONO_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
endif()

if(SUPPORT_SGEN)
  add_executable(mono-sgen ${MONO_SGEN_SOURCES} buildver-sgen.h)
  set_property(TARGET mono-sgen PROPERTY FOLDER "Executables")
  target_link_libraries(mono-sgen ${MONO_SGEN_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
endif()

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
    add_executable(monow WIN32 ${MONO_BOEHM_SOURCES} buildver-boehm.h)
    set_property(TARGET monow PROPERTY FOLDER "Executables")
    target_link_libraries(monow ${MONO_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
  elseif(SUPPORT_SGEN)
    add_executable(monow WIN32 ${MONO_SGEN_SOURCES} buildver-sgen.h)
    set_property(TARGET monow PROPERTY FOLDER "Executables")
    target_link_libraries(monow ${MONO_SGEN_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
  endif()
  if(TARGET monow AND MSVC)
    target_link_libraries(monow "-entry:mainCRTStartup")
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
set_property(TARGET genmdesc PROPERTY FOLDER "Tools")
target_link_libraries(genmdesc monoutils-static)

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
  mini-x86-gsharedvt.c
  tramp-x86-gsharedvt.c
)

set(AMD64_SOURCES
  mini-amd64.c
  mini-amd64.h
  exceptions-amd64.c
  tramp-amd64.c
  mini-amd64-gsharedvt.c
  mini-amd64-gsharedvt.h
  tramp-amd64-gsharedvt.c
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
  mini-arm-gsharedvt.c
  tramp-arm-gsharedvt.c
)

set(ARM64_SOURCES
  mini-arm64.c
  mini-arm64.h
  exceptions-arm64.c
  tramp-arm64.c
  mini-arm64-gsharedvt.c
  tramp-arm64-gsharedvt.c
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
  cfgdump.h
  cfgdump.c
  decompose.c
  mini.h
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
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
  mini-exceptions-native-unwinder.c
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
  llvm-jit.h
  alias-analysis.c
  mini-cross-helpers.c
  arch-stubs.c
  llvm-runtime.h
)

if(X86)
  set(ARCH_SOURCES ${X86_SOURCES})
  set(GENMDESC_ARCH x86)
  set(ARCH_DEFINE __i386__)
endif()

if(AMD64)
  set(ARCH_SOURCES ${AMD64_SOURCES})
  set(GENMDESC_ARCH amd64)
  set(ARCH_DEFINE __x86_64__)
  set(ARCH_FULLAOT_EXCLUDE "--exclude DYNCALL")
endif()

if(POWERPC)
  set(ARCH_SOURCES ${PPC_SOURCES})
  set(GENMDESC_ARCH ppc)
  set(ARCH_DEFINE __ppc__)
  set(GENMDESC_SYMBOL_NAME ppcg4)
endif()

if(POWERPC64)
  set(ARCH_SOURCES ${PPC_SOURCES})
  set(GENMDESC_ARCH ppc64)
  set(ARCH_DEFINE __ppc64__)
  set(GENMDESC_SYMBOL_NAME "${GENMDESC_ARCH}_cpu_desc")
endif()

if(MIPS)
  set(ARCH_SOURCES ${MIPS_SOURCES})
  set(GENMDESC_ARCH mips)
  set(ARCH_DEFINE __mips__)
endif()

if(ARM)
  # pick up arm_dpimacros.h
  include_directories(../arch/arm)
  set(ARCH_SOURCES ${ARM_SOURCES})
  set(GENMDESC_ARCH arm)
  set(ARCH_DEFINE __arm__)
  set(GENMDESC_SYMBOL_NAME "${GENMDESC_ARCH}_cpu_desc")
endif()

if(ARM64)
  set(ARCH_SOURCES ${ARM64_SOURCES})
  set(GENMDESC_ARCH arm64)
  set(ARCH_DEFINE __aarch64__)
  set(GENMDESC_SYMBOL_NAME "${GENMDESC_ARCH}_cpu_desc")
endif()

if(SPARC OR SPARC64)
  set(ARCH_SOURCES ${SPARC_SOURCES})
  set(GENMDESC_ARCH sparc)
  set(ARCH_DEFINE __sparc__)
endif()

if(S390X)
  set(ARCH_SOURCES ${S390X_SOURCES})
  set(GENMDESC_ARCH s390x)
  set(ARCH_DEFINE __s390__)
  set(GENMDESC_SYMBOL_NAME "${GENMDESC_ARCH}_cpu_desc")
endif()

if(IA64)
  set(ARCH_SOURCES ${IA64_SOURCES})
  set(GENMDESC_ARCH ia64)
  set(ARCH_DEFINE __ia64__)
endif()
if(NOT GENMDESC_HEADER_FILE)
  set(GENMDESC_HEADER_FILE "cpu-${GENMDESC_ARCH}.h")
endif()
if(NOT GENMDESC_SYMBOL_NAME)
  set(GENMDESC_SYMBOL_NAME "${GENMDESC_ARCH}_desc")
endif()
if(NOT GENMDESC_MD_FILE)
  set(GENMDESC_MD_FILE "cpu-${GENMDESC_ARCH}.md")
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

# Generate version.h
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
  add_custom_command(
    OUTPUT ${CMAKE_BIN_DIR}/version.h
    COMMAND echo "#define FULL_VERSION_BRANCH \"`git symbolic-ref --short HEAD`\"" > ${CMAKE_BIN_DIR}/version.h
    COMMAND echo "#define FULL_VERSION_COMMIT \"`git rev-parse --short HEAD`\"" >> ${CMAKE_BIN_DIR}/version.h
    COMMAND echo "#define FULL_VERSION FULL_VERSION_BRANCH ## \"/\" ## FULL_VERSION_COMMIT" >> ${CMAKE_BIN_DIR}/version.h
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM
  )
else()
  add_custom_command(
    OUTPUT ${CMAKE_BIN_DIR}/version.h
    COMMAND echo "#define FULL_VERSION \"tarball\"" > ${CMAKE_BIN_DIR}/version.h
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM
  )
endif()

set(GENMDESC_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${GENMDESC_HEADER_FILE}")
set(GENMDESC_MD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${GENMDESC_MD_FILE}")

# we don't always use the perl impl because it's an additional
# build dependency for the poor windows users
# ${ARCH_DEFINE} is the preprocessor symbol that enables all the opcodes
# for the specific platform in mini-ops.h
if(CMAKE_CROSSCOMPILE)
  add_custom_command(
    OUTPUT ${GENMDESC_HEADER_FILE}
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/genmdesc.pl ${ARCH_DEFINE} ${CMAKE_CURRENT_SOURCE_DIR} ${GENMDESC_HEADER_PATH} ${GENMDESC_SYMBOL_NAME} ${GENMDESC_MD_PATH}
    DEPENDS ${GENMDESC_MD_FILE}
    VERBATIM
  )
else()
  # Make sure we use paths that genmdesc will understand
  file(TO_NATIVE_PATH ${GENMDESC_HEADER_PATH} GENMDESC_HEADER_PATH)
  file(TO_NATIVE_PATH ${GENMDESC_MD_PATH} GENMDESC_MD_PATH)

  add_custom_command(
    OUTPUT ${GENMDESC_HEADER_FILE}
    COMMAND $<TARGET_FILE:genmdesc> ${GENMDESC_HEADER_PATH} ${GENMDESC_SYMBOL_NAME} ${GENMDESC_MD_PATH}
    DEPENDS genmdesc ${GENMDESC_MD_FILE}
    VERBATIM
  )
endif()

add_library(mini_objects OBJECT ${COMMON_SOURCES} ${LLVM_SOURCES} ${LLVM_RUNTIME_SOURCES} ${ARCH_SOURCES} ${OS_SOURCES} ${GENMDESC_HEADER_FILE})
set_property(TARGET mini_objects PROPERTY PROJECT_LABEL "mini (objects)")
set_property(TARGET mini_objects PROPERTY FOLDER "Libraries")
add_library(mini INTERFACE)
target_sources(mini INTERFACE $<TARGET_OBJECTS:mini_objects>)
target_link_object_libraries(mini monoutils)
target_link_libraries(mini INTERFACE ${LIBS})

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty.c" "")
endif()

if(SUPPORT_BOEHM)
  if(SHARED_MONO)
    add_library(monoboehm SHARED "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
    set_property(TARGET monoboehm PROPERTY FOLDER "Libraries")
    set_property(TARGET monoboehm PROPERTY PROJECT_LABEL "monoboehm (dynamic)")
    target_link_libraries(monoboehm mini monoruntimeboehm ${LLVMMONOF} ${LIBMONOLDFLAGS})
  endif()
  add_library(monoboehm-static STATIC "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
  set_property(TARGET monoboehm-static PROPERTY FOLDER "Libraries")
  set_property(TARGET monoboehm-static PROPERTY PROJECT_LABEL "monoboehm (static)")
  target_link_libraries(monoboehm-static mini monoruntimeboehm ${LLVMMONOF})
endif()
if(SUPPORT_SGEN)
  if(SHARED_MONO)
    add_library(monosgen SHARED "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
    set_property(TARGET monosgen PROPERTY FOLDER "Libraries")
    set_property(TARGET monosgen PROPERTY PROJECT_LABEL "monosgen (dynamic)")
    target_link_libraries(monosgen mini monoruntimesgen ${LLVMMONOF} ${LIBMONOLDFLAGS})
  endif()
  add_library(monosgen-static STATIC "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
  set_property(TARGET monosgen-static PROPERTY FOLDER "Libraries")
  set_property(TARGET monosgen-static PROPERTY PROJECT_LABEL "monosgen (static)")
  target_link_libraries(monosgen-static mini monoruntimesgen ${LLVMMONOF})
endif()


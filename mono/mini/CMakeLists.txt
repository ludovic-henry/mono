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
  target_link_libraries(mono-boehm ${MONO_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
endif()

if(SUPPORT_SGEN)
  add_executable(mono-sgen ${MONO_SGEN_SOURCES} buildver-sgen.h)
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
    target_link_libraries(monow ${MONO_LIB} ${MONOBINLDFLAGS} ${MONOBIN_PLATFORM_LDFLAGS} ${LLVMMONOF})
  elseif(SUPPORT_SGEN)
    add_executable(monow WIN32 ${MONO_SGEN_SOURCES} buildver-sgen.h)
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

#AM_CFLAGS = \
#-I$(top_srcdir) 	\
#$(GLIB_CFLAGS)		\
#$(LLVM_CFLAGS)		\
#$(PLATFORM_CFLAGS) $(ARCH_CFLAGS) $(SHARED_CFLAGS)
#AM_CXXFLAGS = $(LLVM_CXXFLAGS) $(GLIB_CFLAGS)

add_library(mini_objects OBJECT ${COMMON_SOURCES} ${LLVM_SOURCES} ${LLVM_RUNTIME_SOURCES} ${ARCH_SOURCES} ${OS_SOURCES} ${GENMDESC_HEADER_FILE})
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
    target_link_libraries(monoboehm mini monoruntimeboehm ${LLVMMONOF} ${LIBMONOLDFLAGS})
  endif()
  add_library(monoboehm-static STATIC "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
  target_link_libraries(monoboehm-static mini monoruntimeboehm ${LLVMMONOF})
endif()
if(SUPPORT_SGEN)
  if(SHARED_MONO)
    add_library(monosgen SHARED "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
    target_link_libraries(monosgen mini monoruntimesgen ${LLVMMONOF} ${LIBMONOLDFLAGS})
  endif()
  add_library(monosgen-static STATIC "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
  target_link_libraries(monosgen-static mini monoruntimesgen ${LLVMMONOF})
endif()

if(NACL_CODEGEN)
  set(GENMDESC_OPTS --nacl)
endif()

# we don't always use the perl impl because it's an additional
# build dependency for the poor windows users
# ${ARCH_DEFINE} is the preprocessor symbol that enables all the opcodes
# for the specific platform in mini-ops.h
if(CROSS_COMPILING)
  set(GENMDESC_PRG perl ${CMAKE_CURRENT_SOURCE_DIR}/genmdesc.pl ${ARCH_DEFINE} ${CMAKE_CURRENT_SOURCE_DIR} ${GENMDESC_OPTS})
elseif(NACL_CODEGEN)
  set(GENMDESC_PRG perl ${CMAKE_CURRENT_SOURCE_DIR}/genmdesc.pl ${ARCH_DEFINE} ${CMAKE_CURRENT_SOURCE_DIR} $(GENMDESC_OPTS))
else()
  set(GENMDESC_PRG $<TARGET_FILE:genmdesc> ${GENMDESC_OPTS})
endif()

set(GENMDESC_MD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${GENMDESC_MD_FILE}")
set(GENMDESC_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${GENMDESC_HEADER_FILE}")
if(CYGWIN AND NOT GENMDESC_PRG STREQUAL "perl")
  # Run paths through cygpath to get proper windows paths that genmdesc will understand
  execute_process(COMMAND cygpath.exe -m "${GENMDESC_MD_PATH}" OUTPUT_VARIABLE GENMDESC_MD_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND cygpath.exe -m "${GENMDESC_HEADER_PATH}" OUTPUT_VARIABLE GENMDESC_HEADER_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "GENMDESC_MD_PATH = ${GENMDESC_MD_PATH}")
  message(STATUS "GENMDESC_HEADER_PATH = ${GENMDESC_MD_PATH}")
endif()
add_custom_command(
  OUTPUT ${GENMDESC_HEADER_FILE}
  COMMAND ${GENMDESC_PRG} "${GENMDESC_HEADER_PATH}" ${GENMDESC_SYMBOL_NAME} "${GENMDESC_MD_PATH}"
  DEPENDS genmdesc ${GENMDESC_MD_FILE}
)

# Generate version.h
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE EXITCODE
    OUTPUT_VARIABLE GIT_COMMIT)
  if(EXITCODE EQUAL 0)
    string(STRIP ${GIT_COMMIT} GIT_COMMIT)
    execute_process(
      COMMAND git symbolic-ref --short HEAD
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE SUCCESS
      OUTPUT_VARIABLE GIT_BRANCH)
    if(EXITCODE EQUAL 0)
      # We're on a branch
      string(STRIP ${GIT_BRANCH} GIT_BRANCH)
      file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version.h" "#define FULL_VERSION \"${GIT_BRANCH}/${GIT_COMMIT}\"")
    else()
      # We're in detached state
      file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version.h" "#define FULL_VERSION \"explicit/${GIT_COMMIT}\"")
    endif()
  else()
    message(FATAL_ERROR "Failed to determine current git commit. git exited with ${EXITCODE}")
  endif()
else()
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version.h" "#define FULL_VERSION \"tarball\"")
endif()

# Tests
set(TESTS_FOLDER "Tests/mini")
set(JIT_TESTS_FOLDER "${TESTS_FOLDER}/JIT")
set(ASSEMBLIES_TEST_FOLDER "${TESTS_FOLDER}/Assemblies")
set(TOOLS_TEST_FOLDER "${TESTS_FOLDER}/Tools")

set(TEST_SOURCES
  basic-calls.cs
  basic-long.cs
  objects.cs
  arrays.cs
  basic-float.cs
  basic-math.cs
  basic.cs
  exceptions.cs
  devirtualization.cs
  # TODO: We need ilasm to compile this
  # iltests.il
)

set(CSFLAGS "-unsafe" "-nowarn:0219,0169,0414,0649")
cs_add_assembly(TestDriver.dll SOURCES TestDriver.cs FLAGS ${CSFLAGS} FOLDER ${ASSEMBLIES_TEST_FOLDER})
foreach(TEST_SOURCE ${TEST_SOURCES})
  string(REGEX REPLACE "(\\.cs|\\.il)$" ".exe" TEST_EXE ${TEST_SOURCE})
  cs_add_assembly(${TEST_EXE} SOURCES ${TEST_SOURCE} FLAGS ${CSFLAGS} LIBRARIES TestDriver.dll FOLDER ${ASSEMBLIES_TEST_FOLDER})
  set(REGTESTS ${REGTESTS} ${TEST_EXE})
endforeach()

# TODO: We need Mono.Simd.dll to compile this
# cs_add_assembly(basic-simd.exe SOURCES basic-simd.cs FLAGS ${CSFLAGS} LIBRARIES TestDriver.dll Mono.Simd.dll)
# set(REGTESTS ${REGTESTS} basic-simd.exe)

if(NACL_CODEGEN)
  # TODO: We need Mono.Simd.dll to compile this
  # cs_add_assembly(nacl.exe SOURCES nacl.cs FLAGS ${CSFLAGS} LIBRARIES TestDriver.dll Mono.Simd.dll)
  # set(REGTESTS ${REGTESTS} nacl.exe)
endif()

# TODO: We need ilasm to compile this
# cs_add_assembly(generics-variant-types.dll SOURCES generics-variant-types.il)
# cs_add_assembly(generics.exe SOURCES generics.cs FLAGS ${CSFLAGS} LIBRARIES TestDriver.dll generics-variant-types.dll System.Core.dll)
# set(REGTESTS ${REGTESTS} generics.exe)

# Join test assemblies with \0 as separator
list(SORT REGTESTS)
list(REVERSE REGTESTS)
foreach(REGTEST ${REGTESTS})
  set(TEST_ASSEMBLIES "${REGTEST}\\0${TEST_ASSEMBLIES}")
endforeach()
set(TEST_ASSEMBLIES "${TEST_ASSEMBLIES}\\0")

# gen-jit-regression-tests.exe generates Google Test methods by reflecting over the test methods in the test assemblies.
cs_add_assembly(gen-jit-regression-tests.exe SOURCES gen-jit-regression-tests.cs FOLDER ${TOOLS_TEST_FOLDER})

set(ALL_OPTS
   0
   peephole
   branch
   cfold
   fcmov
   alias_analysis
)
if(NOT DISABLE_SIMD)
  set(ALL_OPTS ${ALL_OPTS}
    simd
    sse2
    simd+sse2
  )
endif()
set(ALL_OPTS ${ALL_OPTS}
  branch+peephole+intrins
  branch+peephole+intrins+alias_analysis
  branch+peephole+linears
  branch+peephole+linears+copyprop
  branch+peephole+linears+cfold
  branch+peephole+linears+copyprop+consprop+deadce
  branch+peephole+linears+copyprop+consprop+deadce+alias_analysis
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+tailc
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+ssa
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+exception
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+exception+cmov
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+exception+abcrem
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+abcrem
  branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+abcrem+shared
  branch+peephole+linears+consprop+copyprop+deadce+loop+inline+intrins+exception+gshared+cmov+simd+alias_analysis+aot+cfold
)

to_native_path(${MONO_CFG_DIR} CONFIG_DIR)
to_native_path(${MONO_PATH} ASSEMBLY_DIR)
string(REPLACE "\\" "\\\\" CONFIG_DIR ${CONFIG_DIR})
string(REPLACE "\\" "\\\\" ASSEMBLY_DIR ${ASSEMBLY_DIR})

set(N 1)
foreach(OPTS ${ALL_OPTS})
  if(HOST_WINDOWS)
    # Windows has some ridiculous limits on path length
    if(N LESS 10)
      set(TEST_NAME "jit-test-0${N}")
    else()
      set(TEST_NAME "jit-test-${N}")
    endif()
    math(EXPR N "${N} + 1")
  else()
    set(TEST_NAME "jit-test-${OPTS}")
    if(OPTS STREQUAL "0")
      set(TEST_NAME "jit-test-noopts")
    endif()
  endif()

  if(OPTS STREQUAL "0")
    set(COMPILE_OPTS "0")
  else()
    string(REPLACE "+" "|MONO_OPT_" COMPILE_OPTS ${OPTS})
    string(TOUPPER ${COMPILE_OPTS} COMPILE_OPTS)
    set(COMPILE_OPTS "MONO_OPT_${COMPILE_OPTS}")
  endif()

  set(TEST_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${TEST_NAME}.cpp)
  set(GEN_JIT_REGRESSION_TEST_EXE ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/gen-jit-regression-tests.exe)
  to_native_path(${TEST_OUTPUT} TEST_OUTPUT_NATIVE)
  to_native_path(${GEN_JIT_REGRESSION_TEST_EXE} GEN_JIT_REGRESSION_TEST_EXE_NATIVE)

  add_custom_command(OUTPUT ${TEST_OUTPUT}
    COMMAND ${MONO_WRAPPER} ${GEN_JIT_REGRESSION_TEST_EXE_NATIVE} ${TEST_OUTPUT_NATIVE} ${OPTS} ${REGTESTS}
    DEPENDS mono-wrapper gen-jit-regression-tests.exe gen-jit-regression-tests.cs ${REGTESTS}
    VERBATIM
  )
  add_executable(${TEST_NAME} jit-regression-tests.cpp jit-regression-tests.h ${TEST_OUTPUT})
  set_property(TARGET ${TEST_NAME} PROPERTY FOLDER ${JIT_TESTS_FOLDER})
  target_link_libraries(${TEST_NAME} gtest monosgen-static)
  if(CYGWIN)
    target_link_libraries(${TEST_NAME} -static)
  endif()
  target_compile_definitions(${TEST_NAME} PRIVATE -DCONFIG_DIR="${CONFIG_DIR}" -DASSEMBLY_DIR="${ASSEMBLY_DIR}" -DCOMPILE_OPTS=${COMPILE_OPTS})
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  add_dependencies(${TEST_NAME} gen-jit-regression-tests.exe ${REGTESTS})
endforeach()

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# STRESS_TESTS_SRC=
#   abort-stress-1.cs
#   abort-stress-2.cs
#   abort-stress-3.cs
#   domain-stress.cs
#   gchandle-stress.cs
#   monitor-stress.cs
#   thread-stress.cs
#   gc-stress.cs
#   gc-copy-stress.cs
#   gc-graystack-stress.cs
#   exit-stress.cs
#   process-stress.cs
#   assembly-load-stress.cs

# Disabled until ?mcs is fixed
# bug-331958.cs
set(BASE_TEST_CS_SRC=
  bug-2907.cs
  array-init.cs
  arraylist.cs
  assembly-load-remap.cs
  assemblyresolve_event.cs
  assemblyresolve_event3.cs
  assemblyresolve_event4.cs
  checked.cs
  char-isnumber.cs
  create-instance.cs
  field-layout.cs
  pack-layout.cs
  pack-bug.cs
  hash-table.cs
  test-ops.cs
  obj.cs
  test-dup-mp.cs
  string.cs
  stringbuilder.cs
  switch.cs
  outparm.cs
  delegate.cs
  bitconverter.cs
  exception.cs
  exception2.cs
  exception3.cs
  exception4.cs
  exception5.cs
  exception6.cs
  exception7.cs
  exception8.cs
  exception10.cs
  exception11.cs
  exception12.cs
  exception13.cs
  exception14.cs
  exception15.cs
  exception16.cs
  exception17.cs
  exception18.cs
  typeload-unaligned.cs
  struct.cs
  valuetype-gettype.cs
  typeof-ptr.cs
  static-constructor.cs
  pinvoke.cs
  pinvoke2.cs
  pinvoke3.cs
  pinvoke11.cs
  pinvoke13.cs
  pinvoke17.cs
  invoke.cs
  invoke2.cs
  runtime-invoke.cs
  invoke-string-ctors.cs
  reinit.cs
  box.cs
  array.cs
  enum.cs
  enum2.cs
  enum-intrins.cs
  property.cs
  enumcast.cs
  assignable-tests.cs
  array-cast.cs
  array-subtype-attr.cs
  cattr-compile.cs
  cattr-field.cs
  cattr-object.cs
  custom-attr.cs
  double-cast.cs
  newobj-valuetype.cs
  arraylist-clone.cs
  setenv.cs
  vtype.cs
  isvaluetype.cs
  iface6.cs
  iface7.cs
  ipaddress.cs
  array-vt.cs
  interface1.cs
  reflection-enum.cs
  reflection-prop.cs
  reflection4.cs
  reflection5.cs
  reflection-const-field.cs
  many-locals.cs
  string-compare.cs
  test-prime.cs
  test-tls.cs
  params.cs
  reflection.cs
  interface.cs
  iface.cs
  iface2.cs
  iface3.cs
  iface4.cs
  iface-large.cs
  virtual-method.cs
  intptrcast.cs
  indexer.cs
  stream.cs
  console.cs
  shift.cs
  jit-int.cs
  jit-uint.cs
  jit-long.cs
  long.cs
  jit-ulong.cs
  jit-float.cs
  pop.cs
  time.cs
  appdomain.cs
  appdomain1.cs
  appdomain2.cs
  appdomain-client.cs
  appdomain-unload.cs
  appdomain-async-invoke.cs
  loader.cs
  pointer.cs
  hashcode.cs
  delegate1.cs
  delegate2.cs
  delegate3.cs
  delegate5.cs
  delegate6.cs
  delegate7.cs
  delegate8.cs
  delegate9.cs
  delegate10.cs
  delegate11.cs
  delegate12.cs
  delegate13.cs
  remoting1.cs
  remoting2.cs
  remoting3.cs
  remoting4.cs
  remoting5.cs
  largeexp.cs
  largeexp2.cs
  marshalbyref1.cs
  static-ctor.cs
  inctest.cs
  bound.cs
  array-invoke.cs
  test-arr.cs
  decimal.cs
  decimal-array.cs
  marshal.cs
  marshal1.cs
  marshal2.cs
  marshal3.cs
  marshal5.cs
  marshal6.cs
  marshal7.cs
  marshal8.cs
  marshal9.cs
  marshalbool.cs
  marshal-valuetypes.cs
  test-byval-in-struct.cs
  thread.cs
  thread5.cs
  thread6.cs
  thread-static.cs
  thread-static-init.cs
  context-static.cs
  float-pop.cs
  interfacecast.cs
  array3.cs
  classinit.cs
  classinit2.cs
  classinit3.cs
  synchronized.cs
  async_read.cs
  threadpool.cs
  threadpool1.cs
  threadpool-exceptions1.cs
  threadpool-exceptions2.cs
  threadpool-exceptions3.cs
  threadpool-exceptions4.cs
  threadpool-exceptions5.cs
  threadpool-exceptions6.cs
  threadpool-exceptions7.cs
  base-definition.cs
  bug-27420.cs
  bug-47295.cs
  bug-46781.cs
  bug-48015.cs
  bug-42136.cs
  bug-59286.cs
  bug-70561.cs
  bug-78311.cs
  bug-78653.cs
  bug-78656.cs
  bug-77127.cs
  bug-323114.cs
  bug-Xamarin-5278.cs
  interlocked.cs
  cross-domain.cs
  appdomain-exit.cs
  delegate-async-exit.cs
  delegate-delegate-exit.cs
  delegate-exit.cs
  finalizer-abort.cs
  finalizer-exception.cs
  finalizer-exit.cs
  finalizer-thread.cs
  main-exit.cs
  main-returns-abort-resetabort.cs
  main-returns-background-abort-resetabort.cs
  main-returns-background-resetabort.cs
  main-returns-background.cs
  main-returns-background-change.cs
  main-returns.cs
  subthread-exit.cs
  desweak.cs
  cominterop.cs
  exists.cs
  handleref.cs
  transparentproxy.cs
  dbnull-missing.cs
  test-type-ctor.cs
  soft-float-tests.cs
  thread-exit.cs
  finalize-parent.cs
  assemblyresolve_event2.2.cs
  interlocked-2.2.cs
  pinvoke-2.2.cs
  bug-78431.2.cs
  bug-79684.2.cs
  catch-generics.2.cs
  event-get.2.cs
  safehandle.2.cs
  stackframes-async.2.cs
  module-cctor-loader.2.cs
  generics-invoke-byref.2.cs
  generic-signature-compare.2.cs
  generics-sharing.2.cs
  shared-generic-methods.2.cs
  shared-generic-synchronized.2.cs
  generic-inlining.2.cs
  generic-initobj.2.cs
  generic-delegate.2.cs
  generic-sizeof.2.cs
  generic-virtual.2.cs
  generic-interface-methods.2.cs
  generic-array-type.2.cs
  generic-method-patching.2.cs
  generic-static-methods.2.cs
  generic-null-call.2.cs
  generic-special.2.cs
  generic-exceptions.2.cs
  generic-virtual2.2.cs
  generic-valuetype-interface.2.cs
  generic-getgenericarguments.2.cs
  generic-type-builder.2.cs
  generic-synchronized.2.cs
  generic-delegate-ctor.2.cs
  generic-array-iface-set.2.cs
  generic-typedef.2.cs
  generic-marshalbyref.2.cs
  generic-xdomain.2.cs
  dynamic-generic-size.cs
  bug-431413.2.cs
  bug-459285.2.cs
  generic-virtual-invoke.2.cs
  bug-461198.2.cs
  generic-sealed-virtual.2.cs
  generic-system-arrays.2.cs
  generic-stack-traces.2.cs
  generic-stack-traces2.2.cs
  bug-472600.2.cs
  recursive-generics.2.cs
  bug-473482.2.cs
  bug-473999.2.cs
  bug-479763.2.cs
  bug-616463.cs
  bug-80392.2.cs
  dynamic-method-access.2.cs
  dynamic-method-finalize.2.cs
  dynamic-method-stack-traces.cs
  bug-82194.2.cs
  anonarray.2.cs
  ienumerator-interfaces.2.cs
  array-enumerator-ifaces.2.cs
  generic_type_definition_encoding.2.cs
  generic_type_definition.2.cs
  bug-333798.2.cs
  bug-333798-tb.2.cs
  bug-335131.2.cs
  bug-322722_patch_bx.2.cs
  bug-348522.2.cs
  bug-340662_bug.cs
  bug-322722_dyn_method_throw.2.cs
  bug-389886-2.cs
  bug-325283.2.cs
  thunks.cs
  winx64structs.cs
  bug-349190.2.cs
  nullable_boxing.2.cs
  valuetype-equals.cs
  custom-modifiers.2.cs
  bug-382986.cs
  test-inline-call-stack.cs
  bug-324535.cs
  modules.cs
  bug-81673.cs
  bug-36848.cs
  bug-81691.cs
  bug-80307.cs
  bug-415577.cs
  filter-stack.cs
  vararg2.cs
  bug-389886-sre-generic-interface-instances.cs
  bug-461867.cs
  bug-461941.cs
  bug-461261.cs
  bug-400716.cs
  bug-462592.cs
  bug-459094.cs
  generic-unloading.2.cs
  generic-unloading-sub.2.cs
  bug-467456.cs
  appdomain-unload-callback.cs
  bug-508538.cs
  bug-472692.2.cs
  gchandles.cs
  interlocked-3.cs
  interlocked-4.2.cs
  appdomain-thread-abort.cs
  xdomain-threads.cs
  w32message.cs
  bug-544446.cs
  gc-altstack.cs
  large-gc-bitmap.cs
  bug-561239.cs
  bug-562150.cs
  bug-575941.cs
  bug-599469.cs
  bug-389886-3.cs
  monitor.cs
  monitor-resurrection.cs
  monitor-wait-abort.cs
  monitor-abort.cs
  dynamic-method-resurrection.cs
  bug-666008.cs
  bug-685908.cs
  sgen-long-vtype.cs
  delegate-invoke.cs
  bug-696593.cs
  bug-705140.cs
  bug-1147.cs
  mono-path.cs
  bug-bxc-795.cs
  bug-3903.cs
  async-with-cb-throws.cs
  appdomain-unload-doesnot-raise-pending-events.cs
  bug-6148.cs
  assembly_append_ordering.cs
  bug-10127.cs
  bug-18026.cs
  allow-synchronous-major.cs
  unload-appdomain-on-shutdown.cs
  block_guard_restore_aligment_on_exit.cs
  thread_static_gc_layout.cs
  sleep.cs
  bug-27147.cs
  bug-30085.cs
  bug-17537.cs
  pinvoke_ppcc.cs
  pinvoke_ppcs.cs
  pinvoke_ppci.cs
  pinvoke_ppcf.cs
  pinvoke_ppcd.cs
  bug-29585.cs
)

set(TEST_CS_SRC_DIST
  ${BASE_TEST_CS_SRC}
  async-exc-compilation.cs
  filter-stack.cs
  finally_guard.cs
  finally_block_ending_in_dead_bb.cs
)

# TODO: Generate these test sources
set(TEST_CS_SRC_GEN
#  runtime-invoke.gen.cs
#  imt_big_iface_test.cs
)

if(AMD64)
  set(TEST_CS_SRC ${BASE_TEST_CS_SRC} ${TEST_CS_SRC_GEN} async-exc-compilation.cs finally_guard.cs finally_block_ending_in_dead_bb.cs)
# #651684
  set(PLATFORM_DISABLED_TESTS finally_guard.exe)
elseif(X86)
  set(TEST_CS_SRC ${BASE_TEST_CS_SRC} ${TEST_CS_SRC_GEN} async-exc-compilation.cs finally_guard.cs finally_block_ending_in_dead_bb.cs)
else()
  set(TEST_CS_SRC ${BASE_TEST_CS_SRC} ${TEST_CS_SRC_GEN})
endif()

if(IA64)
  set(TEST_CS_SRC ${TEST_CS_SRC} async-exc-compilation.cs filter-stack.cs)
  # bug #319249
  set(PLATFORM_DISABLED_TESTS exception17.exe)
  set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} winx64structs.exe)
endif()

if(X86)
  if(HOST_WIN32)
    set(PLATFORM_DISABLED_TESTS async-exc-compilation.exe finally_guard.exe finally_block_ending_in_dead_bb.exe
        bug-18026.exe monitor.exe threadpool-exceptions5.exe)
  endif()
endif()

if(POWERPC)
  # bug #71274
  set(PLATFORM_DISABLED_TESTS finalizer-abort.exe finalizer-exception.exe finalizer-exit.exe)
endif()

if(POWERPC64)
  # FIXME: These tests hang/fail for unknown reasons
  set(PLATFORM_DISABLED_TESTS monitor.exe threadpool-exceptions5.exe)
endif()

if(ARM)
  set(PLATFORM_DISABLED_TESTS filter-stack.exe)
endif()

if(MIPS)
  # monitor.exe is racy
  set(PLATFORM_DISABLED_TESTS filter-stack.exe monitor.exe)
endif()

if(S390X)
  set(PLATFORM_DISABLED_TESTS dynamic-method-resurrection.exe)
  #PLATFORM_DISABLED_TESTS=dynamic-method-resurrection.exe exception17.exe
endif()

if(NACL_CODEGEN)
  # Tests that use Thread.Abort()
  set(PLATFORM_DISABLED_TESTS abort-stress-1.exe
                           abort-stress-2.exe
                           abort-stress-3.exe
                           appdomain-thread-abort.exe
                           async-exc-compilation.exe
                           bug-561239.exe
                           bug-70561.exe
                           finalizer-abort.exe
                           finally_guard.exe
                           finally_block_ending_in_dead_bb.exe
                           main-returns-abort-resetabort.exe
                           main-returns-background-abort-resetabort.exe
                           thread6.exe
                           threadpool-exceptions5.exe
                           threadpool-exceptions6.exe)

  # Tests that rely on AppDomain.Unload
  set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} appdomain-async-invoke.exe
                            appdomain-exit.exe
                            appdomain-unload-callback.exe
                            appdomain-unload.exe
                            domain-stress.exe
                            generic-unloading.2.exe
                            monitor.exe
                            remoting4.exe
                            threadpool-exceptions7.exe
                            xdomain-threads.exe)

  # pinvoke2 attaches a thread to the runtime, but
  # doesn't 'unattach' it and it hangs in GC on exit
  set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} pinvoke2.exe)

  # Tests that currently hang waiting for non-main threads
  # to exit in NaCl, need to investigate.  Most are AppDomain
  # creation and Delegate tests.
  set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} appdomain1.exe
                            delegate9.exe
                            marshal-valuetypes.exe
                            cross-domain.exe
                            stackframes-async.2.exe
                            generic-marshalbyref.2.exe
                            generic-xdomain.2.exe
                            bug-415577.exe)

  # Tests that fail trying to write files (appdomain create mostly)
  set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS}  bug-335131.2.exe
                            bug-349190.2.exe
                            bug-80307.exe
                            bug-462592.exe)

  # FIXME: don't know why delegate2.exe fails, it shouldn't
  set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} delegate2.exe)

  # These tests newly fail with the latest revision. pinvoke3 fails because
  # of a thread attach, the others have not been investigated.  TODO revisit.
  set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} pinvoke3.exe
                            async_read.exe
                            async-with-cb-throws.exe
                            appdomain-unload-doesnot-raise-pending-events.exe
                            gsharing-valuetype-layout.exe)

  if(X86)
    # FIXME: There are problems with async callbacks and results on NaCl 32-bit
    set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} delegate1.exe
                              delegate3.exe
                              delegate5.exe
                              delegate8.exe
                              threadpool.exe
                              threadpool1.exe
                              threadpool-exceptions3.exe
                              bug-323114.exe
                              delegate-exit.exe
                              bug-80392.2.exe)

    # FIXME: These tests hang/fail for unknown reasons, deal with exiting
    set(PLATFORM_DISABLED_TESTS ${PLATFORM_DISABLED_TESTS} main-returns-background-resetabort.exe
                              main-returns-background.exe
                              main-returns-background-change.exe)
  endif()
endif()

# The two finalizer tests only work under sgen
# gc-altstack.exe fails under boehm because it has no support for altstack
# bug-459094.exe creates an extremely deep directory tree
# delegate-invoke.exe depends on 929c6bc9b6d76a273f251e6f5dfacac36e9c38bd which was
# reverted.
# bug-Xamarin-5278.exe got broken by 5d26590e79da139a284459299aee95c25f4cd835
set(DISABLED_TESTS
  delegate-async-exception.exe
  bug-348522.2.exe
  bug-459094.exe
  delegate-invoke.exe
  bug-Xamarin-5278.exe
  ${PLATFORM_DISABLED_TESTS}
  ${EXTRA_DISABLED_TESTS}
)

set(DISABLED_TESTS_WRENCH
  ${DISABLED_TESTS}
  ${PLATFORM_DISABLED_TESTS_WRENCH}
  main-returns-background-resetabort.exe
  main-returns-background-abort-resetabort.exe
  thread6.exe
  assemblyresolve_event3.exe
  delegate2.exe
  finally_guard.exe
  gc-altstack.exe
  generic-xdomain.2.exe
)

set(AOT_DISABLED_TESTS constraints-load.exe)

# These only compile with MS CSC
set(TEST_CSC_SRC vararg.cs)

set(TEST_IL_SRC
  field-access.il
  method-access.il
  ldftn-access.il
  cpblkTest.il
  vbinterface.il
  calliTest.il
  calliGenericTest.il
  ckfiniteTest.il
  fault-handler.il
  locallocTest.il
  initblkTest.il
  qt-instance.il
  vararg.il
  bug-29859.il
  bug-78549.il
  static-fields-nonconst.il
  reload-at-bb-end.il
  test-enum-indstoreil.il
  filter-bug.il
  even-odd.il
  bug-82022.il
  vt-sync-method.il
  enum_types.il
  invalid-token.il
  call_missing_method.il
  call_missing_class.il
  ldfld_missing_field.il
  ldfld_missing_class.il
  find-method.2.il
  bug-79215.2.il
  bug-79956.2.il
  bug-327438.2.il
  bug-387274.2.il
  bug-426309.2.il
  ldtoken_with_byref_typespec.2.il
  resolve_method_bug.2.il
  resolve_field_bug.2.il
  resolve_type_bug.2.il
  generics-sharing-other-exc.2.il
  generic-ldobj.2.il
  generic-mkrefany.2.il
  generic-refanyval.2.il
  generic-ldtoken.2.il
  generic-ldtoken-method.2.il
  generic-ldtoken-field.2.il
  generic-tailcall.2.il
  generic-tailcall2.2.il
  generic-array-exc.2.il
  generic-valuetype-newobj2.2.il
  generic-valuetype-newobj.2.il
  generic-constrained.2.il
  generic-type-load-exception.2.il
  bug-81466.il
  bug457574.il
  bug445361.il
  bug-463303.il
  bug469742.2.il
  bug-528055.il
  constraints-load.il
  array_load_exception.il
  bug-481403.il
  interface-with-static-method.il
  bug-515884.il
  bug-633291.il
  delegate-with-null-target.il
  bug-318677.il
  gsharing-valuetype-layout.il
  invalid_generic_instantiation.il
)

# Tests
set(TESTS_FOLDER "Tests/mono/tests")
set(JIT_TESTS_FOLDER "${TESTS_FOLDER}/JIT")
set(ASSEMBLIES_TEST_FOLDER "${TESTS_FOLDER}/Assemblies")
set(TOOLS_TEST_FOLDER "${TESTS_FOLDER}/Tools")

foreach(TEST_SOURCE ${TEST_CS_SRC})
  string(REGEX REPLACE "\\.cs$" "" TEST_TARGET ${TEST_SOURCE})
  cs_add_executable(${TEST_TARGET} SOURCES ${TEST_SOURCE} FLAGS ${CSFLAGS} LIBRARIES TestDriver FOLDER ${ASSEMBLIES_TEST_FOLDER})
endforeach()

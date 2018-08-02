#! /bin/sh -e

helix_root="$(pwd)"

cd net_4_x || exit 1    # TODO: get rid of this, currently required because the compiler-tester tries to find the profile dir by looking for "net_4_x"

r="$(pwd)"
MONO_CFG_DIR="$r/runtime/etc"
PATH="$r/runtime/_tmpinst/bin:$PATH"
MONO_EXECUTABLE=${MONO_EXECUTABLE:-"$r/mono-sgen"}
MONO_PATH="$r:$r/tests"
export MONO_CFG_DIR MONO_PATH MONO_EXECUTABLE PATH
chmod +x "${MONO_EXECUTABLE}"


if ! "${MONO_EXECUTABLE}" --version; then  # this can happen when running the i386 binary on amd64 and the i386 packages aren't installed
    sudo dpkg --add-architecture i386
    sudo apt update
    sudo apt install -y libc6-i386 lib32gcc1
fi

if [ "$1" = "run-bcl-tests" ]; then
    if [ "$2" = "xunit" ]; then
        export REMOTE_EXECUTOR="$r/RemoteExecutorConsoleApp.exe"
        "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug xunit.console.exe "tests/$3" -noappdomain -noshadow -parallel none -xml "${helix_root}/testResults.xml" -notrait category=failing -notrait category=nonmonotests -notrait Benchmark=true -notrait category=outerloop -notrait category=nonlinuxtests
        exit $?
    elif [ "$2" = "nunit" ]; then
        MONO_REGISTRY_PATH="$HOME/.mono/registry" MONO_TESTS_IN_PROGRESS="yes" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug nunit-lite-console.exe "tests/$3" -exclude=NotWorking,CAS -labels -format:xunit -result:"${helix_root}/testResults.xml"
        exit $?
    else
        echo "Unknown test runner."
        exit 1
    fi
fi

if [ "$1" = "run-verify" ]; then
    verifiable_files=$(find . -maxdepth 1 -name "*.dll" -or -name "*.exe" | grep -v System.Runtime.CompilerServices.Unsafe.dll)
    ok=true
    for asm in $verifiable_files; do
        echo "$asm"
        if [ ! -f "$asm" ]; then continue; fi
        if "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --compile-all --verify-all --security=verifiable "$asm"; then
            echo "$asm verified OK"
        else
            echo "$asm verification failed"
            ok=false
        fi
    done;
    if [ "$ok" = "true" ]; then
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='verify' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='1' failed='0' skipped='0' errors='0' time='0'><collection total='1' passed='1' failed='0' skipped='0' name='Test collection for verify' time='0'><test name='verify.all' type='verify' method='all' time='0' result='Pass'></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='verify' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='0' failed='1' skipped='0' errors='0' time='0'><collection total='1' passed='0' failed='1' skipped='0' name='Test collection for verify' time='0'><test name='verify.all' type='verify' method='all' time='0' result='Fail'><failure exception-type='VerifyException'><message><![CDATA[Verifying framework assemblies failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi
fi

if [ "$1" = "run-mcs" ]; then
    cd tests/mcs || exit 1
    MONO_PATH=".:$MONO_PATH" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --verify-all compiler-tester.exe -mode:pos -files:v4 -compiler:"$r/mcs.exe" -issues:known-issues-net_4_x -log:net_4_x.log -il:ver-il-net_4_x.xml -resultXml:"${helix_root}/testResults.xml" -compiler-options:"-d:NET_4_0;NET_4_5 -debug"
    exit $?
fi

if [ "$1" = "run-mcs-errors" ]; then
    cd tests/mcs-errors || exit 1
    MONO_PATH=".:$MONO_PATH" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" compiler-tester.exe -mode:neg -files:v4 -compiler:"$r/mcs.exe" -issues:known-issues-net_4_x -log:net_4_x.log -resultXml:"${helix_root}/testResults.xml" -compiler-options:"-v --break-on-ice -d:NET_4_0;NET_4_5"
    exit $?
fi

if [ "$1" = "run-aot-test" ]; then
    failed=0
    passed=0
    failed_tests=""
    profile=$(pwd)
    tmpfile=$(mktemp -t mono_aot_outputXXXXXX) || exit 1
    rm -f "test-aot-*.stdout" "test-aot-*.stderr" "${helix_root}/testResults-cases.xml"
    for assembly in "$profile"/*.dll; do
        asm_name=$(basename "$assembly")
        echo "... $asm_name"
        for conf in "|regular" "--gc=boehm|boehm"; do
            name=$(echo $conf | cut -d\| -f 2)
            params=$(echo $conf | cut -d\| -f 1)
            test_name="${asm_name}|${name}"
            echo "  $test_name"
            if "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" "$params" --aot=outfile="$tmpfile" "$assembly" > "test-aot-${name}-${asm_name}.stdout" 2> "test-aot-${name}-${asm_name}.stderr"
            then
                passed=$((passed + 1))
                resultstring="Pass"
            else \
                failed=$((failed + 1))
                failed_tests="${failed_tests} $test_name"
                resultstring="Fail"
            fi
            echo "<test name='aot-test.$name.$asm_name' type='aot-test.$name' method='$asm_name' time='0' result='$resultstring'>" >> "${helix_root}/testResults-cases.xml"
            if [ "$resultstring" = "Fail" ]; then
                echo "<failure exception-type='AotTestException'><message><![CDATA[
                    STDOUT:
                    $(cat "test-aot-${name}-${asm_name}.stdout")
                    STDERR:
                    $(cat "test-aot-${name}-${asm_name}.stderr")]]></message><stack-trace></stack-trace></failure>" >> "${helix_root}/testResults-cases.xml"; fi
            echo "</test>" >> "${helix_root}/testResults-cases.xml"
        done
    done
    echo "<?xml version='1.0' encoding='utf-8'?>\
    <assemblies>\
        <assembly name='aot-test' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='$((passed + failed))' passed='$passed' failed='$failed' skipped='0' errors='0' time='0'>\
            <collection total='$((passed + failed))' passed='$passed' failed='$failed' skipped='0' name='Test collection for aot-test' time='0'>\
                $(cat "${helix_root}/testResults-cases.xml")
            </collection>\
        </assembly>\
    </assemblies>" > "${helix_root}/testResults.xml";
    rm "$tmpfile"
    echo "${passed} test(s) passed. ${failed} test(s) did not pass."
    if [ "${failed}" != 0 ]; then
        echo ""
        echo "Failed tests:"
        echo ""
        for i in ${failed_tests}; do
            echo "${i}";
        done
        exit 1
    fi
    exit 0
fi

if [ "$1" = "run-mini" ]; then
    cd tests/mini || exit 1
    "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --regression ./*.exe > regressiontests.out 2>&1
    cat regressiontests.out
    if grep -q "100% pass" regressiontests.out; then
        resultstring="Pass"
        failurescount=0
        successcount=1
    else
        resultstring="Fail"
        failurescount=1
        successcount=0
    fi
    echo "<?xml version='1.0' encoding='utf-8'?>\
        <assemblies>\
            <assembly name='mini.regression-tests' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='$successcount' failed='$failurescount' skipped='0' errors='0' time='0'>\
                <collection total='1' passed='$successcount' failed='$failurescount' skipped='0' name='Test collection for mini.regression-tests' time='0'>\
                    <test name='mini.regression-tests.all' type='mini.regression-tests' method='all' time='0' result='$resultstring'>" > "${helix_root}/testResults.xml"
                    if [ "$resultstring" = "Fail" ]; then echo "<failure exception-type='MiniRegressionTestsException'><message><![CDATA[$(cat regressiontests.out)]]></message><stack-trace></stack-trace></failure>" >> "${helix_root}/testResults.xml"; fi
                echo "</test>
                </collection>\
            </assembly>\
        </assemblies>" >> "${helix_root}/testResults.xml";
    exit $failurescount
fi

if [ "$1" = "run-symbolicate" ]; then
    cd tests/symbolicate || exit 1

    "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --aot 2>&1 | grep -q "AOT compilation is not supported" && echo "No AOT support, skipping tests." && exit 0

    ok=true
    for config in without-aot with-aot with-aot-msym; do
        OUT_DIR="$config"
        MSYM_DIR="$OUT_DIR/msymdir"
        STACKTRACE_FILE="$OUT_DIR/stacktrace.out"
        SYMBOLICATE_RAW_FILE="$OUT_DIR/symbolicate_raw.out"
        SYMBOLICATE_RESULT_FILE="$OUT_DIR/symbolicate.result"
        SYMBOLICATE_EXPECTED_FILE=symbolicate.expected

        echo "Checking StackTraceDumper.exe in configuration $config..."
        rm -rf "$OUT_DIR"
        mkdir -p "$OUT_DIR"
        mkdir -p "$MSYM_DIR"

        cp StackTraceDumper.exe "$OUT_DIR"
        cp StackTraceDumper.pdb "$OUT_DIR"

        # store symbols
        "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" "$r/mono-symbolicate.exe" store-symbols "$MSYM_DIR" "$OUT_DIR"
        "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" "$r/mono-symbolicate.exe" store-symbols "$MSYM_DIR" "$r"

        if [ "$config" = "with-aot" ]; then "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" -O=-inline --aot "$OUT_DIR/StackTraceDumper.exe"; fi
        if [ "$config" = "with-aot-msym" ]; then "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" -O=-inline --aot=msym-dir="$MSYM_DIR" "$OUT_DIR/StackTraceDumper.exe"; fi

        # check diff
        "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" -O=-inline StackTraceDumper.exe > "$STACKTRACE_FILE"
        "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" "$r/mono-symbolicate.exe" "$MSYM_DIR" "$STACKTRACE_FILE" > "$SYMBOLICATE_RAW_FILE"
        tr "\\\\" "/" < "$SYMBOLICATE_RAW_FILE" | sed "s,) .* in .*/mcs/,) in mcs/," | sed "s,) .* in .*/external/,) in external/," | sed '/\[MVID\]/d' | sed '/\[AOTID\]/d' > "$SYMBOLICATE_RESULT_FILE"

        DIFF=$(diff -up "$SYMBOLICATE_EXPECTED_FILE" "$SYMBOLICATE_RESULT_FILE")
        if [ ! -z "$DIFF" ]; then
            echo "ERROR: Symbolicate tests failed."
            echo "If $SYMBOLICATE_RESULT_FILE is correct copy it to $SYMBOLICATE_EXPECTED_FILE."
            echo "Otherwise runtime sequence points need to be fixed."
            echo ""
            echo "$DIFF"
            ok=false
        else
            echo "Success."
        fi
    done

    if [ "$ok" = "true" ]; then
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='symbolicate' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='1' failed='0' skipped='0' errors='0' time='0'><collection total='1' passed='1' failed='0' skipped='0' name='Test collection for symbolicate' time='0'><test name='symbolicate.all' type='symbolicate' method='all' time='0' result='Pass'></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='symbolicate' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='0' failed='1' skipped='0' errors='0' time='0'><collection total='1' passed='0' failed='1' skipped='0' name='Test collection for symbolicate' time='0'><test name='symbolicate.all' type='symbolicate' method='all' time='0' result='Fail'><failure exception-type='SymbolicateException'><message><![CDATA[Symbolicate tests failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi

fi

if [ "$1" = "run-csi" ]; then
    cd tests/csi || exit 1
    echo 'Console.WriteLine ("hello world: " + DateTime.Now)' > csi-test.csx

    ok=true
    "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" csi.exe csi-test.csx > csi-test-output.txt || ok=false
    cat csi-test-output.txt && grep -q "hello world" csi-test-output.txt || ok=false

    if [ "$ok" = "true" ]; then
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='csi' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='1' failed='0' skipped='0' errors='0' time='0'><collection total='1' passed='1' failed='0' skipped='0' name='Test collection for csi' time='0'><test name='csi.all' type='csi' method='all' time='0' result='Pass'></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='csi' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='0' failed='1' skipped='0' errors='0' time='0'><collection total='1' passed='0' failed='1' skipped='0' name='Test collection for csi' time='0'><test name='csi.all' type='csi' method='all' time='0' result='Fail'><failure exception-type='CsiException'><message><![CDATA[csi.exe tests failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi

fi

if [ "$1" = "run-profiler" ]; then
    cd tests/profiler || exit 1
    perl ptestrunner.pl "helix" xunit "${helix_root}/testResults.xml"
fi

if [ "$1" = "run-runtime" ]; then
    cd tests/runtime || exit 1
    export CI=1
    TESTS_CS="generic-unloading-sub.2.exe \
    create-instance.exe \
    bug-2907.exe  \
    array-init.exe  \
    arraylist.exe  \
    assembly-load-remap.exe \
    assembly-load-bytes.exe \
    assembly-loadfile.exe \
    assembly-loadfrom.exe \
    assembly-load-bytes-bindingredirect.exe \
    assembly-loadfile-bindingredirect.exe \
    assembly-loadfrom-bindingredirect.exe \
    assembly-loadfrom-simplename.exe \
    assemblyresolve_event.exe \
    assemblyresolve_event3.exe \
    assemblyresolve_event4.exe \
    assemblyresolve_event5.exe \
    assemblyresolve_event6.exe \
    checked.exe  \
    char-isnumber.exe \
    field-layout.exe  \
    pack-layout.exe  \
    pack-bug.exe  \
    hash-table.exe  \
    test-ops.exe  \
    obj.exe   \
    test-dup-mp.exe  \
    string.exe  \
    stringbuilder.exe \
    switch.exe  \
    outparm.exe  \
    delegate.exe  \
    bitconverter.exe  \
    exception.exe  \
    exception2.exe  \
    exception3.exe  \
    exception4.exe  \
    exception5.exe  \
    exception6.exe  \
    exception7.exe  \
    exception8.exe  \
    exception10.exe  \
    exception11.exe  \
    exception12.exe  \
    exception13.exe  \
    exception14.exe  \
    exception15.exe  \
    exception16.exe  \
    exception17.exe  \
    exception18.exe  \
    exception19.exe  \
    exception20.exe  \
    typeload-unaligned.exe \
    struct.exe  \
    valuetype-gettype.exe \
    typeof-ptr.exe  \
    static-constructor.exe \
    pinvoke.exe  \
    pinvoke-utf8.exe  \
    pinvoke3.exe  \
    pinvoke11.exe  \
    pinvoke13.exe  \
    pinvoke17.exe  \
    invoke.exe  \
    invoke2.exe  \
    runtime-invoke.exe  \
    invoke-string-ctors.exe  \
    reinit.exe  \
    box.exe   \
    array.exe  \
    enum.exe   \
    enum2.exe  \
    enum-intrins.exe  \
    property.exe  \
    enumcast.exe  \
    assignable-tests.exe \
    array-cast.exe  \
    array-subtype-attr.exe \
    cattr-compile.exe \
    cattr-field.exe  \
    cattr-object.exe  \
    custom-attr.exe  \
    double-cast.exe  \
    newobj-valuetype.exe \
    arraylist-clone.exe \
    setenv.exe  \
    vtype.exe  \
    isvaluetype.exe  \
    iface6.exe  \
    iface7.exe  \
    ipaddress.exe  \
    array-vt.exe  \
    interface1.exe  \
    reflection-enum.exe \
    reflection-prop.exe \
    reflection4.exe  \
    reflection5.exe  \
    reflection-const-field.exe \
    many-locals.exe  \
    string-compare.exe \
    test-prime.exe  \
    test-tls.exe  \
    params.exe  \
    reflection.exe  \
    interface.exe  \
    iface.exe  \
    iface2.exe  \
    iface3.exe  \
    iface4.exe  \
    iface-large.exe  \
    iface-contravariant1.exe \
    virtual-method.exe \
    intptrcast.exe  \
    indexer.exe  \
    stream.exe  \
    console.exe  \
    shift.exe  \
    jit-int.exe  \
    jit-uint.exe  \
    jit-long.exe  \
    long.exe   \
    jit-ulong.exe  \
    jit-float.exe  \
    pop.exe   \
    time.exe   \
    pointer.exe  \
    hashcode.exe  \
    delegate1.exe  \
    delegate2.exe  \
    delegate3.exe  \
    delegate5.exe  \
    delegate6.exe  \
    delegate7.exe  \
    delegate8.exe  \
    delegate10.exe  \
    delegate11.exe  \
    delegate12.exe  \
    delegate13.exe  \
    delegate14.exe  \
    delegate15.exe  \
    largeexp.exe  \
    largeexp2.exe  \
    marshalbyref1.exe \
    static-ctor.exe  \
    inctest.exe  \
    bound.exe  \
    array-invoke.exe  \
    test-arr.exe  \
    decimal.exe  \
    decimal-array.exe \
    marshal.exe  \
    marshal1.exe  \
    marshal2.exe  \
    marshal3.exe  \
    marshal5.exe  \
    marshal6.exe  \
    marshal7.exe  \
    marshal8.exe  \
    marshal9.exe  \
    marshalbool.exe  \
    test-byval-in-struct.exe \
    thread.exe  \
    thread5.exe  \
    thread-static.exe \
    thread-static-init.exe \
    context-static.exe \
    float-pop.exe  \
    interfacecast.exe \
    array3.exe  \
    classinit.exe  \
    classinit2.exe  \
    classinit3.exe  \
    synchronized.exe  \
    async_read.exe  \
    threadpool.exe  \
    threadpool1.exe  \
    threadpool-exceptions1.exe \
    threadpool-exceptions2.exe \
    threadpool-exceptions3.exe \
    threadpool-exceptions4.exe \
    threadpool-exceptions5.exe \
    threadpool-exceptions6.exe \
    base-definition.exe \
    bug-27420.exe  \
    bug-46781.exe  \
    bug-42136.exe  \
    bug-59286.exe  \
    bug-70561.exe  \
    bug-78311.exe  \
    bug-78653.exe  \
    bug-78656.exe  \
    bug-77127.exe  \
    bug-323114.exe  \
    bug-Xamarin-5278.exe \
    interlocked.exe  \
    delegate-async-exit.exe \
    delegate-delegate-exit.exe \
    delegate-exit.exe \
    delegate-disposed-hashcode.exe \
    finalizer-abort.exe \
    finalizer-exception.exe \
    finalizer-exit.exe \
    finalizer-thread.exe \
    main-exit.exe \
    main-returns-abort-resetabort.exe \
    main-returns-background-abort-resetabort.exe \
    main-returns-background-resetabort.exe \
    main-returns-background.exe \
    main-returns-background-change.exe \
    main-returns.exe  \
    subthread-exit.exe \
    desweak.exe  \
    exists.exe  \
    handleref.exe \
    install_eh_callback.exe \
    dbnull-missing.exe \
    test-type-ctor.exe  \
    soft-float-tests.exe \
    thread-exit.exe  \
    finalize-parent.exe \
    interlocked-2.2.exe \
    pinvoke-2.2.exe   \
    bug-78431.2.exe   \
    bug-79684.2.exe   \
    catch-generics.2.exe \
    event-get.2.exe  \
    safehandle.2.exe  \
    module-cctor-loader.2.exe \
    generics-invoke-byref.2.exe \
    generic-signature-compare.2.exe \
    generics-sharing.2.exe \
    shared-generic-methods.2.exe \
    shared-generic-synchronized.2.exe \
    generic-inlining.2.exe \
    generic-initobj.2.exe \
    generic-delegate.2.exe \
    generic-sizeof.2.exe \
    generic-virtual.2.exe \
    generic-interface-methods.2.exe \
    generic-array-type.2.exe \
    generic-method-patching.2.exe \
    generic-static-methods.2.exe \
    generic-null-call.2.exe \
    generic-special.2.exe \
    generic-special2.2.exe \
    generic-exceptions.2.exe \
    generic-virtual2.2.exe \
    generic-valuetype-interface.2.exe \
    generic-getgenericarguments.2.exe \
    generic-synchronized.2.exe \
    generic-delegate-ctor.2.exe \
    generic-array-iface-set.2.exe \
    generic-typedef.2.exe \
    bug-431413.2.exe \
    bug-459285.2.exe \
    generic-virtual-invoke.2.exe \
    bug-461198.2.exe \
    generic-sealed-virtual.2.exe \
    generic-system-arrays.2.exe \
    generic-stack-traces.2.exe \
    generic-stack-traces2.2.exe \
    bug-472600.2.exe \
    recursive-generics.2.exe \
    bug-473482.2.exe \
    bug-473999.2.exe \
    bug-479763.2.exe \
    bug-616463.exe \
    bug-80392.2.exe  \
    bug-82194.2.exe \
    anonarray.2.exe \
    ienumerator-interfaces.2.exe \
    array-enumerator-ifaces.2.exe \
    generic_type_definition_encoding.2.exe \
    bug-333798.2.exe  \
    bug-348522.2.exe  \
    bug-340662_bug.exe \
    bug-325283.2.exe \
    thunks.exe \
    winx64structs.exe \
    nullable_boxing.2.exe \
    valuetype-equals.exe \
    custom-modifiers.2.exe \
    custom-modifiers-inheritance.exe \
    bug-382986.exe \
    test-inline-call-stack.exe \
    bug-324535.exe \
    modules.exe \
    bug-81673.exe \
    bug-81691.exe \
    bug-415577.exe \
    filter-stack.exe \
    vararg.exe \
    vararg2.exe \
    bug-461867.exe \
    bug-461941.exe \
    bug-461261.exe \
    bug-400716.exe \
    bug-459094.exe \
    bug-467456.exe \
    bug-508538.exe \
    bug-472692.2.exe  \
    gchandles.exe \
    interlocked-3.exe \
    interlocked-4.2.exe \
    w32message.exe \
    gc-altstack.exe \
    large-gc-bitmap.exe \
    bug-561239.exe \
    bug-562150.exe \
    bug-599469.exe \
    monitor-resurrection.exe \
    monitor-wait-abort.exe \
    monitor-abort.exe \
    bug-666008.exe \
    bug-685908.exe \
    sgen-long-vtype.exe \
    delegate-invoke.exe \
    delegate-prop.exe \
    bug-696593.exe \
    bug-705140.exe \
    bug-1147.exe \
    mono-path.exe \
    bug-bxc-795.exe \
    bug-3903.exe \
    async-with-cb-throws.exe \
    bug-6148.exe \
    bug-10127.exe \
    bug-18026.exe \
    allow-synchronous-major.exe \
    block_guard_restore_aligment_on_exit.exe \
    thread_static_gc_layout.exe \
    sleep.exe \
    bug-27147.exe \
    bug-30085.exe \
    bug-17537.exe \
    pinvoke_ppcc.exe \
    pinvoke_ppcs.exe \
    pinvoke_ppci.exe \
    pinvoke_ppcf.exe \
    pinvoke_ppcd.exe \
    bug-29585.exe \
    priority.exe \
    abort-cctor.exe \
    abort-try-holes.exe \
    thread-native-exit.exe \
    reference-loader.exe \
    thread-suspend-suspended.exe \
    thread-suspend-selfsuspended.exe \
    remoting4.exe \
    remoting1.exe \
    remoting2.exe \
    remoting3.exe \
    remoting5.exe \
    appdomain.exe \
    appdomain-client.exe \
    appdomain-unload.exe \
    appdomain-async-invoke.exe \
    appdomain-thread-abort.exe \
    appdomain1.exe \
    appdomain2.exe \
    appdomain-exit.exe \
    appdomain-serialize-exception.exe \
    assemblyresolve_event2.2.exe \
    appdomain-unload-callback.exe \
    appdomain-unload-doesnot-raise-pending-events.exe \
    appdomain-unload-asmload.exe \
    appdomain-unload-exception.exe \
    unload-appdomain-on-shutdown.exe \
    appdomain-marshalbyref-assemblyload.exe \
    bug-47295.exe \
    loader.exe \
    pinvoke2.exe \
    generic-type-builder.2.exe \
    dynamic-generic-size.exe \
    cominterop.exe \
    dynamic-method-access.2.exe \
    dynamic-method-finalize.2.exe \
    dynamic-method-stack-traces.exe \
    generic_type_definition.2.exe \
    bug-333798-tb.2.exe \
    bug-335131.2.exe \
    bug-322722_patch_bx.2.exe \
    bug-322722_dyn_method_throw.2.exe \
    bug-389886-2.exe \
    bug-349190.2.exe \
    bug-389886-sre-generic-interface-instances.exe \
    bug-462592.exe \
    bug-575941.exe \
    bug-389886-3.exe \
    constant-division.exe \
    dynamic-method-resurrection.exe \
    bug-80307.exe \
    assembly_append_ordering.exe \
    bug-544446.exe \
    bug-36848.exe \
    generic-marshalbyref.2.exe \
    stackframes-async.2.exe \
    transparentproxy.exe \
    bug-48015.exe \
    delegate9.exe \
    marshal-valuetypes.exe \
    xdomain-threads.exe \
    monitor.exe \
    generic-xdomain.2.exe \
    threadpool-exceptions7.exe \
    cross-domain.exe \
    generic-unloading.2.exe \
    namedmutex-destroy-race.exe \
    thread6.exe \
    thread7.exe \
    appdomain-threadpool-unload.exe \
    process-unref-race.exe \
    bug-46661.exe \
    w32message.exe \
    runtime-invoke.gen.exe \
    imt_big_iface_test.exe \
    bug-58782-plain-throw.exe \
    bug-58782-capture-and-throw.exe \
    recursive-struct-arrays.exe \
    struct-explicit-layout.exe \
    bug-59281.exe \
    init_array_with_lazy_type.exe \
    weak-fields.exe \
    threads-leak.exe \
    threads-init.exe \
    bug-60848.exe \
    bug-59400.exe \
    tailcall-generic-cast-cs.exe \
    tailcall-interface.exe \
    bug-60843.exe \
    nested_type_visibility.exe \
    dynamic-method-churn.exe \
    verbose.exe"

    # TODO: only ported runtest-managed for now
    "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug test-runner.exe --verbose --xunit "${helix_root}/testResults.xml" --config tests-config --runtime "${MONO_EXECUTABLE}" --mono-path "$r" -j a --testsuite-name "runtime" --timeout 300 --disabled "$DISABLED_TESTS" $TESTS_CS # $(TESTS_IL) $(TESTS_BENCH)

fi

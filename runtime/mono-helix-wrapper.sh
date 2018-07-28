#! /bin/sh

helix_root="$(pwd)"

cd net_4_x || exit 1    # TODO: get rid of this, currently required because the compiler-tester tries to find the profile dir by looking for "net_4_x"

r="$(pwd)"
MONO_CFG_DIR="$r/runtime/etc"
PATH="$r/runtime/_tmpinst/bin:$PATH"
MONO_EXECUTABLE=${MONO_EXECUTABLE:-"$r/mono-sgen"}
MONO_PATH="$r:$r/tests"
export MONO_CFG_DIR MONO_PATH PATH
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
        MONO_REGISTRY_PATH="$HOME/.mono/registry" MONO_TESTS_IN_PROGRESS="yes" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug nunit-lite-console.exe "tests/$3" -format:xunit -result:"${helix_root}/testResults.xml"
        exit $?
    else
        echo "Unknown test runner."
        exit 1
    fi
fi

if [ "$1" = "run-verify" ]; then
    verifiable_files=$(find . -name "*.dll" -or -name "*.exe" | grep -v System.Runtime.CompilerServices.Unsafe.dll)
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
    rm -f "test-aot-${name}.stdout" "test-aot-${name}.stderr"
    for assembly in "$profile"/*.dll; do
        asm_name=$(basename "$assembly")
        echo "... $asm_name"
        for conf in "|regular" "--gc=boehm|boehm"; do
            name=$(echo $conf | cut -d\| -f 2)
            params=$(echo $conf | cut -d\| -f 1)
            test_name="${asm_name}|${name}"
            echo "  $test_name"
            if "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" "$params" --aot=outfile="$tmpfile" "$assembly" >> "test-aot-${name}.stdout" 2>> "test-aot-${name}.stderr"
            then
                passed=$((passed + 1))
            else \
                failed=$((failed + 1))
                failed_tests="${failed_tests} $test_name"
            fi
        done
    done
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
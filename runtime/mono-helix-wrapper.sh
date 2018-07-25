#! /bin/sh

helix_root="$(pwd)"

cd net_4_x    # TODO: get rid of this, currently required because the compiler-tester tries to find the profile dir by looking for "net_4_x"

r="$(pwd)"
MONO_CFG_DIR="$r/runtime/etc"
PATH="$r/runtime/_tmpinst/bin:$PATH"
MONO_EXECUTABLE=${MONO_EXECUTABLE:-"$r/mono-sgen"}
MONO_PATH="$r:$r/tests"
export MONO_CFG_DIR MONO_PATH PATH
chmod +x "${MONO_EXECUTABLE}"

"${MONO_EXECUTABLE}" --version

if [ $? -ne 0 ]; then  # this can happen when running the i386 binary on amd64 and the i386 packages aren't installed
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
    verifiable_files=$(ls "$(pwd)" | grep -E '\.(dll|exe)$' | grep -v System.Runtime.CompilerServices.Unsafe.dll)
    ok=true
    for asm in $verifiable_files; do
        echo $asm
        if [ ! -f $asm ]; then continue; fi
        if "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --compile-all --verify-all --security=verifiable $asm; then
            echo $asm verified OK
        else
            echo $asm verification failed
            ok=false
        fi
    done;
    if [ "$ok" = "false" ]; then
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='verify' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='0' failed='1' skipped='0' errors='0' time='0'><collection total='1' passed='0' failed='1' skipped='0' name='Test collection for verify' time='0'><test name='verify.all' type='verify' method='all' time='0' result='Fail'><failure exception-type='VerifyException'><message><![CDATA[Verifying framework assemblies failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    else
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='verify' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='1' failed='0' skipped='0' errors='0' time='0'><collection total='1' passed='1' failed='0' skipped='0' name='Test collection for verify' time='0'><test name='verify.all' type='verify' method='all' time='0' result='Pass'></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    fi
fi

if [ "$1" = "run-compiler" ]; then
    cd tests/compiler
    MONO_PATH=".:$MONO_PATH" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --verify-all compiler-tester.exe -mode:pos -files:v4 -compiler:"$r/mcs.exe" -issues:known-issues-net_4_x -log:net_4_x.log -il:ver-il-net_4_x.xml -compiler-options:"-d:NET_4_0;NET_4_5 -debug"
    if [ $? -eq 0 ]; then
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='compiler' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='1' failed='0' skipped='0' errors='0' time='0'><collection total='1' passed='1' failed='0' skipped='0' name='Test collection for compiler' time='0'><test name='compiler.all' type='verify' method='all' time='0' result='Pass'></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='compiler' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='0' failed='1' skipped='0' errors='0' time='0'><collection total='1' passed='0' failed='1' skipped='0' name='Test collection for compiler' time='0'><test name='compiler.all' type='verify' method='all' time='0' result='Fail'><failure exception-type='CompilerException'><message><![CDATA[Compiler tests failed. Check the log for more details.]]></message></failure>/test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi
fi

if [ "$1" = "run-compiler-errors" ]; then
    cd tests/compiler-errors
    MONO_PATH=".:$MONO_PATH" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" compiler-tester.exe -mode:neg -files:v4 -compiler:"$r/mcs.exe" -issues:known-issues-net_4_x -log:net_4_x.log -compiler-options:"-v --break-on-ice -d:NET_4_0;NET_4_5"
    if [ $? -eq 0 ]; then
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='compiler-errors' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='1' failed='0' skipped='0' errors='0' time='0'><collection total='1' passed='1' failed='0' skipped='0' name='Test collection for compiler-errors' time='0'><test name='compiler-errors.all' type='verify' method='all' time='0' result='Pass'></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version='1.0' encoding='utf-8'?><assemblies><assembly name='compiler-errors' environment='Mono' test-framework='custom' run-date='$(date +%F)' run-time='$(date +%T)' total='1' passed='0' failed='1' skipped='0' errors='0' time='0'><collection total='1' passed='0' failed='1' skipped='0' name='Test collection for compiler-errors' time='0'><test name='compiler-errors.all' type='verify' method='all' time='0' result='Fail'><failure exception-type='CompilerErrorsException'><message><![CDATA[Compiler error tests failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi
fi

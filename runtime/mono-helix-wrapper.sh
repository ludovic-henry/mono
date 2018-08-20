#! /bin/sh -e

helix_root="$(pwd)"

cd "$1" || exit 1
cd net_4_x || exit 1    # TODO: get rid of this, currently required because the compiler-tester tries to find the profile dir by looking for "net_4_x"

r="$(pwd)"
MONO_CFG_DIR="$r/runtime/etc"
PATH="$r/runtime/_tmpinst/bin:$PATH"
MONO_EXECUTABLE=${MONO_EXECUTABLE:-"$r/mono-sgen"}
MONO_PATH="$r"
export MONO_CFG_DIR MONO_PATH MONO_EXECUTABLE PATH

if file "${MONO_EXECUTABLE}" | grep "ELF 32-bit"; then  # when running the i386 binary on amd64, install the i386 packages
    sudo dpkg --add-architecture i386
    sudo apt update
    sudo apt install -y libc6-i386 lib32gcc1 libsqlite3-0:i386 libglib2.0-0:i386 libcairo2:i386 libfreetype6:i386 libjpeg62-turbo:i386 libtiff5:i386 libgif7:i386 libpng16-16:i386 libx11-6:i386 exif:i386 libfontconfig1:i386
    wget "https://xamjenkinsartifact.blob.core.windows.net/test-libgdiplus-mainline/280/debian-9-i386/src/.libs/libgdiplus.so" -O "$r/mono-libgdiplus.so"
else
    wget "https://xamjenkinsartifact.blob.core.windows.net/test-libgdiplus-mainline/280/debian-9-amd64/src/.libs/libgdiplus.so" -O "$r/mono-libgdiplus.so"
fi

ldd "$r/mono-libgdiplus.so"
sed "s,\$helix_root_dir,$r,g" "$r/runtime/etc/mono/config.tmpl" > "$r/runtime/etc/mono/config"
sed "s,\$helix_root_dir,$r,g" "$r/tests/runtime/tests-config.tmpl" > "$r/tests/runtime/tests-config"

chmod +x "${MONO_EXECUTABLE}"
"${MONO_EXECUTABLE}" --version

if [ "$2" = "run-xunit" ]; then
    export MONO_PATH="$r/tests:$MONO_PATH"
    export REMOTE_EXECUTOR="$r/RemoteExecutorConsoleApp.exe"
    "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug xunit.console.exe "tests/$3" -noappdomain -noshadow -parallel none -xml "${helix_root}/testResults.xml" -notrait category=failing -notrait category=nonmonotests -notrait Benchmark=true -notrait category=outerloop -notrait category=nonlinuxtests
    exit $?
fi

if [ "$2" = "run-nunit" ]; then
    export MONO_PATH="$r/tests:$MONO_PATH"
    case "$3" in
        *"Microsoft.Build"*)
            export TESTING_MONO=a
            export MSBuildExtensionsPath="$r/tests/xbuild/extensions"
            export XBUILD_FRAMEWORK_FOLDERS_PATH="$r/tests/xbuild/frameworks"
            ;;
        *"Mono.Messaging.RabbitMQ"*)
            export MONO_MESSAGING_PROVIDER=Mono.Messaging.RabbitMQ.RabbitMQMessagingProvider,Mono.Messaging.RabbitMQ
            ;;
        *"System.Windows.Forms"*)
            sudo apt install -y xvfb xauth
            XVFBRUN="xvfb-run -a --"
            ADDITIONAL_TEST_EXCLUDES="NotWithXvfb" # TODO: find out why this works on Jenkins?
            ;;
    esac
    cp -f "tests/${3}.nunitlite.config" nunit-lite-console.exe.config
    MONO_REGISTRY_PATH="$HOME/.mono/registry" MONO_TESTS_IN_PROGRESS="yes" $XVFBRUN "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug nunit-lite-console.exe "tests/$3" -exclude=NotWorking,CAS,$ADDITIONAL_TEST_EXCLUDES -labels -format:xunit -result:"${helix_root}/testResults.xml"
    exit $?
fi

if [ "$2" = "run-verify" ]; then
    verifiable_files=$(find . -maxdepth 1 -name "*.dll" -or -name "*.exe" | grep -v System.Runtime.CompilerServices.Unsafe.dll | grep -v Xunit.NetCore.Extensions.dll)
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
        echo "<?xml version=\"1.0\" encoding=\"utf-8\"?><assemblies><assembly name=\"verify\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"1\" passed=\"1\" failed=\"0\" skipped=\"0\" errors=\"0\" time=\"0\"><collection total=\"1\" passed=\"1\" failed=\"0\" skipped=\"0\" name=\"Test collection for verify\" time=\"0\"><test name=\"verify.all\" type=\"verify\" method=\"all\" time=\"0\" result=\"Pass\"></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version=\"1.0\" encoding=\"utf-8\"?><assemblies><assembly name=\"verify\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"1\" passed=\"0\" failed=\"1\" skipped=\"0\" errors=\"0\" time=\"0\"><collection total=\"1\" passed=\"0\" failed=\"1\" skipped=\"0\" name=\"Test collection for verify\" time=\"0\"><test name=\"verify.all\" type=\"verify\" method=\"all\" time=\"0\" result=\"Fail\"><failure exception-type=\"VerifyException\"><message><![CDATA[Verifying framework assemblies failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi
fi

if [ "$2" = "run-mcs" ]; then
    cd tests/mcs || exit 1
    MONO_PATH=".:$MONO_PATH" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --verify-all compiler-tester.exe -mode:pos -files:v4 -compiler:"$r/mcs.exe" -reference-dir:"$r" -issues:known-issues-net_4_x -log:net_4_x.log -il:ver-il-net_4_x.xml -resultXml:"${helix_root}/testResults.xml" -compiler-options:"-d:NET_4_0;NET_4_5 -debug"
    exit $?
fi

if [ "$2" = "run-mcs-errors" ]; then
    cd tests/mcs-errors || exit 1
    MONO_PATH=".:$MONO_PATH" "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" compiler-tester.exe -mode:neg -files:v4 -compiler:"$r/mcs.exe" -reference-dir:"$r" -issues:known-issues-net_4_x -log:net_4_x.log -resultXml:"${helix_root}/testResults.xml" -compiler-options:"-v --break-on-ice -d:NET_4_0;NET_4_5"
    exit $?
fi

if [ "$2" = "run-aot-test" ]; then
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
            if "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" $params --aot=outfile="$tmpfile" "$assembly" > "test-aot-${name}-${asm_name}.stdout" 2> "test-aot-${name}-${asm_name}.stderr"
            then
                passed=$((passed + 1))
                resultstring="Pass"
            else \
                failed=$((failed + 1))
                failed_tests="${failed_tests} $test_name"
                resultstring="Fail"
            fi
            echo "<test name=\"aot-test.$name.$asm_name\" type=\"aot-test.$name\" method=\"$asm_name\" time=\"0\" result=\"$resultstring\">" >> "${helix_root}/testResults-cases.xml"
            if [ "$resultstring" = "Fail" ]; then
                echo "<failure exception-type=\"AotTestException\"><message><![CDATA[
                    STDOUT:
                    $(cat "test-aot-${name}-${asm_name}.stdout")
                    STDERR:
                    $(cat "test-aot-${name}-${asm_name}.stderr")]]></message><stack-trace></stack-trace></failure>" >> "${helix_root}/testResults-cases.xml"; fi
            echo "</test>" >> "${helix_root}/testResults-cases.xml"
        done
    done
    echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>\
    <assemblies>\
        <assembly name=\"aot-test\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"$((passed + failed))\" passed=\"$passed\" failed=\"$failed\" skipped=\"0\" errors=\"0\" time=\"0\">\
            <collection total=\"$((passed + failed))\" passed=\"$passed\" failed=\"$failed\" skipped=\"0\" name=\"Test collection for aot-test\" time=\"0\">\
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

if [ "$2" = "run-mini" ]; then
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
    echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>\
        <assemblies>\
            <assembly name=\"mini.regression-tests\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"1\" passed=\"$successcount\" failed=\"$failurescount\" skipped=\"0\" errors=\"0\" time=\"0\">\
                <collection total=\"1\" passed=\"$successcount\" failed=\"$failurescount\" skipped=\"0\" name=\"Test collection for mini.regression-tests\" time=\"0\">\
                    <test name=\"mini.regression-tests.all\" type=\"mini.regression-tests\" method=\"all\" time=\"0\" result=\"$resultstring\">" > "${helix_root}/testResults.xml"
                    if [ "$resultstring" = "Fail" ]; then echo "<failure exception-type=\"MiniRegressionTestsException\"><message><![CDATA[$(cat regressiontests.out)]]></message><stack-trace></stack-trace></failure>" >> "${helix_root}/testResults.xml"; fi
                echo "</test>
                </collection>\
            </assembly>\
        </assemblies>" >> "${helix_root}/testResults.xml";
    exit $failurescount
fi

if [ "$2" = "run-symbolicate" ]; then
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
        echo "<?xml version=\"1.0\" encoding=\"utf-8\"?><assemblies><assembly name=\"symbolicate\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"1\" passed=\"1\" failed=\"0\" skipped=\"0\" errors=\"0\" time=\"0\"><collection total=\"1\" passed=\"1\" failed=\"0\" skipped=\"0\" name=\"Test collection for symbolicate\" time=\"0\"><test name=\"symbolicate.all\" type=\"symbolicate\" method=\"all\" time=\"0\" result=\"Pass\"></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version=\"1.0\" encoding=\"utf-8\"?><assemblies><assembly name=\"symbolicate\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"1\" passed=\"0\" failed=\"1\" skipped=\"0\" errors=\"0\" time=\"0\"><collection total=\"1\" passed=\"0\" failed=\"1\" skipped=\"0\" name=\"Test collection for symbolicate\" time=\"0\"><test name=\"symbolicate.all\" type=\"symbolicate\" method=\"all\" time=\"0\" result=\"Fail\"><failure exception-type=\"SymbolicateException\"><message><![CDATA[Symbolicate tests failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi

fi

if [ "$2" = "run-csi" ]; then
    cd tests/csi || exit 1
    echo "Console.WriteLine (\"hello world: \" + DateTime.Now)" > csi-test.csx

    ok=true
    "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" csi.exe csi-test.csx > csi-test-output.txt || ok=false
    cat csi-test-output.txt && grep -q "hello world" csi-test-output.txt || ok=false

    if [ "$ok" = "true" ]; then
        echo "<?xml version=\"1.0\" encoding=\"utf-8\"?><assemblies><assembly name=\"csi\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"1\" passed=\"1\" failed=\"0\" skipped=\"0\" errors=\"0\" time=\"0\"><collection total=\"1\" passed=\"1\" failed=\"0\" skipped=\"0\" name=\"Test collection for csi\" time=\"0\"><test name=\"csi.all\" type=\"csi\" method=\"all\" time=\"0\" result=\"Pass\"></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 0
    else
        echo "<?xml version=\"1.0\" encoding=\"utf-8\"?><assemblies><assembly name=\"csi\" environment=\"Mono\" test-framework=\"custom\" run-date=\"$(date +%F)\" run-time=\"$(date +%T)\" total=\"1\" passed=\"0\" failed=\"1\" skipped=\"0\" errors=\"0\" time=\"0\"><collection total=\"1\" passed=\"0\" failed=\"1\" skipped=\"0\" name=\"Test collection for csi\" time=\"0\"><test name=\"csi.all\" type=\"csi\" method=\"all\" time=\"0\" result=\"Fail\"><failure exception-type=\"CsiException\"><message><![CDATA[csi.exe tests failed. Check the log for more details.]]></message></failure></test></collection></assembly></assemblies>" > "${helix_root}/testResults.xml";
        exit 1
    fi

fi

if [ "$2" = "run-profiler" ]; then
    cd tests/profiler || exit 1
    chmod +x mprof-report
    PATH="$(pwd):$PATH"
    perl ptestrunner.pl "helix" xunit "${helix_root}/testResults.xml"
fi

if [ "$2" = "run-runtime" ]; then
    cd tests/runtime || exit 1
    export CI=1

    # TODO: only ported runtest-managed for now
    "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug test-runner.exe --verbose --xunit "${helix_root}/testResults.xml" --config tests-config --runtime "${MONO_EXECUTABLE}" --mono-path "$r" -j a --testsuite-name "runtime" --timeout 300 --disabled "$DISABLED_TESTS" $(cat runtime-test-list.txt)

fi

#! /bin/sh
r="$(pwd)"
MONO_CFG_DIR="$r/runtime/etc"
PATH="$r/runtime/_tmpinst/bin:$PATH"
MONO_EXECUTABLE=${MONO_EXECUTABLE:-"$r/mono"}
MONO_PATH="$r:$r/tests"
export MONO_CFG_DIR MONO_PATH PATH
chmod +x "${MONO_EXECUTABLE}"

if [ "$1" = "run-bcl-tests" ]; then
    exec "${MONO_EXECUTABLE}" --config "$r/runtime/etc/mono/config" --debug xunit.console.exe "tests/$2" -noappdomain -noshadow -parallel none -xml testResults.xml -notrait category=failing -notrait category=nonmonotests -notrait Benchmark=true -notrait category=outerloop -notrait category=nonlinuxtests
fi

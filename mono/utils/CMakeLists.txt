cmake_minimum_required (VERSION 2.8.10)

project (mono-utils)

if(POLICY CMP0026)
cmake_policy(SET CMP0026 NEW)
endif()
if(POLICY CMP0042)
cmake_policy(SET CMP0042 NEW)
endif()

enable_language(C)
enable_language(CXX)

# FIXME: cmake's asm detection is broken when using xcrun.
set (CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
set (CMAKE_ASM_COMPILER_ARG1 "${CMAKE_C_COMPILER_ARG1}")
set (CMAKE_ASM_COMPILER_ID "${CMAKE_C_COMPILER_ID}")
enable_language(ASM)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_CXX_FLAGS} ${MONO_CFLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_CXX_FLAGS} ${MONO_CFLAGS}")
set (CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${MONO_CFLAGS}")
set (CMAKE_MACOSX_RPATH 1)

include_directories (
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../mono
	${CMAKE_CURRENT_SOURCE_DIR}/../../eglib/src
)

list (APPEND MONO_UTILS_SOURCES
	mono-md5.c
	mono-sha1.c
	mono-logger.c
	mono-logger-internals.h
	mono-codeman.c
	dlmalloc.h
	dlmalloc.c
	mono-counters.c
	mono-compiler.h
	mono-complex.h
	mono-dl.c
	mono-dl-windows.c
	mono-dl-darwin.c
	mono-dl-posix.c
	mono-dl.h
	mono-dl-windows-internals.h
	mono-log-windows.c
	mono-log-common.c
	mono-log-posix.c
	mono-log-android.c
	mono-log-darwin.c
	mono-internal-hash.c
	mono-internal-hash.h
	mono-io-portability.c
	mono-io-portability.h
	monobitset.c
	mono-filemap.c
	mono-math.c
	mono-mmap.c
	mono-mmap-windows.c
	mono-mmap.h
	mono-mmap-internals.h
	mono-mmap-windows-internals.h
	mono-os-mutex.h
	mono-coop-mutex.h
	mono-once.h
	mono-lazy-init.h
	mono-networkinterfaces.c
	mono-networkinterfaces.h
	mono-proclib.c
	mono-proclib-windows.c
	mono-proclib.h
	mono-proclib-windows-internals.h
	mono-publib.c
	mono-string.h
	mono-time.c
	mono-time.h
	strenc.h
	strenc.c
	mono-uri.c
	mono-poll.c
	mono-path.c
	mono-os-semaphore.h
	mono-coop-semaphore.h
	mono-sigcontext.h
	mono-stdlib.c
	mono-property-hash.h
	mono-property-hash.c
	mono-value-hash.h
	mono-value-hash.c
	freebsd-elf_common.h
	freebsd-elf32.h
	freebsd-elf64.h
	freebsd-dwarf.h
	dtrace.h
	gc_wrapper.h
	mono-error.c
	mono-error-internals.h
	monobitset.h
	mono-codeman.h
	mono-counters.h
	mono-digest.h
	mono-error.h
	mono-machine.h
	mono-math.h
	mono-membar.h
	mono-path.h
	mono-poll.h
	mono-uri.h
	mono-stdlib.h
	valgrind.h
	mach-support.c
	mach-support.h
	memcheck.h
	mono-context.c
	mono-context.h
	mono-stack-unwinding.h
	hazard-pointer.c
	hazard-pointer.h
	lock-free-queue.c
	lock-free-queue.h
	lock-free-alloc.c
	lock-free-alloc.h
	lock-free-array-queue.c
	lock-free-array-queue.h
	mono-linked-list-set.c
	mono-linked-list-set.h
	mono-threads.c
	mono-threads-state-machine.c
	mono-threads-posix.c
	mono-threads-posix-signals.c
	mono-threads-mach.c
	mono-threads-mach-helper.c
	mono-threads-windows.c
	mono-threads-linux.c
	mono-threads-freebsd.c
	mono-threads-netbsd.c
	mono-threads-openbsd.c
	mono-threads-android.c
	mono-threads.h
	mono-threads-debug.h
	mono-threads-api.h
	mono-threads-coop.c
	mono-threads-coop.h
	mono-tls.h
	mono-tls.c
	linux_magic.h
	mono-memory-model.h
	atomic.h
	atomic.c
	mono-hwcap.h
	mono-hwcap.c
	mono-hwcap-vars.h
	bsearch.h
	bsearch.c
	mono-signal-handler.h
	mono-conc-hashtable.h
	mono-conc-hashtable.c
	sha1.h
	sha1.c
	json.h
	json.c
	networking.c
	networking-posix.c
	networking-fallback.c
	networking-missing.c
	networking-windows.c
	networking.h
	mono-rand.c
	mono-rand-windows.c
	mono-rand.h
	mono-rand-windows-internals.h
	memfuncs.c
	memfuncs.h
	parse.c
	parse.h
	checked-build.c
	checked-build.h
	os-event.h
	refcount.h
)

if (MONO_HOST_WIN32)
	list (APPEND MONO_UTILS_SOURCES
		os-event-win32.c)
else ()
	list (APPEND MONO_UTILS_SOURCES
		os-event-unix.c)
endif ()

if (MONO_ENABLE_DTRACE)
	list (APPEND MONO_UTILS_SOURCES
		mono-dtrace.h)
endif ()

if (NOT MONO_CROSS_COMPILE)
	if (MONO_HOST_X86)
		list (APPEND MONO_UTILS_SOURCES
			mach-support-x86.c
			mono-hwcap-x86.c)

	elseif (MONO_HOST_AMD64)
		list (APPEND MONO_UTILS_SOURCES
			mach-support-amd64.c
			mono-hwcap-x86.c)

	elseif (MONO_HOST_ARM)
		list (APPEND MONO_UTILS_SOURCES
			mach-support-arm.c
			mono-hwcap-arm.c)

	elseif (MONO_HOST_ARM64)
		list (APPEND MONO_UTILS_SOURCES
			mach-support-arm64.c
			mono-hwcap-arm64.c)

	elseif (MONO_HOST_MIPS)
		list (APPEND MONO_UTILS_SOURCES
			mono-hwcap-mips.c)

	elseif (MONO_HOST_POWERPC OR MONO_HOST_POWERPC64)
		list (APPEND MONO_UTILS_SOURCES
			mono-hwcap-ppc.c)

	elseif (MONO_HOST_SPARC OR MONO_HOST_SPARC64)
		list (APPEND MONO_UTILS_SOURCES
			mono-hwcap-sparc.c)

	elseif (MONO_HOST_IA64)
		list (APPEND MONO_UTILS_SOURCES
			mono-hwcap-ia64.c)

	elseif (MONO_HOST_S390X)
		list (APPEND MONO_UTILS_SOURCES
			mono-hwcap-s390x.c)

	endif ()
else ()
	list (APPEND MONO_UTILS_SOURCES
		mach-support-unknown.c
		mach-support-cross.c)

endif ()

add_library (mono-utils-objects OBJECT ${MONO_UTILS_SOURCES})
set_target_properties (mono-utils-objects PROPERTIES
	COMPILE_FLAGS "-Wall -Wsign-compare -Wmissing-field-initializers -ggdb -fvisibility=hidden -fPIC")

if ENABLE_DTRACE

mono-dtrace.h: $(top_srcdir)/data/mono.d
	$(DTRACE) $(DTRACEFLAGS) -h -s $(top_srcdir)/data/mono.d -o $@ || > $@

endif

EXTRA_DIST = \
	os-event-win32.c \
	os-event-unix.c \
	mono-md5.c		\
	mono-sha1.c		\
	mono-logger.c		\
	mono-logger-internals.h	\
	mono-codeman.c		\
	dlmalloc.h      	\
	dlmalloc.c      	\
	mono-counters.c		\
	mono-compiler.h		\
	mono-complex.h		\
	mono-dl.c		\
	mono-dl-windows.c	\
	mono-dl-darwin.c	\
	mono-dl-posix.c		\
	mono-dl.h		\
	mono-dl-windows-internals.h	\
	mono-log-windows.c	\
	mono-log-common.c	\
	mono-log-posix.c	\
	mono-log-android.c \
	mono-log-darwin.c \
	mono-internal-hash.c	\
	mono-internal-hash.h	\
	mono-io-portability.c 	\
	mono-io-portability.h	\
	monobitset.c		\
	mono-filemap.c		\
	mono-math.c  		\
	mono-mmap.c  		\
	mono-mmap-windows.c  		\
	mono-mmap.h  		\
	mono-mmap-internals.h	\
	mono-mmap-windows-internals.h	\
	mono-os-mutex.h		\
	mono-coop-mutex.h		\
	mono-once.h		\
	mono-lazy-init.h		\
	mono-networkinterfaces.c		\
	mono-networkinterfaces.h		\
	mono-proclib.c		\
	mono-proclib-windows.c		\
	mono-proclib.h		\
	mono-proclib-windows-internals.h		\
	mono-publib.c		\
	mono-string.h		\
	mono-time.c  		\
	mono-time.h  		\
	strenc.h		\
	strenc.c		\
	mono-uri.c		\
	mono-poll.c		\
	mono-path.c		\
	mono-os-semaphore.h	\
	mono-coop-semaphore.h		\
	mono-sigcontext.h	\
	mono-stdlib.c 		\
	mono-property-hash.h 	\
	mono-property-hash.c 	\
	mono-value-hash.h 	\
	mono-value-hash.c 	\
	freebsd-elf_common.h 	\
	freebsd-elf32.h		\
	freebsd-elf64.h		\
	freebsd-dwarf.h 	\
	dtrace.h			\
	gc_wrapper.h		\
	mono-error.c	\
	mono-error-internals.h	\
	monobitset.h	\
	mono-codeman.h	\
	mono-counters.h	\
	mono-digest.h	\
	mono-error.h	\
	mono-machine.h	\
	mono-math.h	\
	mono-membar.h	\
	mono-path.h	\
	mono-poll.h	\
	mono-uri.h	\
	mono-stdlib.h	\
	valgrind.h	\
	mach-support.c	\
	mach-support.h  \
	memcheck.h	\
	mono-context.c	\
	mono-context.h	\
	mono-stack-unwinding.h	\
	hazard-pointer.c	\
	hazard-pointer.h	\
	lock-free-queue.c	\
	lock-free-queue.h	\
	lock-free-alloc.c	\
	lock-free-alloc.h	\
	lock-free-array-queue.c	\
	lock-free-array-queue.h	\
	mono-linked-list-set.c	\
	mono-linked-list-set.h	\
	mono-threads.c	\
	mono-threads-state-machine.c	\
	mono-threads-posix.c	\
	mono-threads-posix-signals.c	\
	mono-threads-mach.c	\
	mono-threads-mach-helper.c	\
	mono-threads-windows.c	\
	mono-threads-linux.c	\
	mono-threads-freebsd.c	\
	mono-threads-netbsd.c	\
	mono-threads-openbsd.c	\
	mono-threads-android.c	\
	mono-threads.h	\
	mono-threads-debug.h	\
	mono-threads-api.h	\
	mono-threads-coop.c	\
	mono-threads-coop.h	\
	mono-tls.h	\
	mono-tls.c	\
	linux_magic.h	\
	mono-memory-model.h	\
	atomic.h	\
	atomic.c	\
	mono-hwcap.h	\
	mono-hwcap.c	\
	mono-hwcap-vars.h	\
	bsearch.h	\
	bsearch.c	\
	mono-signal-handler.h	\
	mono-conc-hashtable.h	\
	mono-conc-hashtable.c	\
	sha1.h		\
	sha1.c	\
	json.h	\
	json.c	\
	networking.c	\
	networking-posix.c	\
	networking-fallback.c	\
	networking-missing.c	\
	networking-windows.c	\
	networking.h	\
	mono-rand.c	\
	mono-rand-windows.c	\
	mono-rand.h \
	mono-rand-windows-internals.h \
	memfuncs.c \
	memfuncs.h \
	parse.c \
	parse.h	\
	checked-build.c \
	checked-build.h \
	os-event.h \
	refcount.h \
	mach-support-x86.c \
	mach-support-amd64.c \
	mach-support-arm.c \
	mach-support-arm64.c \
	mach-support-unknown.c \
	mono-hwcap-x86.c \
	mono-hwcap-x86.c \
	mono-hwcap-arm.c \
	mono-hwcap-arm64.c \
	mono-hwcap-mips.c \
	mono-hwcap-ppc.c \
	mono-hwcap-ppc.c \
	mono-hwcap-sparc.c \
	mono-hwcap-sparc.c \
	mono-hwcap-ia64.c \
	mono-hwcap-s390x.c \
	mono-hwcap-cross.c \
	mono-embed.h \
	mono-embed.c \
	CMakeLists.txt

CMAKE_VERBOSE=$(if $(V),VERBOSE=1,)

CMAKE_ARGS = \
	-D CMAKE_INSTALL_PREFIX:PATH=$(prefix) \
	-D SRC_DIR:PATH=$(abs_top_srcdir)/mono/btls \
	-D MONO_CFLAGS:STRING="$(LIBGC_CPPFLAGS) $(GLIB_CFLAGS) $(SHARED_CFLAGS)"

if CROSS_COMPILE
	CMAKE_ARGS += -D MONO_CROSS_COMPILE:BOOL=1
endif

if X86
	CMAKE_ARGS += -D MONO_HOST_X86:BOOL=1
endif

if AMD64
	CMAKE_ARGS += -D MONO_HOST_AMD64:BOOL=1
endif

if ARM
	CMAKE_ARGS += -D MONO_HOST_ARM:BOOL=1
endif

if ARM64
	CMAKE_ARGS += -D MONO_HOST_ARM64:BOOL=1
endif

if MIPS
	CMAKE_ARGS += -D MONO_HOST_MIPS:BOOL=1
endif

if POWERPC
	CMAKE_ARGS += -D MONO_HOST_POWERPC:BOOL=1
endif

if POWERPC64
	CMAKE_ARGS += -D MONO_HOST_POWERPC64:BOOL=1
endif

if SPARC
	CMAKE_ARGS += -D MONO_HOST_SPARC:BOOL=1
endif

if SPARC64
	CMAKE_ARGS += -D MONO_HOST_SPARC64:BOOL=1
endif

if IA64
	CMAKE_ARGS += -D MONO_HOST_IA64:BOOL=1
endif

if S390X
	CMAKE_ARGS += -D MONO_HOST_S390X:BOOL=1
endif

all-local: build/libmono-utils-shared$(libsuffix) build/libmono-utils-static.a

build/libmono-utils-shared$(libsuffix) build/libmono-utils-static.a: build/Makefile
	$(MAKE) -C build $(CMAKE_VERBOSE)

build/Makefile:
	-mkdir -p build
	(cd build && $(CMAKE) $(CMAKE_ARGS) $(abs_top_srcdir)/mono/utils)

clean-local:
	-rm -rf build

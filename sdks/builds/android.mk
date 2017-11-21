
##
# Parameters:
#  $(1): target
#  $(2): arch
#  $(3): platform
#  $(4): abi_name
#  $(5): host_triple
#
# Flags:
#  android_$(1)_CFLAGS
#  android_$(1)_CXXFLAGS
#  android_$(1)_LDFLAGS
define AndroidTargetTemplate

_android_$(1)_AR=$$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-ar
_android_$(1)_AS=$$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-as
_android_$(1)_CC=$$(CCACHE) $$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-clang
_android_$(1)_CXX=$$(CCACHE) $$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-clang++
_android_$(1)_CPP=$$(CCACHE) $$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-cpp -I$$(TOP)/sdks/builds/toolchains/android-$(1)/usr/include
_android_$(1)_CXXCPP=$$(CCACHE) $$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-cpp -I$$(TOP)/sdks/builds/toolchains/android-$(1)/usr/include
_android_$(1)_DLLTOOL=
_android_$(1)_LD=$$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-ld
_android_$(1)_OBJDUMP="$$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-objdump"
_android_$(1)_RANLIB=$$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-ranlib
_android_$(1)_STRIP=$$(TOP)/sdks/builds/toolchains/android-$(1)/bin/$(4)-strip

_android_$(1)_AC_VARS= \
	mono_cv_uscore=yes \
	ac_cv_func_sched_getaffinity=no \
	ac_cv_func_sched_setaffinity=no

_android_$(1)_CFLAGS= \
	$(if $(filter $(RELEASE),true),-O2 -g,-O0 -ggdb3 -fno-omit-frame-pointer) \
	-fstack-protector \
	-DMONODROID=1 \
	$$(android_$(1)_CFLAGS)

_android_$(1)_CXXFLAGS= \
	$(if $(filter $(RELEASE),true),-O2 -g,-O0 -ggdb3 -fno-omit-frame-pointer) \
	-fstack-protector \
	-DMONODROID=1 \
	$$(android_$(1)_CXXFLAGS)

_android_$(1)_LDFLAGS= \
	-z now -z relro -z noexecstack \
	-ldl -lm -llog -lc -lgcc \
	-Wl,-rpath-link=$$(NDK_DIR)/platforms/android-$(3)/arch-$(2)/usr/lib,-dynamic-linker=/system/bin/linker \
	-L$$(NDK_DIR)/platforms/android-$(3)/arch-$(2)/usr/lib \
	$$(android_$(1)_LDFLAGS)

_android_$(1)_CONFIGURE_ENVIRONMENT = \
	AR="$$(_android_$(1)_AR)"	\
	AS="$$(_android_$(1)_AS)"	\
	CC="$$(_android_$(1)_CC)"	\
	CFLAGS="$$(_android_$(1)_CFLAGS)" \
	CXX="$$(_android_$(1)_CXX)" \
	CXXFLAGS="$$(_android_$(1)_CXXFLAGS) " \
	CPP="$$(_android_$(1)_CPP) $$(_android_$(1)_CPPFLAGS)"	\
	CXXCPP="$$(_android_$(1)_CXXCPP)"	\
	DLLTOOL="$$(_android_$(1)_DLLTOOL)" \
	LD="$$(_android_$(1)_LD)"	\
	LDFLAGS="$$(_android_$(1)_LDFLAGS)"	\
	OBJDUMP="$$(_android_$(1)_OBJDUMP)" \
	STRIP="$$(_android_$(1)_STRIP)" \
	RANLIB="$$(_android_$(1)_RANLIB)"

_android_$(1)_CONFIGURE_FLAGS= \
	--host=$(5) \
	--cache-file=$$(TOP)/sdks/builds/android-$(1)-$$(CONFIGURATION).config.cache \
	--prefix=$$(TOP)/sdks/out/android-$(1)-$$(CONFIGURATION) \
	--disable-boehm \
	--disable-executables \
	--disable-iconv \
	--disable-mcs-build \
	--disable-nls \
	--enable-dynamic-btls \
	--enable-maintainer-mode \
	--enable-minimal=ssa,portability,attach,verifier,full_messages,sgen_remset,sgen_marksweep_par,sgen_marksweep_fixed,sgen_marksweep_fixed_par,sgen_copying,logging,security,shared_handles,interpreter \
	--with-btls-android-ndk=$$(NDK_DIR) \
	--with-sigaltstack=yes \
	--with-tls=pthread \
	--without-ikvm-native

.stamp-android-$(1)-toolchain:
	python "$$(NDK_DIR)/build/tools/make_standalone_toolchain.py" --verbose --force --api=$(3) --arch=$(2) --install-dir=$$(TOP)/sdks/builds/toolchains/android-$(1)
	touch $$@

.stamp-android-$(1)-$$(CONFIGURATION)-configure:
	mkdir -p $$(TOP)/sdks/builds/android-$(1)-$$(CONFIGURATION)
	cd $$(TOP)/sdks/builds/android-$(1)-$$(CONFIGURATION) && $$(TOP)/configure $$(_android_$(1)_AC_VARS) $$(_android_$(1)_CONFIGURE_ENVIRONMENT) $$(_android_$(1)_CONFIGURE_FLAGS)
	touch $$@

.PHONY: .stamp-android-$(1)-configure
.stamp-android-$(1)-configure: .stamp-android-$(1)-$$(CONFIGURATION)-configure

.PHONY: build-custom-android-$(1)
build-custom-android-$(1):
	$$(MAKE) -C android-$(1)-$$(CONFIGURATION)

.PHONY: setup-custom-android-$(1)
setup-custom-android-$(1):
	mkdir -p $$(TOP)/sdks/out/android-$(1)-$$(CONFIGURATION)

.PHONY: package-android-$(1)-$$(CONFIGURATION)
package-android-$(1)-$$(CONFIGURATION):
	$$(MAKE) -C $$(TOP)/sdks/builds/android-$(1)-$$(CONFIGURATION)/mono install
	$$(MAKE) -C $$(TOP)/sdks/builds/android-$(1)-$$(CONFIGURATION)/support install

.PHONY: package-android-$(1)
package-android-$(1):
	$$(MAKE) package-android-$(1)-$$(CONFIGURATION)

.PHONY: clean-android-$(1)-$$(CONFIGURATION)
clean-android-$(1)-$$(CONFIGURATION):
	rm -rf .stamp-android-$(1)-toolchain .stamp-android-$(1)-$$(CONFIGURATION)-configure $$(TOP)/sdks/builds/toolchains/android-$(1) $$(TOP)/sdks/builds/android-$(1)-$$(CONFIGURATION) $$(TOP)/sdks/builds/android-$(1)-$$(CONFIGURATION).config.cache $$(TOP)/sdks/out/android-$(1)-$$(CONFIGURATION)

.PHONY: clean-android-$(1)
clean-android-$(1):
	$$(MAKE) clean-android-$(1)-$$(CONFIGURATION)

TARGETS += android-$(1)

endef

## android-armeabi
android_armeabi_CFLAGS=-D__POSIX_VISIBLE=201002 -DSK_RELEASE -DNDEBUG -UDEBUG -fpic -march=armv5te
android_armeabi_CXXFLAGS=-D__POSIX_VISIBLE=201002 -DSK_RELEASE -DNDEBUG -UDEBUG -fpic -march=armv5te
android_armeabi_LDFLAGS=-Wl,--fix-cortex-a8
$(eval $(call AndroidTargetTemplate,armeabi,arm,9,arm-linux-androideabi,armv5-linux-androideabi))

## android-armeabi-v7a
android_armeabi-v7a_CFLAGS=-D__POSIX_VISIBLE=201002 -DSK_RELEASE -DNDEBUG -UDEBUG -fpic -march=armv7-a -mtune=cortex-a8 -mfpu=vfp -mfloat-abi=softfp
android_armeabi-v7a_CXXFLAGS=-D__POSIX_VISIBLE=201002 -DSK_RELEASE -DNDEBUG -UDEBUG -fpic -march=armv7-a -mtune=cortex-a8 -mfpu=vfp -mfloat-abi=softfp
android_armeabi-v7a_LDFLAGS=-Wl,--fix-cortex-a8
$(eval $(call AndroidTargetTemplate,armeabi-v7a,arm,9,arm-linux-androideabi,armv5-linux-androideabi))

## android-arm64-v8a
android_arm64-v8a_CFLAGS=-D__POSIX_VISIBLE=201002 -DSK_RELEASE -DNDEBUG -UDEBUG -fpic -DL_cuserid=9 -DANDROID64
android_arm64-v8a_CXXFLAGS=-D__POSIX_VISIBLE=201002 -DSK_RELEASE -DNDEBUG -UDEBUG -fpic -DL_cuserid=9 -DANDROID64
$(eval $(call AndroidTargetTemplate,arm64-v8a,arm64,21,aarch64-linux-android,aarch64-linux-android))

## android-x86
$(eval $(call AndroidTargetTemplate,x86,x86,9,i686-linux-android,i686-linux-android))

## android-x86_64
android_x86_64_CFLAGS=-DL_cuserid=9
android_x86_64_CXXFLAGS=-DL_cuserid=9
$(eval $(call AndroidTargetTemplate,x86_64,x86_64,21,x86_64-linux-android,x86_64-linux-android))

##
# Parameters:
#  $(1): target
#  $(2): arch
#  $(3): target_triple
#
# Flags:
#  android_$(1)_CFLAGS
#  android_$(1)_CXXFLAGS
define AndroidCrossTemplate

_android_$(1)_AR=ar
_android_$(1)_AS=as
_android_$(1)_CC=$$(CCACHE) clang -m32
_android_$(1)_CXX=$$(CCACHE) clang++ -m32
_android_$(1)_CPP=$$(CCACHE) cpp
_android_$(1)_CXXCPP=$$(CCACHE) cpp
_android_$(1)_LD=ld
_android_$(1)_OBJDUMP=objdump
_android_$(1)_RANLIB=ranlib
_android_$(1)_STRIP=strip

_android_$(1)_AC_VARS=

_android_$(1)_CFLAGS= \
	$(if $(filter $(RELEASE),true),,-DDEBUG_CROSS) \
	$(if $(filter $(shell uname),Darwin),-mmacosx-version-min=10.9) \
	-DXAMARIN_PRODUCT_VERSION=0 \
	$$(android_$(1)_CFLAGS)

_android_$(1)_CXXFLAGS= \
	$(if $(filter $(RELEASE),true),,-DDEBUG_CROSS) \
	$(if $(filter $(shell uname),Darwin),-mmacosx-version-min=10.9) \
	-DXAMARIN_PRODUCT_VERSION=0 \
	-stdlib=libc++ \
	$$(android_$(1)_CXXFLAGS)

_android_$(1)_CONFIGURE_ENVIRONMENT = \
	AR="$$(_android_$(1)_AR)"	\
	AS="$$(_android_$(1)_AS)"	\
	CC="$$(_android_$(1)_CC)"	\
	CFLAGS="$$(_android_$(1)_CFLAGS)" \
	CXX="$$(_android_$(1)_CXX)" \
	CXXFLAGS="$$(_android_$(1)_CXXFLAGS) " \
	CPP="$$(_android_$(1)_CPP) $$(_android_$(1)_CPPFLAGS)"	\
	CXXCPP="$$(_android_$(1)_CXXCPP)"	\
	LD="$$(_android_$(1)_LD)"	\
	OBJDUMP="$$(_android_$(1)_OBJDUMP)" \
	STRIP="$$(_android_$(1)_STRIP)" \
	RANLIB="$$(_android_$(1)_RANLIB)"

_android_$(1)_CONFIGURE_FLAGS= \
	--build=$(if $(filter $(shell uname),Darwin),i386-apple-darwin11.2.0,$(error FIXME: add support for Linux)) \
	--target=$(3) \
	--cache-file=$$(TOP)/sdks/builds/android-$(1).config.cache \
	--prefix=$$(TOP)/sdks/out/android-$(1) \
	--disable-boehm \
	--disable-mcs-build \
	--disable-nls \
	--enable-extension-module \
	--enable-maintainer-mode \
	--with-cross-offsets=$(2)-linux-android.h \
	--with-tls=pthread

#FIXME we need to compile llvm
# _android_$(1)_CONFIGURE_FLAGS += --enable-llvm --with-llvm=

.stamp-android-$(1)-toolchain:
	touch $$@

.stamp-android-$(1)-configure:
	mkdir -p $$(TOP)/sdks/builds/android-$(1)
	cd $$(TOP)/sdks/builds/android-$(1) && $$(TOP)/configure $$(_android_$(1)_AC_VARS) $$(_android_$(1)_CONFIGURE_ENVIRONMENT) $$(_android_$(1)_CONFIGURE_FLAGS)
	touch $$@

$$(TOP)/sdks/builds/android-$(1)/$(2)-linux-android.h: .stamp-android-$(1)-configure $$(TOP)/tools/offsets-tool/MonoAotOffsetsDumper.exe
	cd $$(TOP)/sdks/builds/android-$(1) && \
		MONO_PATH=$(TOP)/tools/offsets-tool/CppSharp/osx_32 \
			mono --arch=32 --debug $$(TOP)/tools/offsets-tool/MonoAotOffsetsDumper.exe \
				--mono $$(TOP) --abi $(2)-linux-android --android-ndk $$(NDK_DIR) --android-api $(4) --out $$(TOP)/sdks/builds/android-$(1)/ --targetdir $$(TOP)/sdks/builds/android-$(1)

build-android-$(1): $$(TOP)/sdks/builds/android-$(1)/$(2)-linux-android.h

.PHONY: package-android-$(1)
package-android-$(1):
	$$(MAKE) -C $$(TOP)/sdks/builds/android-$(1)/mono install
	$$(MAKE) -C $$(TOP)/sdks/builds/android-$(1)/support install

.PHONY: clean-android-$(1)
clean-android-$(1):
	rm -rf .stamp-android-$(1)-toolchain $$(TOP)/sdks/builds/android-$(1) $$(TOP)/sdks/builds/android-$(1).config.cache $$(TOP)/sdks/out/android-$(1)

TARGETS += android-$(1)

endef

$(eval $(call AndroidCrossTemplate,cross-arm,armv5-none,armv5-linux-androideabi,9))

$(eval $(call AndroidCrossTemplate,cross-arm64,aarch64-v8a,aarch64-v8a-linux-android,21))

$(eval $(call AndroidCrossTemplate,cross-x86,i686-none,i686-linux-android,9))

$(eval $(call AndroidCrossTemplate,cross-x86_64,x86_64-none,x86_64-linux-android,21))

thisdir = class/corlib/CommonCrypto
SUBDIRS =
include ../../../build/rules.make

.NOTPARALLEL:
all:
	MONO_PATH="$(topdir)/class/lib/$(BUILDPROFILE)-$(BUILDPLATFORM)$(PLATFORM_PATH_SEPARATOR)$$MONO_PATH" $(RUNTIME) $(RUNTIME_FLAGS) $(topdir)/class/lib/$(BUILDPROFILE)-$(BUILDPLATFORM)/commoncryptogenerator.exe


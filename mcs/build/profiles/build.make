# -*- makefile -*-

BUILD_PROFILE = basic

BOOTSTRAP_MCS = MONO_PATH="$(topdir)/class/lib/$(BUILD_PROFILE)$(PLATFORM_PATH_SEPARATOR)$$MONO_PATH" $(INTERNAL_CSC)
MCS = $(BOOTSTRAP_MCS)

# nuttzing!

profile-check:
	@:

DEFAULT_REFERENCES = -r:$(topdir)/class/lib/$(PROFILE)/mscorlib.dll
PROFILE_MCS_FLAGS = -d:NET_4_0 -d:NET_4_5 -d:MONO -d:WIN_PLATFORM -nowarn:1699 -nostdlib $(DEFAULT_REFERENCES)

NO_SIGN_ASSEMBLY = yes
NO_TEST = yes
NO_INSTALL = yes

FRAMEWORK_VERSION = 4.5

# -*- makefile -*-

include $(topdir)/build/profiles/xbuild_12.make

PLATFORMS = linux macos win32

PROFILE_MCS_FLAGS += -d:XBUILD_14

XBUILD_VERSION = 14.0

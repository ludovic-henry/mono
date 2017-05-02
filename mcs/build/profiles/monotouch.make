include $(topdir)/build/profiles/monotouch_runtime.make

PLATFORMS = ios

PROFILE_MCS_FLAGS += \
	-d:FULL_AOT_RUNTIME

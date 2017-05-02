thisdir = class/System.ServiceModel.Activation
SUBDIRS = 
include ../../build/rules.make

LIBRARY = System.ServiceModel.Activation.dll

LIB_REFS = System.Core System plainservice/System.ServiceModel
LIB_MCS_FLAGS =

TEST_MCS_FLAGS =
TEST_LIB_REFS = System System.Core

include ../../build/library.make

# $(1): platform
define BuildDependencies
servicemodel = $(call the_libdir_base,$(1))plainservice/System.ServiceModel.dll

$(call the_libdir_base,$(1))$(LIBRARY): $(servicemodel)

$(servicemodel): 
	(cd ../System.ServiceModel; $(MAKE) $@)

.NOTPARALLEL: $(servicemodel)
endef

$(foreach platform,$(or $(PLATFORMS),$(BUILD_PLATFORM)),$(eval $(call BuildDependencies,$(platform))))

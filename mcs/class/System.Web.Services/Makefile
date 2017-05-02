thisdir = class/System.Web.Services
SUBDIRS = 
include ../../build/rules.make

LIBRARY = System.Web.Services.dll
ifdef MOBILE_PROFILE
LIB_REFS = System System.Xml
LIB_MCS_FLAGS = \
	-nowarn:649 -nowarn:168		\
	-resource:System.Web.Services.Description/wsdl-1.1.xsd,wsdl-1.1.xsd \
	-resource:System.Web.Services.Description/wsdl-1.1-soap.xsd,wsdl-1.1-soap.xsd \
	-resource:System.Web.Services.Description/web-reference.xsd,web-reference.xsd
else
LIB_REFS = System System.EnterpriseServices System.Xml System.Data
LIB_MCS_FLAGS = -nowarn:168,612,618,649 -d:MONO_BROKEN_CONFIGURATION_DLL

ifndef NO_SYSTEM_WEB_DEPENDENCY
LIB_REFS += plainweb/System.Web
endif

ifndef NO_SYSTEM_DESIGN_DEPENDENCY
LIB_REFS += plaindesign/System.Design
endif

ifndef NO_SYSTEM_DIRECTORY_SERVICES_DEPENDENCY
LIB_REFS += System.DirectoryServices
endif

LIB_REFS += System.Configuration
LIB_MCS_FLAGS += \
	-resource:System.Web.Services.Description/wsdl-1.1.xsd,wsdl-1.1.xsd \
	-resource:System.Web.Services.Description/wsdl-1.1-soap.xsd,wsdl-1.1-soap.xsd \
	-resource:System.Web.Services.Description/web-reference.xsd,web-reference.xsd
endif

TEST_MCS_FLAGS = $(LIB_MCS_FLAGS) -nowarn:618

ifndef NO_THREAD_ABORT
TEST_MCS_FLAGS += -d:MONO_FEATURE_THREAD_ABORT
endif

ifndef NO_THREAD_SUSPEND_RESUME
TEST_MCS_FLAGS += -d:MONO_FEATURE_THREAD_SUSPEND_RESUME
endif

EXTRA_DISTFILES = \
	System.Web.Services.Description/web-reference.xsd	\
	System.Web.Services.Description/wsdl-1.1.xsd		\
	System.Web.Services.Description/wsdl-1.1-soap.xsd	\
	System.Web.Services.Description/wsdl.genxs		\
	Test/System.Web.Services.Description/*.wsdl

include ../../build/library.make

# $(1): platform
define BuildDependencies
ifndef NO_SYSTEM_WEB_DEPENDENCY
system_web_deps = $(call the_libdir_base,$(1))plainweb/System.Web.dll
endif

ifndef NO_SYSTEM_DESIGN_DEPENDENCY
system_design_deps = $(call the_libdir_base,$(1))plaindesign/System.Design.dll
endif

$(call the_libdir_base,$(1))$(LIBRARY): $(system_web_deps) $(system_design_deps)

$(system_web_deps):
	(cd ../System.Web; $(MAKE) $@)

$(system_design_deps):
	(cd ../System.Design; $(MAKE) $@)

.NOTPARALLEL: $(system_web_deps) $(system_design_deps)
endef

$(foreach platform,$(or $(PLATFORMS),$(BUILD_PLATFORM)),$(eval $(call BuildDependencies,$(platform))))

# -*- makefile -*-
#
# The rules for building our class libraries.
#
# The NO_TEST stuff is not too pleasant but whatcha
# gonna do.

# All the dep files now land in the same directory so we
# munge in the library name to keep the files from clashing.

# The including makefile can set the following variables:
# LIB_MCS_FLAGS - Command line flags passed to mcs.
# LIB_REFS      - This should be a space separated list of assembly names which are added to the mcs
#                 command line.
#

# All dependent libs become dependent dirs for parallel builds
# Have to rename to handle differences between assembly/directory names
DEP_LIBS=$(patsubst System.Xml,System.XML,$(LIB_REFS))

_FILTER_OUT = $(foreach x,$(2),$(if $(findstring $(1),$(x)),,$(x)))

LIB_REFS_FULL = $(call _FILTER_OUT,=, $(LIB_REFS))
LIB_REFS_ALIAS = $(filter-out $(LIB_REFS_FULL),$(LIB_REFS))

LIB_MCS_FLAGS += $(patsubst %,-r:$(topdir)/class/lib/$(PROFILE)/%.dll,$(LIB_REFS_FULL))
LIB_MCS_FLAGS += $(patsubst %,-r:%.dll, $(subst =,=$(topdir)/class/lib/$(PROFILE)/,$(LIB_REFS_ALIAS)))

ifndef LIBRARY_NAME
LIBRARY_NAME = $(LIBRARY)
endif

ifdef LIBRARY_COMPAT
lib_dir = compat
else
lib_dir = lib
endif

# $(1): platform
libdir_base = $(topdir)/class/$(lib_dir)/$(PROFILE)$(if $(PLATFORMS),-$(1))/$(if $(LIBRARY_SUBDIR),$(LIBRARY_SUBDIR)/)

ifdef RESOURCE_STRINGS
ifneq (basic, $(PROFILE))
RESOURCE_STRINGS_FILES += $(RESOURCE_STRINGS:%=--resourcestrings:%)
endif
endif

#
# The bare directory contains the plain versions of System and System.Xml
#
# $(1): platform
bare_libdir = $(call libdir_base,$(1))bare/

#
# The secxml directory contains the System version that depends on 
# System.Xml and Mono.Security
#
# $(1): platform
secxml_libdir = $(call libdirbase,$(1))secxml

# $(1): platform
the_libdir = $(call libdir_base,$(1))$(intermediate)

# $(1): platform
build_libdir = $(call the_libdir,$(1))$(if $(LIBRARY_USE_INTERMEDIATE_FILE),tmp/)

# $(1): platform
the_lib = $(call the_libdir,$(1))$(LIBRARY_NAME)

# $(1): platform
build_lib = $(call build_libdir,$(1))$(LIBRARY_NAME)

ifdef NO_SIGN_ASSEMBLY
SN = :
else
ifeq ("$(SN)","")
sn = $(topdir)/class/lib/$(BUILD_PROFILE)/sn.exe
SN = MONO_PATH="$(topdir)/class/lib/$(BUILD_PROFILE)$(PLATFORM_PATH_SEPARATOR)$$MONO_PATH" $(RUNTIME) $(RUNTIME_FLAGS) $(sn) -q
endif
endif

ifeq ($(BUILD_PLATFORM), win32)
GACDIR = `cygpath -w $(mono_libdir)`
GACROOT = `cygpath -w $(DESTDIR)$(mono_libdir)`
test_flags += -d:WINDOWS
else
GACDIR = $(mono_libdir)
GACROOT = $(DESTDIR)$(mono_libdir)
endif

ifndef NO_BUILD
all-local: $(extra_targets)
endif

ifeq ($(LIBRARY_COMPILE),$(BOOT_COMPILE))
is_boot=true
else
is_boot=false
endif

csproj-local: csproj-library csproj-test

intermediate_clean=$(subst /,-,$(intermediate))

csproj-library:
	$(foreach platform,$(or $(PLATFORMS),$(BUILD_PLATFORM)), \
		config_file=`basename $(LIBRARY) .dll`-$(intermediate_clean)$(PROFILE)$(if $(PLATFORMS),$(platform)).input; \
		case "$(thisdir)" in *"Facades"*) \=Facades_$$config_file;; *"legacy"*) config_file=legacy_$$config_file;; esac; \
		echo $(thisdir):$$config_file >> $(topdir)/../msvc/scripts/order; \
		(echo $(is_boot); \
		echo $(USE_MCS_FLAGS) $(LIBRARY_FLAGS) $(LIB_MCS_FLAGS); \
		echo $(LIBRARY_NAME); \
		echo $(BUILT_SOURCES_cmdline); \
		echo $(call build_lib,$(platform)); \
		echo $(FRAMEWORK_VERSION); \
		echo $(PROFILE); \
		$(if $(PLATFORMS),echo $(platform);) \
		echo $(RESOURCE_DEFS); \
		echo $(response)) > $(topdir)/../msvc/scripts/inputs/$$config_file)

csproj-test:

install-local: all-local
test-local: all-local
uninstall-local:

ifdef NO_INSTALL
install-local uninstall-local:
	@:

else

ifdef LIBRARY_INSTALL_DIR
install-local:
	$(MKINSTALLDIRS) $(DESTDIR)$(LIBRARY_INSTALL_DIR)
	$(INSTALL_LIB) $(call the_lib,$(BUILD_PLATFORM)) $(DESTDIR)$(LIBRARY_INSTALL_DIR)/$(LIBRARY_NAME)
	test ! -f $(call the_lib,$(BUILD_PLATFORM)).mdb || $(INSTALL_LIB) $(call the_lib,$(BUILD_PLATFORM)).mdb $(DESTDIR)$(LIBRARY_INSTALL_DIR)/$(LIBRARY_NAME).mdb
	test ! -f $(patsubst %.dll,%.pdb,$(call the_lib,$(BUILD_PLATFORM))) || $(INSTALL_LIB) $(patsubst %.dll,%.pdb,$(call the_lib,$(BUILD_PLATFORM))) $(DESTDIR)$(LIBRARY_INSTALL_DIR)/$(LIBRARY_NAME:.dll=.pdb)
ifdef PLATFORM_AOT_SUFFIX
	test ! -f $(call the_lib,$(BUILD_PLATFORM))$(PLATFORM_AOT_SUFFIX) || $(INSTALL_LIB) $(call the_lib,$(BUILD_PLATFORM))$(PLATFORM_AOT_SUFFIX) $(DESTDIR)$(LIBRARY_INSTALL_DIR)
endif

uninstall-local:
	-rm -f $(DESTDIR)$(LIBRARY_INSTALL_DIR)/$(LIBRARY_NAME) $(DESTDIR)$(LIBRARY_INSTALL_DIR)/$(LIBRARY_NAME).mdb $(DESTDIR)$(LIBRARY_INSTALL_DIR)/$(LIBRARY_NAME:.dll=.pdb)

else

# If RUNTIME_HAS_CONSISTENT_GACDIR is set, it implies that the internal GACDIR
# of the runtime is the same as the GACDIR we want.  So, we don't need to pass it
# to gacutil.  Note that the GACDIR we want may not be the same as the value of
# GACDIR set above, since the user could have overridden the value of $(prefix).
#
# This makes a difference only when we're building from the mono/ tree, since we
# have to ensure that the internal GACDIR of the in-tree runtime matches where we
# install the DLLs.

ifndef RUNTIME_HAS_CONSISTENT_GACDIR
gacdir_flag = /gacdir $(GACDIR)
endif

ifndef LIBRARY_PACKAGE
ifdef LIBRARY_COMPAT
LIBRARY_PACKAGE = compat-$(FRAMEWORK_VERSION)
else
LIBRARY_PACKAGE = $(FRAMEWORK_VERSION)
endif
endif

ifneq (none, $(LIBRARY_PACKAGE))
package_flag = /package $(LIBRARY_PACKAGE)
endif

install-local: $(gacutil)
	$(GACUTIL) /i $(call the_lib,$(BUILD_PLATFORM)) /f $(gacdir_flag) /root $(GACROOT) $(package_flag)

uninstall-local: $(gacutil)
	-$(GACUTIL) /u $(LIBRARY_NAME:.dll=) $(gacdir_flag) /root $(GACROOT) $(package_flag)

endif # LIBRARY_INSTALL_DIR
endif # NO_INSTALL

clean-local:
	-rm -f $(tests_CLEAN_FILES) $(library_CLEAN_FILES) $(CLEAN_FILES)

test-local run-test-local run-test-ondotnet-local:
	@:

#
# RESOURCES_DEFS is a list of ID,FILE pairs separated by spaces
# for each of those, generate a rule that produces ID.resource from
# FILE using the resgen tool, adds the generated file to CLENA_FILES and
# passes the resource to the compiler
#
ccomma = ,
define RESOURCE_template
$(1).resources: $(2)
	$(RESGEN) "$$<" "$$@"

GEN_RESOURCE_DEPS += $(1).resources
GEN_RESOURCE_FLAGS += -resource:$(1).resources
CLEAN_FILES += $(1).resources
DIST_LISTED_RESOURCES += $(2)
endef

ifdef RESOURCE_DEFS
$(foreach pair,$(RESOURCE_DEFS), $(eval $(call RESOURCE_template,$(word 1, $(subst $(ccomma), ,$(pair))), $(word 2, $(subst $(ccomma), ,$(pair))))))
endif

DISTFILES = $(wildcard *$(LIBRARY)*.sources) $(EXTRA_DISTFILES) $(DIST_LISTED_RESOURCES)

ASSEMBLY      = $(LIBRARY)
ASSEMBLY_EXT  = .dll
the_assembly  = $(call the_lib,$(BUILD_PLATFORM))
include $(topdir)/build/tests.make

ifdef HAVE_CS_TESTS
DISTFILES += $(test_sourcefile)

csproj-test:
	config_file=`basename $(LIBRARY) .dll`-tests-$(PROFILE).input; \
	echo $(thisdir):$$config_file >> $(topdir)/../msvc/scripts/order; \
	(echo false; \
	echo $(USE_MCS_FLAGS) -r:$(the_assembly) $(TEST_MCS_FLAGS); \
	echo $(test_lib); \
	echo $(BUILT_SOURCES_cmdline); \
	echo $(test_lib); \
	echo $(FRAMEWORK_VERSION); \
	echo $(PROFILE); \
	echo ""; \
	echo $(test_response)) > $(topdir)/../msvc/scripts/inputs/$$config_file

endif

# make dist will collect files in .sources files from all profiles
dist-local: dist-default
	subs=' ' ; \
	for f in `$(topdir)/tools/removecomments.sh $(filter-out $(wildcard *_test.dll.sources) $(wildcard *_xtest.dll.sources) $(wildcard *exclude.sources),$(wildcard *.sources))` $(TEST_FILES) ; do \
	  case $$f in \
	  ../*) : ;; \
	  *.g.cs) : ;; \
	  *) dest=`dirname "$$f"` ; \
	     case $$subs in *" $$dest "*) : ;; *) subs=" $$dest$$subs" ; $(MKINSTALLDIRS) $(distdir)/$$dest ;; esac ; \
	     cp -p "$$f" $(distdir)/$$dest || exit 1 ;; \
	  esac ; done ; \
	for d in . $$subs ; do \
	  case $$d in .) : ;; *) test ! -f $$d/ChangeLog || cp -p $$d/ChangeLog $(distdir)/$$d ;; esac ; done

ifndef LIBRARY_COMPILE
LIBRARY_COMPILE = $(CSCOMPILE)
endif

ifndef LIBRARY_SNK
LIBRARY_SNK = $(topdir)/class/mono.snk
endif

ifdef BUILT_SOURCES
library_CLEAN_FILES += $(BUILT_SOURCES)
ifeq (cat, $(PLATFORM_CHANGE_SEPARATOR_CMD))
BUILT_SOURCES_cmdline = $(BUILT_SOURCES)
else
BUILT_SOURCES_cmdline = `echo $(BUILT_SOURCES) | $(PLATFORM_CHANGE_SEPARATOR_CMD)`
endif
endif

# The library

# If the directory contains the per profile include file, generate list file.

# $(1): platform
define BuildLibrary
PROFILE_$(1)_sources = $$(firstword $$(if $$(PLATFORMS),$$(wildcard $(1)_$$(PROFILE)_$$(LIBRARY).sources)) $$(wildcard $$(PROFILE)_$$(LIBRARY).sources) $$(wildcard $$(LIBRARY).sources))
PROFILE_$(1)_excludes = $$(firstword $$(if $$(PLATFORMS),$$(wildcard $(1)_$$(PROFILE)_$$(LIBRARY).exclude.sources)) $$(wildcard $$(PROFILE)_$$(LIBRARY).exclude.sources))

$(1)_sourcefile = $$(depsdir)/$(1)_$$(PROFILE)_$$(LIBRARY).sources
$$($(1)_sourcefile): $$(PROFILE_$(1)_sources) $$(PROFILE_$(1)_excludes) $$(topdir)/build/gensources.sh
	@echo Creating the per profile and platform list $$@ ...
	$$(SHELL) $$(topdir)/build/gensources.sh $$@ '$$(PROFILE_$(1)_sources)' '$$(PROFILE_$(1)_excludes)'

library_CLEAN_FILES += $(1)_sourcefile

PLATFORM_$(1)_excludes = $$(wildcard $$(LIBRARY).$(1)-excludes)

$(1)_response = $$(depsdir)/$(1)_$$(PROFILE)_$$(LIBRARY_SUBDIR)_$$(LIBRARY).response
ifdef $$(PLATFORM_$(1)_excludes)
$$($(1)_response): $$($(1)_sourcefile) $$(PLATFORM_$(1)_excludes)
	@echo Filtering $$($(1)_sourcefile) to $$@ ...
	@sort $$($(1)_sourcefile) $$(PLATFORM_$(1)_excludes) | uniq -u | $$(PLATFORM_CHANGE_SEPARATOR_CMD) >$$@
else
$$($(1)_response): $$($(1)_sourcefile)
	@echo Converting $$($(1)_sourcefile) to $$@ ...
	@cat $$($(1)_sourcefile) | $$(PLATFORM_CHANGE_SEPARATOR_CMD) >$$@
endif

library_CLEAN_FILES += $(1)_response

$$($(1)_response): $$(topdir)/build/platform-library.make $$(depsdir)/.stamp

ifndef NO_BUILD
all-local: $($(1)_makefrag)
endif

$(1)_makefrag = $$(depsdir)/$(1)_$$(PROFILE)_$$(LIBRARY_SUBDIR)_$$(LIBRARY).makefrag
$($(1)_makefrag): $$($(1)_sourcefile)
#	@echo Creating $$@ ...
	@sed 's,^,$$(call build_lib,$(1)): ,' $$< >$$@
	@if test ! -f $$($(1)_sourcefile).makefrag; then :; else \
	   cat $$($(1)_sourcefile).makefrag >> $$@ ; \
	   echo '$$@: $$($(1)_sourcefile).makefrag' >> $$@; \
	   echo '$$($(1)_sourcefile).makefrag:' >> $$@; fi

library_CLEAN_FILES += $($(1)_makefrag)

-include $($(1)_makefrag)

$$(call the_lib,$(1)): $$(call the_libdir,$(1))/.stamp

ifndef NO_BUILD

$$(call build_lib,$(1)): $$($(1)_response) $$(sn) $$(BUILT_SOURCES) $$(patsubst %,%/.stamp,$$(call build_libdir$(1))) $$(GEN_RESOURCE_DEPS)
	$$(LIBRARY_COMPILE) $$(LIBRARY_FLAGS) $$(LIB_MCS_FLAGS) $$(GEN_RESOURCE_FLAGS) -target:library -out:$$@ $$(BUILT_SOURCES_cmdline) @$$($(1)_response)
ifdef RESOURCE_STRINGS_FILES
	$$(Q) $$(STRING_REPLACER) $$(RESOURCE_STRINGS_FILES) $$@
endif
	$$(Q) $$(SN) -R $$@ $$(LIBRARY_SNK)

ifdef LIBRARY_USE_INTERMEDIATE_FILE
$$(call the_lib,$(1)): $$(call build_lib,$(1))
	$$(Q) cp $$(call build_lib,$(1)) $$@
	echo $$(Q) $$(SN) -v $$@
	$$(Q) test ! -f $$<.mdb || mv $$<.mdb $$@.mdb
	$$(Q) test ! -f $$(patsubst %.dll,%.pdb,$$<) || mv $$(patsubst %.dll,%.pdb,$$<) $$(patsubst %.dll,%.pdb,$$@)
endif

all-local: $$(call the_lib,$(1))

endif # NO_BUILD

ifdef PLATFORM_AOT_SUFFIX
$$(call the_lib,$(1))$$(PLATFORM_AOT_SUFFIX): $$(call the_lib,$(1))
	$$(Q_AOT) MONO_PATH='$$(call the_libdir_base,$(1))' > $(1)_$$(PROFILE)_$$(LIBRARY_NAME)_aot.log 2>&1 $$(RUNTIME) $$(AOT_BUILD_FLAGS) --debug $$<

all-local-aot: $$(call the_lib,$(1))$$(PLATFORM_AOT_SUFFIX)

library_CLEAN_FILES += $(1)_$$(PROFILE)_$$(LIBRARY_NAME)_aot.log

library_CLEAN_FILES += $$(call the_lib,$(1)) $$(call the_lib,$(1))$$(PLATFORM_AOT_SUFFIX) $$(call the_lib,$(1)).mdb $$(patsubst %.dll,%.pdb,$$(call the_lib,$(1)))
library_CLEAN_FILES += $$(call build_lib,$(1)) $$(call build_lib,$(1))$$(PLATFORM_AOT_SUFFIX) $$(call build_lib,$(1)).mdb $$(patsubst %.dll,%.pdb,$$(call build_lib,$(1)))
endif

endef # BuildLibrary

$(foreach platform,$(or $(PLATFORMS),$(BUILD_PLATFORM)),$(eval $(call BuildLibrary,$(platform))))

# for now, don't give any /lib flags or set MONO_PATH, since we
# give a full path to the assembly.

## Include corcompare stuff
include $(topdir)/build/corcompare.make

ifndef NO_BUILD
all-local: $(test_makefrag) $(btest_makefrag)
endif

$(makefrag) $(test_response) $(test_makefrag) $(btest_response) $(btest_makefrag): $(topdir)/build/library.make $(depsdir)/.stamp

## Documentation stuff

# $(1): platform
define UpdateDocumentation

$(call the_libdir,$(1))/.doc-stamp: $(call the_lib,$(1))
	$(MDOC_UP) $(call the_lib,$(1))
	@echo "doc-stamp" > $@

doc-update-local: $(call the_libdir,$(1))/.doc-stamp

endef # UpdateDocumentation

$(foreach platform,$(or $(PLATFORMS),$(BUILD_PLATFORM)),$(eval $(call UpdateDocumentation,$(platform))))

# Need to be here so it comes after the definition of DEP_DIRS/DEP_LIBS
gen-deps:
	@echo "$(DEPS_TARGET_DIR): $(DEP_DIRS) $(DEP_LIBS)" >> $(DEPS_FILE)

# Should be $(BUILD_PROFILE) but still missing System.Windows.Forms
resx2sr=$(topdir)/class/lib/net_4_x/resx2sr.exe

update-corefx-sr: $(resx2sr) $(RESX_RESOURCE_STRING)
	MONO_PATH="$(topdir)/class/lib/$(BUILD_PROFILE)$(PLATFORM_PATH_SEPARATOR)$$MONO_PATH" $(RUNTIME) $(RUNTIME_FLAGS) $(resx2sr) $(RESX_RESOURCE_STRING) >corefx/SR.cs

$(resx2sr):
	$(MAKE) -C $(topdir)/tools/resx2sr

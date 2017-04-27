thisdir := .

SUBDIRS := build jay mcs class nunit24 ilasm tools tests errors docs packages

# Resgen is corlib specific tool

basic_SUBDIRS := build jay class
build_SUBDIRS := build class class/aot-compiler tools ilasm
monodroid_SUBDIRS := build class
monotouch_SUBDIRS := build class
monotouch_watch_SUBDIRS := build class
monotouch_tv_SUBDIRS := build class
monotouch_runtime_SUBDIRS := build class
monotouch_watch_runtime_SUBDIRS := build class
monotouch_tv_runtime_SUBDIRS := build class
xammac_SUBDIRS := build class
testing_aot_hybrid_SUBDIRS := build class
testing_aot_full_SUBDIRS := build class
binary_reference_assemblies_SUBDIRS := build class
net_4_x_SUBDIRS := build class nunit24 ilasm tools tests errors docs mcs class/aot-compiler packages
xammac_net_4_5_SUBDIRS := build class
xbuild_12_SUBDIRS := build class tools/xbuild
xbuild_14_SUBDIRS := build class tools/xbuild
winaot_SUBDIRS := build class
orbis_SUBDIRS := build class

include build/rules.make

monodroid_PLATFORMS := android
monotouch_PLATFORMS := ios
monotouch_watch_PLATFORMS := watchos
monotouch_tv_PLATFORMS := tvos
monotouch_runtime_PLATFORMS := macos
monotouch_watch_runtime_PLATFORMS := macos
monotouch_tv_runtime_PLATFORMS := macos
xammac_PLATFORMS := macos
testing_aot_hybrid_PLATFORMS := macos win32 linux
testing_aot_full_PLATFORMS := macos win32 linux
binary_reference_assemblies_PLATFORMS := macos win32 linux
net_4_x_PLATFORMS := macos win32 linux
xammac_net_4_5_PLATFORMS := macos
xbuild_12_PLATFORMS := macos win32 linux
xbuild_14_PLATFORMS := macos win32 linux
winaot_PLATFORMS := winaot
orbis_PLATFORMS := orbis

ifndef PLATFORMS
PLATFORMS = $(BUILDPLATFORM)
endif

all-recursive $(STD_TARGETS:=-recursive): dir-check platform-check profile-check

.PHONY: all-local
all-local:
ifdef ALWAYS_AOT
	$(MAKE) do-aot
else
	@:
endif

.PHONY: $(STD_TARGETS:=-local)
$(STD_TARGETS:=-local):
	@:

dir-check:
	@if [ "$(NO_DIR_CHECK)" = "" -a "$(PROFILE)" != "basic" ]; then $(MAKE) -C ../runtime; fi

# fun specialty targets

PROFILES = net_4_x binary_reference_assemblies xbuild_12 xbuild_14

.PHONY: all-profiles $(STD_TARGETS:=-profiles)
all-profiles $(STD_TARGETS:=-profiles): %-profiles: profiles-do--%
	@:

# Call every profile-do--%, for example:
#   `make PROFILES='basic build net_4_x' PLATFORMS='macos win32' profiles-do--all`
# from a macos machine will generate
#   `make profile-do--basic--macos--all profile-do--build--macos--all profile-do--net_4_x--macos--all profile-do--net_4_x--win32--all`
profiles-do--%:
	$(MAKE) $(foreach profile,$(PROFILES),$(foreach platform,$(if $(filter basic build,$(profile)),$(BUILDPLATFORM),$(or $(filter $($(profile)_PLATFORMS),$(PLATFORMS)),$(error nothing to build for profile "$(PROFILES)" and platforms "$(PLATFORMS)"))),profile-do--$(profile)--$(platform)--$*))

# xbuild_12 and xbuild_14 will try to install the same files, so they need
# to be ordered
profile-do--xbuild_14--%-install: profile-do--xbuild_12--%--install

# We don't want to run the tests in parallel.  We want behaviour like -k.
profiles-do--run-test:
	ret=:; $(foreach profile,$(PROFILES),$(foreach platform,$(if $(filter basic build,$(profile)),$(BUILDPLATFORM),$(or $(filter $($(profile)_PLATFORMS),$(PLATFORMS)),$(error nothing to build for profile "$(PROFILES)" and platforms "$(PLATFORMS)"))), { $(MAKE) PROFILE=$(profile) PLATFORM=$(platform) run-test || ret=false; }; )) $$ret

# Orchestrate the bootstrap here.

#  $(1): the profile to build
#  $(2): the profile to depend on
#  $(3): the platform to build
define BuildProfileForPlatform
$(foreach target,all clean install,profile-do--$(1)--$(3)--$(target)): profile-do--$(1)--$(3)--%: $(if $(filter basic,$(1)),,profile-do--$(2)--$(if $(filter build,$(2)),$(BUILDPLATFORM),$(3))--%)
	$(MAKE) PROFILE=$(1) PLATFORM=$(3) $$*
endef

#  $(1): the profile to build
#  $(2): the profile to depend on
define BuildProfile
$(foreach platform,$(if $(filter basic build,$(1)),$(BUILDPLATFORM),$($(1)_PLATFORMS)),$(eval $(call BuildProfileForPlatform,$(1),$(2),$(platform))))
endef

$(call BuildProfile,xbuild_14,net_4_x)
$(call BuildProfile,xbuild_12,net_4_x)
$(call BuildProfile,binary_reference_assemblies,build)
$(call BuildProfile,net_4_x,build)
$(call BuildProfile,monodroid,build)
$(call BuildProfile,monotouch,build)
$(call BuildProfile,monotouch_watch,build)
$(call BuildProfile,monotouch_tv,build)
$(call BuildProfile,monotouch_runtime,build)
$(call BuildProfile,monotouch_watch_runtime,build)
$(call BuildProfile,monotouch_tv_runtime,build)
$(call BuildProfile,xammac,build)
$(call BuildProfile,xammac_net_4_5,build)
$(call BuildProfile,testing_aot_hybrid,build)
$(call BuildProfile,testing_aot_full,build)
$(call BuildProfile,winaot,build)
$(call BuildProfile,orbis,build)
$(call BuildProfile,build,basic)
$(call BuildProfile,basic)

testcorlib:
	@cd class/corlib && $(MAKE) test run-test

compiler-tests:
	$(MAKE) TEST_SUBDIRS="tests errors" run-test-profiles

package := mcs-$(VERSION)

DISTFILES = \
	AUTHORS			\
	COPYING			\
	INSTALL.txt		\
	Makefile		\
	mkinstalldirs		\
	MonoIcon.png		\
	README			\
	ScalableMonoIcon.svg	\
	winexe.in

dist-local: dist-default

csproj-local:

dist-pre:
	rm -rf $(package)
	mkdir $(package)

dist-tarball: dist-pre
	$(MAKE) distdir='$(package)' dist-recursive
	tar cvjf $(package).tar.bz2 $(package)

dist: dist-tarball
	rm -rf $(package)

# the egrep -v is kind of a hack (to get rid of the makefrags)
# but otherwise we have to make dist then make clean which
# is sort of not kosher. And it breaks with DIST_ONLY_SUBDIRS.
#
# We need to set prefix on make so class/System/Makefile can find
# the installed System.Xml to build properly

distcheck: dist-tarball
	rm -rf InstallTest Distcheck-MCS ; \
	mkdir InstallTest ; \
	destdir=`cd InstallTest && pwd` ; \
	mv $(package) Distcheck-MCS ; \
	(cd Distcheck-MCS && \
	    $(MAKE) prefix=$(prefix) && $(MAKE) test && $(MAKE) install DESTDIR="$$destdir" && \
	    $(MAKE) clean && $(MAKE) dist || exit 1) || exit 1 ; \
	mv Distcheck-MCS $(package) ; \
	tar tjf $(package)/$(package).tar.bz2 |sed -e 's,/$$,,' |sort >distdist.list ; \
	rm $(package)/$(package).tar.bz2 ; \
	tar tjf $(package).tar.bz2 |sed -e 's,/$$,,' |sort >before.list ; \
	find $(package) |egrep -v '(makefrag|response)' |sed -e 's,/$$,,' |sort >after.list ; \
	cmp before.list after.list || exit 1 ; \
	cmp before.list distdist.list || exit 1 ; \
	rm -f before.list after.list distdist.list ; \
	rm -rf $(package) InstallTest

monocharge:
	chargedir=monocharge-`date -u +%Y%m%d` ; \
	mkdir "$$chargedir" ; \
	DESTDIR=`cd "$$chargedir" && pwd` ; \
	$(MAKE) install DESTDIR="$$DESTDIR" || exit 1 ; \
	tar cvjf "$$chargedir".tar.bz2 "$$chargedir" ; \
	rm -rf "$$chargedir"

# A bare-bones monocharge.

monocharge-lite:
	chargedir=monocharge-lite-`date -u +%Y%m%d` ; \
	mkdir "$$chargedir" ; \
	DESTDIR=`cd "$$chargedir" && pwd` ; \
	$(MAKE) -C mcs install DESTDIR="$$DESTDIR" || exit 1; \
	$(MAKE) -C class/corlib install DESTDIR="$$DESTDIR" || exit 1; \
	$(MAKE) -C class/System install DESTDIR="$$DESTDIR" || exit 1; \
	$(MAKE) -C class/System.XML install DESTDIR="$$DESTDIR" || exit 1; \
	$(MAKE) -C class/Mono.CSharp.Debugger install DESTDIR="$$DESTDIR" || exit 1; \
	tar cvjf "$$chargedir".tar.bz2 "$$chargedir" ; \
	rm -rf "$$chargedir"

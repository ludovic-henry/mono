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

all-recursive $(STD_TARGETS:=-recursive): dir-check platform-check profile-check

.PHONY: all-local $(STD_TARGETS:=-local)
all-local $(STD_TARGETS:=-local):
	@:

dir-check:
	@if [ "$(NO_DIR_CHECK)" = "" -a "$(PROFILE)" != "basic" ]; then $(MAKE) -C ../runtime; fi

# fun specialty targets

PROFILES = net_4_x binary_reference_assemblies xbuild_12 xbuild_14

.PHONY: all-profiles $(STD_TARGETS:=-profiles)
all-profiles $(STD_TARGETS:=-profiles): %-profiles: profiles-do--%
	@:

# $(1): the profile
define ListPlaformForProfile
$(if $(filter basic build,$(1)), $(BUILD_PLATFORM), $(or $(filter $($(1)_PLATFORMS),$(PLATFORMS)),$(error nothing to build for profile "$(1)" and platforms "$(PLATFORMS)")))
endef

# Call every profile-do--%
# For example: `make PROFILES='basic build net_4_x' profiles-do--all` will generate `make profile-do--basic--all
#  profile-do--build--all profile-do--net_4_x--all`
profiles-do--%:
	$(MAKE) $(foreach profile,$(PROFILES),profile-do--$(profile)--$*)

# xbuild_12 and xbuild_14 will try to install the same files, so they need
# to be ordered
profile-do--xbuild_14--%-install: profile-do--xbuild_12--%--install

# We don't want to run the tests in parallel.  We want behaviour like -k.
profiles-do--run-test:
	ret=:; \
	$(foreach profile,$(PROFILES),$(MAKE) PROFILE=$(profile) run-test || ret=false;) \
	$$ret

# Orchestrate the bootstrap here.

# $(1): the profile to build
# $(2): the profile to depend on
define BuildProfile
$(foreach target,all clean install,profile-do--$(1)--$(target)): profile-do--$(1)--%: $(if $(filter basic,$(1)),,profile-do--$(2)--%)
	$$(MAKE) PROFILE=$(1) $$*
endef

$(eval $(call BuildProfile,xbuild_14,net_4_x))
$(eval $(call BuildProfile,xbuild_12,net_4_x))
$(eval $(call BuildProfile,binary_reference_assemblies,build))
$(eval $(call BuildProfile,net_4_x,build))
$(eval $(call BuildProfile,monodroid,build))
$(eval $(call BuildProfile,monotouch,build))
$(eval $(call BuildProfile,monotouch_watch,build))
$(eval $(call BuildProfile,monotouch_tv,build))
$(eval $(call BuildProfile,monotouch_runtime,build))
$(eval $(call BuildProfile,monotouch_watch_runtime,build))
$(eval $(call BuildProfile,monotouch_tv_runtime,build))
$(eval $(call BuildProfile,xammac,build))
$(eval $(call BuildProfile,xammac_net_4_5,build))
$(eval $(call BuildProfile,testing_aot_hybrid,build))
$(eval $(call BuildProfile,testing_aot_full,build))
$(eval $(call BuildProfile,winaot,build))
$(eval $(call BuildProfile,orbis,build))
$(eval $(call BuildProfile,build,basic))
$(eval $(call BuildProfile,basic))

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

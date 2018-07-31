ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = $(WERROR_CFLAGS)

MONOTOUCH_SUBDIRS = $(libgc_dir) mono

# Some tools might not build when cross-compiling
if CROSS_COMPILING
tools_dir =
else
tools_dir = tools
endif

SUBDIRS = mk po $(libgc_dir) llvm mono $(ikvm_native_dir) support data runtime scripts man samples $(tools_dir) msvc $(docs_dir) acceptance-tests
# Keep in sync with SUBDIRS
DIST_SUBDIRS = m4 mk po $(libgc_dir) llvm mono ikvm-native support data runtime scripts man samples tools msvc docs acceptance-tests

all: update_submodules

SUBMODULE_ERROR='Could not recursively update all git submodules. You may experience compilation problems if some submodules are out of date'
update_submodules:
	@$(srcdir)/scripts/update_submodules.sh

.PHONY: update_submodules

EXTRA_DIST= \
	README.md 		\
	LICENSE 		\
	autogen.sh 		\
	mkinstalldirs 		\
	mono-uninstalled.pc.in 	\
	winconfig.h 		\
	code_of_conduct.md 	\
	external		\
	mcs/class/referencesource

DISTCHECK_CONFIGURE_FLAGS = EXTERNAL_MCS=false EXTERNAL_RUNTIME=false

# Distribute the 'mcs' tree too
GIT_DIR ?= $(srcdir)/.git
dist-hook:
	test -d $(distdir)/mcs || mkdir $(distdir)/mcs
	d=`cd $(distdir)/mcs && pwd`; cd $(mcs_topdir) && $(MAKE) distdir=$$d dist-recursive
	rm -rf `find $(top_distdir)/external -path '*\.git'`
	rm -f `find $(top_distdir)/external -path '*\.exe' -not -path '*/roslyn-binaries/*'`
	rm -f `find $(top_distdir)/external -path '*\.dll' -not -path '*/binary-reference-assemblies/*' -not -path '*/roslyn-binaries/*'`
	rm -rf "$(top_distdir)/external/linker/linker/Tests"

pkgconfigdir = $(libdir)/pkgconfig
noinst_DATA = mono-uninstalled.pc
DISTCLEANFILES= mono-uninstalled.pc

# building with monolite
.PHONY: get-monolite-latest 
get-monolite-latest:
	$(MAKE) -C $(mcs_topdir)/class get-monolite-latest

if BITCODE
BITCODE_CHECK=yes
endif

.PHONY: check-ci
check-ci:
	MONO_LLVMONLY=$(BITCODE_CHECK) $(srcdir)/scripts/ci/run-test-$(TEST_PROFILE).sh

.PHONY: validate do-build-mono-mcs mcs-do-clean mcs-do-tests
validate: do-build-mono-mcs
	$(MAKE) mcs-do-tests
do-build-mono-mcs: mcs-do-clean
	$(MAKE) all
mcs-do-clean:
	cd runtime && $(MAKE) clean-local
	cd mono/tests && $(MAKE) clean
mcs-do-tests:
	cd runtime && $(MAKE) check-local
	cd mono/tests && $(MAKE) check

.PHONY: compiler-tests mcs-do-compiler-tests
compiler-tests:
	$(MAKE) test_select='TEST_SUBDIRS="tests errors"' validate
mcs-do-compiler-tests:
	$(MAKE) test_select='TEST_SUBDIRS="tests errors"' mcs-do-tests

.PHONY: bootstrap-world
bootstrap-world: compiler-tests
	$(MAKE) install

if INSTALL_MONOTOUCH
monotouch-do-build: config.h
	@list='$(MONOTOUCH_SUBDIRS)'; for subdir in $$list; do \
	  case "x$$subdir" in \
		xmono ) target="monotouch-do-build";; \
		* ) target="all";; \
	  esac; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$target); \
        done;
	(cd runtime && $(MAKE) $(AM_MAKEFLAGS) monotouch-do-build)

monotouch-do-clean:
	@list='$(MONOTOUCH_SUBDIRS)'; for subdir in $$list; do \
	  case "x$$subdir" in \
		xmono ) target="monotouch-do-clean";; \
		* ) target="clean";; \
	  esac; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$target); \
        done;
	(cd runtime && $(MAKE) $(AM_MAKEFLAGS) monotouch-do-clean)

endif

update-csproj:
	-rm msvc/scripts/order 
	-rm msvc/scripts/order.xml
	-rm -rf msvc/scripts/inputs
	-mkdir msvc/scripts/inputs
	(cd runtime; $(MAKE) V=1 extra_targets=csproj-local)

package-inputs:
	echo '<?xml version="1.0" encoding="utf-8"?>' > msvc/scripts/order.xml
	echo '<root>' >> msvc/scripts/order.xml
	for i in `cat msvc/scripts/order`; do \
		set `echo $$i | sed -e 's/:/ /' -e 's/.input//'`; \
		cat msvc/scripts/inputs/$$2.input | sed -e 's/\\\\/\\/g' -e 's/\\/\\\\/g' | \
		(echo "    <project dir=\"$$1\" library=\"$$2\">"; \
		 read boot;   echo "      <boot>$$boot</boot>"; \
		 read flags;  echo "      <flags>$$flags</flags>"; \
		 read sources;echo "      <sources>$$sources</sources>"; \
		 read output; echo "      <output>$$output</output>"; \
		 read built;  echo "      <built_sources>`echo $$built | sed 's/\\\/\\\\/g'`</built_sources>"; \
		 read libou;  echo "      <library_output>$$libou</library_output>"; \
		 read fx_ver; echo "      <fx_version>$$fx_ver</fx_version>"; \
		 read profile; echo "      <profile>$$profile</profile>"; \
		 read resxt;  echo "      <resources>$$resxt</resources>"; \
		 read resp;   echo "      <response>$$resp</response>"; \
		 echo "    </project>") >> msvc/scripts/order.xml; \
	done
	echo "</root>" >> msvc/scripts/order.xml

# Update llvm version in configure.ac to the output of $LLVM_DIR/bin/llvm-config --version
update-llvm-version:
	if test "x$$LLVM_DIR" = "x"; then echo "Set the make variable LLVM_DIR to the directory containing the LLVM installation."; exit 1; fi
	REV=`$(LLVM_DIR)/bin/llvm-config --version` && sed -e "s,expected_llvm_version=.*,expected_llvm_version=\"$$REV\"," < configure.ac > tmp && mv tmp configure.ac && echo "Version set to $$REV."


update-solution-files:
	cd msvc/scripts && $(MAKE) genproj.exe || exit $$?;
	$(MAKE) update-csproj
	$(MAKE) package-inputs
	(cd msvc/scripts; mono --debug genproj.exe $(GENPROJ_ARGS))

update-solution-files-with-tests:
	$(MAKE) "GENPROJ_ARGS=2012 true true" update-solution-files

package-helix:
	rm -rf helix-payload
	mkdir -p helix-payload/net_4_x/runtime
	mkdir -p helix-payload/net_4_x/tests/mcs
	mkdir -p helix-payload/net_4_x/tests/mcs-errors
	mkdir -p helix-payload/net_4_x/tests/mini
	mkdir -p helix-payload/net_4_x/tests/profiler
	mkdir -p helix-payload/net_4_x/tests/symbolicate
	mkdir -p helix-payload/net_4_x/tests/csi
	cp -R -L $(top_builddir)/runtime/mono-helix-wrapper.sh helix-payload/
	cp -R -L $(mcs_topdir)/class/lib/net_4_x helix-payload/
	cp -R -L helix-payload/net_4_x/compiler-tester.* helix-payload/net_4_x/tests/mcs/
	cp -R -L helix-payload/net_4_x/compiler-tester.* helix-payload/net_4_x/tests/mcs-errors/
	cp -R -L $(mcs_topdir)/tests/known-issues-net_4_x helix-payload/net_4_x/tests/mcs/known-issues-net_4_x
	cp -R -L $(mcs_topdir)/tests/ver-il-net_4_x.xml helix-payload/net_4_x/tests/mcs/ver-il-net_4_x.xml
	cp -R -L $(mcs_topdir)/tests/dlls helix-payload/net_4_x/tests/mcs/
	cp -R -L $(mcs_topdir)/tests/*.cs helix-payload/net_4_x/tests/mcs/
	cp -R -L $(mcs_topdir)/tests/*.xml helix-payload/net_4_x/tests/mcs/
	cp -R -L $(mcs_topdir)/tests/*.inc helix-payload/net_4_x/tests/mcs/
	cp -R -L $(mcs_topdir)/tests/*.dll helix-payload/net_4_x/tests/mcs/
	cp -R -L $(mcs_topdir)/tests/*.snk helix-payload/net_4_x/tests/mcs/
	cp -R -L $(mcs_topdir)/errors/known-issues-net_4_x helix-payload/net_4_x/tests/mcs-errors/known-issues-net_4_x
	cp -R -L $(mcs_topdir)/errors/dlls helix-payload/net_4_x/tests/mcs-errors/
	cp -R -L $(mcs_topdir)/errors/*.cs helix-payload/net_4_x/tests/mcs-errors/
	cp -R -L $(mcs_topdir)/errors/*.inc helix-payload/net_4_x/tests/mcs-errors/
	cp -R -L $(mcs_topdir)/errors/*.dll helix-payload/net_4_x/tests/mcs-errors/
	cp -R -L $(mcs_topdir)/errors/*.snk helix-payload/net_4_x/tests/mcs-errors/
	cp -R -L $(mcs_topdir)/errors/*.pub helix-payload/net_4_x/tests/mcs-errors/
	cp -R -L $(mcs_topdir)/tools/mono-symbolicate/StackTraceDumper.exe $(mcs_topdir)/tools/mono-symbolicate/StackTraceDumper.pdb $(mcs_topdir)/tools/mono-symbolicate/Test/symbolicate.expected helix-payload/net_4_x/tests/symbolicate/
	cp -R -L $(dir $(CSC))/* helix-payload/net_4_x/tests/csi/
	cp -R -L $(top_builddir)/mono/mini/mono-sgen helix-payload/net_4_x/
	cp -R -L $(top_builddir)/mono/mini/mono-boehm helix-payload/net_4_x/
	chmod +x helix-payload/net_4_x/mono-sgen
	chmod +x helix-payload/net_4_x/mono-boehm
	cp -R -L $(top_builddir)/mono/mini/*.exe $(top_builddir)/mono/mini/TestDriver.dll $(top_builddir)/mono/mini/MemoryIntrinsics.dll $(top_builddir)/mono/mini/generics-variant-types.dll helix-payload/net_4_x/tests/mini/
	cp -R -L $(top_builddir)/mono/metadata/.libs/libmono-system-native$(libsuffix) helix-payload/net_4_x/
	cp -R -L $(top_builddir)/mono/btls/build-shared/libmono-btls-shared$(libsuffix) helix-payload/net_4_x/
	cp -R -L $(top_builddir)/mono/profiler/.libs/libmono-profiler-log$(libsuffix) helix-payload/net_4_x/
	cp -R -L $(top_builddir)/mono/profiler/test-*.exe helix-payload/net_4_x/tests/profiler/
	cp -R -L $(top_builddir)/mono/profiler/ptestrunner.pl helix-payload/net_4_x/tests/profiler/
	cp -R -L $(top_builddir)/mono/profiler/mprof-report helix-payload/net_4_x/tests/profiler/
	cp -R -L $(top_builddir)/support/.libs/libMonoPosixHelper$(libsuffix) helix-payload/net_4_x/
	cp -R -L $(top_builddir)/runtime/etc helix-payload/net_4_x/runtime/
	sed 's,$$mono_libdir/,,g' $(top_builddir)/data/config > helix-payload/net_4_x/runtime/etc/mono/config
	cp -R -L $(top_srcdir)/external/xunit-binaries/*.dll $(top_srcdir)/external/xunit-binaries/*.exe $(top_srcdir)/external/xunit-binaries/*.config helix-payload/net_4_x/
	cp -R -L $(top_srcdir)/external/xunit-binaries/xunit.execution.desktop.dll helix-payload/net_4_x/tests/
	find helix-payload -empty -print0 | xargs -0 rm -rf  # FIXME: helix doesn't seem to like empty files/folders
# TODO: find out if we can keep the AOT images
#	find helix-payload -name "*.dylib" -print0 | xargs -0 rm -rf
#	find helix-payload -name "*.dSYM" -print0 | xargs -0 rm -rf
#	find helix-payload -name "*.so" -print0 | xargs -0 rm -rf
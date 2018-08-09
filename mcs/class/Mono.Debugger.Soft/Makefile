thisdir = class/Mono.Debugger.Soft
include ../../build/rules.make

LIBRARY = Mono.Debugger.Soft.dll
LIBRARY_SNK = ../mono.snk

LIB_REFS = System Mono.Cecil System.Core
LIB_MCS_FLAGS = /unsafe -D:MONO_DATACONVERTER_STATIC_METHODS /publicsign
KEYFILE = $(LIBRARY_SNK)

TEST_MCS_FLAGS =
TEST_LIB_REFS = Mono.Cecil System System.Core

VALID_TEST_PROFILE := $(filter net_4_x monodroid, $(PROFILE))

# The test exe is not profile specific, and compiling a 2.0 will make the 4.5 tests fail
ifdef VALID_TEST_PROFILE

test-local: $(topdir)/class/lib/$(PROFILE)/tests/dtest-app.exe $(topdir)/class/lib/$(PROFILE)/tests/dtest-excfilter.exe

$(topdir)/class/lib/$(PROFILE)/tests/dtest-excfilter.exe: Test/dtest-excfilter.il
	mkdir -p $(dir $@)
	$(ILASM) -out:$@ /exe /debug Test/dtest-excfilter.il

else

NO_TEST=1
check:

endif

TEST_HELPERS_SOURCES = \
	../test-helpers/NetworkHelpers.cs \
	Test/TypeLoadClass.cs

$(topdir)/class/lib/$(PROFILE)/tests/dtest-app.exe: Test/dtest-app.cs $(TEST_HELPERS_SOURCES)
	mkdir -p $(dir $@)
	$(CSCOMPILE) -r:$(topdir)/class/lib/$(PROFILE)/mscorlib.dll -r:$(topdir)/class/lib/$(PROFILE)/System.Core.dll -r:$(topdir)/class/lib/$(PROFILE)/System.dll -out:$@ -unsafe $(PLATFORM_DEBUG_FLAGS) -optimize- Test/dtest-app.cs $(TEST_HELPERS_SOURCES)

CLEAN_FILES = $(addprefix $(topdir)/class/lib/$(PROFILE)/tests/, dtest-app.exe dtest-app.exe.mdb dtest-app.pdb dtest-excfilter.exe dtest-excfilter.exe.mdb dtest-excfilter.pdb)

foo:
	echo $(CLEAN_FILES)

EXTRA_DISTFILES = \
	Test/dtest-app.cs \
	Test/dtest.cs \
	Test/dtest-excfilter.il

#NO_TEST = yes

include ../../build/library.make

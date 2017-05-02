thisdir=class/System.Data/Test/DataProviderTests/datareadertests

include ../../../../../build/rules.make

LIB_MCS_FLAGS = /r:System.Data,System.Data.OracleClient,System,System.Xml

DATABASE=all

LIBRARY = DbAdapter.dll
NO_INSTALL=yes
NO_SIGN_ASSEMBLY=yes

include $(topdir)/build/library.make

Runtests.exe: $(call the_lib,$(BUILD_PLATFORM))
	$(CSCOMPILE) /r:$(call the_lib,$(BUILD_PLATFORM)) Runtests.cs

DbAdapter.dll: 
	$(CSCOMPILE) /r:$(REFERENCES) /target:library /out:DbAdapter.dll @$(SOURCEFILE)

run-test-local: run-test-db
run-test-db: Runtests.exe
	$(TEST_RUNTIME) Runtests.exe $(DATABASE) | tee TestResults.log

test-local: Runtests.exe

CLEAN_FILES = *.exe *.dll *.log

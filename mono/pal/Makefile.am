
COREFX_ROOT=$(abs_top_srcdir)/external/corefx

COREFX_BUILD_ARCH=x64
COREFX_BUILD_OS=OSX
COREFX_BUILD_TYPE=Release

all-local: libmono-pal$(libsuffix)

.PHONY: libmono-pal$(libsuffix)
libmono-pal$(libsuffix):
	$(COREFX_ROOT)/src/Native/build-native.sh $(COREFX_BUILD_ARCH) $(COREFX_BUILD_TYPE) $(COREFX_BUILD_OS)
	cp $(COREFX_ROOT)/bin/$(COREFX_BUILD_OS).$(COREFX_BUILD_ARCH).$(COREFX_BUILD_TYPE)/Native/System.Native$(libsuffix) libmono-pal$(libsuffix)

clean-local:
	-rm -f libmono-pal$(libsuffix)

install-exec-local:
	$(mkinstalldirs) "$(DESTDIR)$(libdir)"
	$(install_sh) libmono-pal* "$(DESTDIR)$(libdir)"

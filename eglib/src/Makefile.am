
.PHONY: $(abs_top_builddir)/cmake-build/eglib/src/libeglib.a
$(abs_top_builddir)/cmake-build/eglib/src/libeglib.a:
	$(MAKE) all -C $(abs_top_builddir)/cmake-build/eglib/src

$(abs_top_builddir)/eglib/src/libeglib.a: $(abs_top_builddir)/cmake-build/eglib/src/libeglib.a
	cp $< $@

all-local: $(abs_top_builddir)/eglib/src/libeglib.a

clean-local:
	-$(MAKE) clean $(abs_top_builddir)/cmake-build/eglib/src

install-exec-local:
	$(mkinstalldirs) "$(DESTDIR)$(libdir)"
	$(install_sh) $(builddir)/build/libeglib.a "$(DESTDIR)$(libdir)"

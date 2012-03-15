##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common

pkgdata_DATA = version
EASY_INSTALL = cfg lng voice mappings ttscp root

version: $(top_srcdir)/configure.in
	@echo $(VERSION) > version

install-data-hook:
	cd $(srcdir) && find $(EASY_INSTALL) -type d -exec $(INSTALL_DATA) -d $(pkgdatadir)/{} \;
	cd $(srcdir) && find $(EASY_INSTALL) -type f -exec $(INSTALL_DATA) $(srcdir)/{} $(pkgdatadir)/{} \;

EXTRA_DIST = $(pkgdata_DATA) $(EASY_INSTALL)

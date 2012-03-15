##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common

pkgdata_DATA = version
EASY_INSTALL = cfg lng voice mappings ttscp root

version: $(top_srcdir)/configure.in
	@echo $(VERSION) > version

# If you don't have find, the following will fail.
# In this case note that all you need is a recursive copy
# of the content of this directory to the "right" place,
# such as cp -r * /usr/share/epos

install-data-local:	
	$(INSTALL_DATA) -d $(pkgdatadir)/root
	cd $(srcdir) && find $(EASY_INSTALL) -type d -exec $(INSTALL_DATA) -d -m 755 $(pkgdatadir)/{} \;
	cd $(srcdir) && find $(EASY_INSTALL) -type f -exec $(INSTALL_DATA) $(srcdir)/{} $(pkgdatadir)/{} \;

EXTRA_DIST = $(pkgdata_DATA) $(EASY_INSTALL)

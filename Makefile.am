##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common
export VERSION

# Minimum automake version:

AUTOMAKE_OPTIONS=1.4 foreign

SUBDIRS = src cfg doc

EXTRA_DIST=arch Makefile.common Changes

#distcleancheck:
#	@@:

dist-hook:
	@. $(distdir)/arch/unix/test.version

upload:
	@. $(srcdir)/arch/unix/upload/upload $(VERSION)

##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common
export VERSION

# Minimum automake version:

AUTOMAKE_OPTIONS=1.11 foreign

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src cfg arch

EXTRA_DIST=Makefile.common WELCOME

#distcleancheck:
#	@@:

dist-hook:
	@. $(distdir)/arch/unix/test.version

upload:
	@. $(srcdir)/arch/unix/upload/upload $(VERSION)


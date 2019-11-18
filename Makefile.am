ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in \
  $(srcdir)/src/config.h.in~ \
  $(srcdir)/configure

SUBDIRS = src examples

pkgconfig_DATA = libdnstap.pc

dist_doc_DATA = README.md

EXTRA_DIST = m4

test: check

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

SUBDIRS = test

AM_CFLAGS = -I$(tinyframe_CFLAGS) \
  -I$(protobuf_c_CFLAGS)

lib_LTLIBRARIES = libdnswire.la

libdnswire_la_SOURCES = decoder.c dnstap.c dnswire.c encoder.c reader.c \
  writer.c
nodist_libdnswire_la_SOURCES = dnstap.pb-c.c
BUILT_SOURCES += dnswire/dnstap.pb-c.h
nobase_include_HEADERS = dnswire/decoder.h dnswire/dnstap.h \
  dnswire/dnswire.h dnswire/encoder.h dnswire/reader.h dnswire/writer.h
nobase_nodist_include_HEADERS = dnswire/version.h dnswire/dnstap.pb-c.h \
  dnswire/dnstap-macros.h
libdnswire_la_LDFLAGS = -version-info $(DNSWIRE_LIBRARY_VERSION) \
  -ltinyframe -lprotobuf-c

CLEANFILES += $(nodist_libdnswire_la_SOURCES)
EXTRA_DIST += dnstap.pb/dnstap.proto dnstap.pb/LICENSE dnstap.pb/README.md

dnswire/dnstap.pb-c.h: dnstap.pb-c.c
	mkdir -p dnswire/
	cp dnstap.pb-c.h dnswire/

dnstap.pb-c.c: dnstap.pb/dnstap.proto
	$(AM_V_GEN)@PROTOC_C@ "--c_out=." -I$(srcdir)/dnstap.pb $<

BUILT_SOURCES += dnswire/dnstap-macros.h
EXTRA_DIST += gen-macros.sh dnstap.fields

dnswire/dnstap-macros.h: dnstap.fields gen-macros.sh
	$(AM_V_GEN)"$(srcdir)/gen-macros.sh" $< >$@

CLEANFILES += $(BUILT_SOURCES)

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = test*.log test*.trs \
  test2.dnstap

AM_CFLAGS = -I$(top_srcdir)/src \
  -I$(tinyframe_CFLAGS) \
  -I$(protobuf_c_CFLAGS)

check_PROGRAMS = test1 test2
TESTS = test1.sh test2.sh

test1.sh: test1

test1_SOURCES = test1.c
test1_LDADD = ../libdnstap.la
test1_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

test2.sh: test2 test1

test2_SOURCES = test2.c
test2_LDADD = ../libdnstap.la
test2_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

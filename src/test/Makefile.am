4MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = test*.log test*.trs \
  test1.out test2.out test2.dnstap test3.out test4.dnstap test4.out

AM_CFLAGS = -I$(top_srcdir)/src \
  $(tinyframe_CFLAGS) \
  $(protobuf_c_CFLAGS)

check_PROGRAMS = test1 test2 test3 test4
TESTS = test1.sh test2.sh test3.sh test4.sh
EXTRA_DIST = create_dnstap.c print_dnstap.c $(TESTS) test.dnstap \
  test1.gold test2.gold test3.gold test4.gold

test1_SOURCES = test1.c
test1_LDADD = ../libdnswire.la
test1_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

test2_SOURCES = test2.c
test2_LDADD = ../libdnswire.la
test2_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

test3_SOURCES = test3.c
test3_LDADD = ../libdnswire.la
test3_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

test4_SOURCES = test4.c
test4_LDADD = ../libdnswire.la
test4_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

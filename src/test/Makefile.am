4MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = test*.log test*.trs \
  test1.out test2.out test3.out test3.dnstap test4.out test4.dnstap \
  test5.out test5.sock

AM_CFLAGS = -I$(top_srcdir)/src \
  $(tinyframe_CFLAGS) \
  $(protobuf_c_CFLAGS)

check_PROGRAMS = reader_read reader_push writer_write writer_pop \
  reader_unixsock writer_unixsock
TESTS = test1.sh test2.sh test3.sh test4.sh test5.sh
EXTRA_DIST = create_dnstap.c print_dnstap.c $(TESTS) test.dnstap \
  test1.gold test2.gold test3.gold test4.gold test5.gold

reader_read_SOURCES = reader_read.c
reader_read_LDADD = ../libdnswire.la
reader_read_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

reader_push_SOURCES = reader_push.c
reader_push_LDADD = ../libdnswire.la
reader_push_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

writer_write_SOURCES = writer_write.c
writer_write_LDADD = ../libdnswire.la
writer_write_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

writer_pop_SOURCES = writer_pop.c
writer_pop_LDADD = ../libdnswire.la
writer_pop_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

reader_unixsock_SOURCES = reader_unixsock.c
reader_unixsock_LDADD = ../libdnswire.la
reader_unixsock_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

writer_unixsock_SOURCES = writer_unixsock.c
writer_unixsock_LDADD = ../libdnswire.la
writer_unixsock_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

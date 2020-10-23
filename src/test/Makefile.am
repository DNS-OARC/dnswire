4MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = test*.log test*.trs \
  test1.out test2.out test3.out test3.dnstap test4.out test4.dnstap \
  test5.out test5.sock *.gcda *.gcno *.gcov

AM_CFLAGS = -I$(top_srcdir)/src \
  $(tinyframe_CFLAGS) \
  $(protobuf_c_CFLAGS)

check_PROGRAMS = reader_read reader_push writer_write writer_pop \
  reader_unixsock writer_unixsock test_dnstap test_encoder test_decoder \
  test_reader test_writer
TESTS = test1.sh test2.sh test3.sh test4.sh test5.sh test6.sh
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

test_dnstap_SOURCES = test_dnstap.c
test_dnstap_LDADD = ../libdnswire.la
test_dnstap_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

test_encoder_SOURCES = test_encoder.c
test_encoder_LDADD = ../libdnswire.la
test_encoder_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

test_decoder_SOURCES = test_decoder.c
test_decoder_LDADD = ../libdnswire.la
test_decoder_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

test_reader_SOURCES = test_reader.c
test_reader_LDADD = ../libdnswire.la
test_reader_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

test_writer_SOURCES = test_writer.c
test_writer_LDADD = ../libdnswire.la
test_writer_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS) -static

if ENABLE_GCOV
gcov-local:
	for src in $(reader_read_SOURCES) $(reader_push_SOURCES) \
$(writer_write_SOURCES) $(writer_pop_SOURCES) $(reader_unixsock_SOURCES) \
$(writer_unixsock_SOURCES) $(test_dnstap_SOURCES) $(test_encoder_SOURCES) \
$(test_decoder_SOURCES) $(test_reader_SOURCES) $(test_writer_SOURCES); do \
	  gcov -l -r -s "$(srcdir)" "$$src"; \
	done
endif

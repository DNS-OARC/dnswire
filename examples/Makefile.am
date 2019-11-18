MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES =

AM_CFLAGS = -I$(top_srcdir)/src \
  -I$(tinyframe_CFLAGS) \
  -I$(protobuf_c_CFLAGS)

if BUILD_EXAMPLES

noinst_PROGRAMS = simple_reader simple_writer simple_sender simple_receiver \
  multi_sender_uv

simple_reader_SOURCES = simple_reader.c
simple_reader_LDADD = ../src/libdnstap.la
simple_reader_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

simple_writer_SOURCES = simple_writer.c
simple_writer_LDADD = ../src/libdnstap.la
simple_writer_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

simple_sender_SOURCES = simple_sender.c
simple_sender_LDADD = ../src/libdnstap.la
simple_sender_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

simple_receiver_SOURCES = simple_receiver.c
simple_receiver_LDADD = ../src/libdnstap.la
simple_receiver_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

multi_sender_uv_SOURCES = multi_sender_uv.c
multi_sender_uv_LDADD = ../src/libdnstap.la
multi_sender_uv_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

endif

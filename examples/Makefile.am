MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES =

AM_CFLAGS = -I$(top_srcdir)/src \
  -I$(tinyframe_CFLAGS) \
  -I$(protobuf_c_CFLAGS)

AM_LDFLAGS = $(protobuf_c_LIBS) $(tinyframe_LIBS)

EXTRA_DIST = print_dnstap.c create_dnstap.c

if BUILD_EXAMPLES

noinst_PROGRAMS = reader writer simple_reader simple_writer simple_sender \
  simple_receiver

reader_SOURCES = reader.c
reader_LDADD = ../src/libdnswire.la

writer_SOURCES = writer.c
writer_LDADD = ../src/libdnswire.la

simple_reader_SOURCES = simple_reader.c
simple_reader_LDADD = ../src/libdnswire.la

simple_writer_SOURCES = simple_writer.c
simple_writer_LDADD = ../src/libdnswire.la

simple_sender_SOURCES = simple_sender.c
simple_sender_LDADD = ../src/libdnswire.la

simple_receiver_SOURCES = simple_receiver.c
simple_receiver_LDADD = ../src/libdnswire.la

if HAVE_LIBUV

AM_CFLAGS += -I$(uv_CFLAGS)
AM_LDFLAGS += $(uv_LIBS)

noinst_PROGRAMS += daemon_sender_uv client_receiver_uv

daemon_sender_uv_SOURCES = daemon_sender_uv.c
daemon_sender_uv_LDADD = ../src/libdnswire.la

client_receiver_uv_SOURCES = client_receiver_uv.c
client_receiver_uv_LDADD = ../src/libdnswire.la

endif

endif

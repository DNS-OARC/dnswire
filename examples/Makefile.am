MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES =

AM_CFLAGS = -I$(top_srcdir)/src \
  $(tinyframe_CFLAGS) \
  $(protobuf_c_CFLAGS)

AM_LDFLAGS = $(protobuf_c_LIBS) \
  $(tinyframe_LIBS) -static

EXTRA_DIST = print_dnstap.c create_dnstap.c

if BUILD_EXAMPLES

noinst_PROGRAMS = reader writer sender receiver reader_sender

reader_SOURCES = reader.c
reader_LDADD = ../src/libdnswire.la

writer_SOURCES = writer.c
writer_LDADD = ../src/libdnswire.la

sender_SOURCES = sender.c
sender_LDADD = ../src/libdnswire.la

receiver_SOURCES = receiver.c
receiver_LDADD = ../src/libdnswire.la

reader_sender_SOURCES = reader_sender.c
reader_sender_LDADD = ../src/libdnswire.la

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

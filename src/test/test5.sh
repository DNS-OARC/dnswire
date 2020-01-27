#!/bin/sh -xe

rm -f test5.sock
./writer_reader_unixsock test5.sock | grep -v _time > test5.out
diff -u "$srcdir/test5.gold" test5.out

#!/bin/sh -xe

rm -f test5.sock
./writer_unixsock test5.sock &
./reader_unixsock test5.sock | grep -v _time | grep -v ^version: > test5.out
rm -f test5.sock
diff -u "$srcdir/test5.gold" test5.out

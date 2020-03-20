#!/bin/sh -xe

rm -f test4.dnstap
./writer_pop test4.dnstap > test4.out
./reader_read test4.dnstap | grep -v _time | grep -v ^version: >> test4.out
diff -u "$srcdir/test4.gold" test4.out

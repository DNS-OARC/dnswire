#!/bin/sh -xe

rm -f test3.dnstap
./writer_write test3.dnstap > test3.out
./reader_push test3.dnstap 4096 | grep -v _time >> test3.out
diff -u "$srcdir/test3.gold" test3.out

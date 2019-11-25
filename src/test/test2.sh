#!/bin/sh -xe

rm -f test2.dnstap
./test2 test2.dnstap
./test1 test2.dnstap 4096 | grep -v _time > test2.out
diff -u "$srcdir/test2.gold" test2.out

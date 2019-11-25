#!/bin/sh -xe

rm -f test4.dnstap
./test4 test4.dnstap
./test3 test4.dnstap 4096 | grep -v _time > test4.out
diff -u "$srcdir/test4.gold" test4.out

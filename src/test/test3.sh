#!/bin/sh -xe

./test3 "$srcdir/test.dnstap" 10
./test3 "$srcdir/test.dnstap" 18
./test3 "$srcdir/test.dnstap" 32
./test3 "$srcdir/test.dnstap" 64

./test3 "$srcdir/test.dnstap" 4096 > test3.out
diff -u "$srcdir/test3.gold" test3.out

#!/bin/sh -xe

./test1 "$srcdir/test.dnstap" 10
./test1 "$srcdir/test.dnstap" 18
./test1 "$srcdir/test.dnstap" 32
./test1 "$srcdir/test.dnstap" 64

./test1 "$srcdir/test.dnstap" 4096 > test1.out
diff -u "$srcdir/test1.gold" test1.out

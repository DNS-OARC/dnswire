#!/bin/sh -xe

./reader_push "$srcdir/test.dnstap" 10
./reader_push "$srcdir/test.dnstap" 18
./reader_push "$srcdir/test.dnstap" 32
./reader_push "$srcdir/test.dnstap" 64

./reader_push "$srcdir/test.dnstap" 4096 > test2.out
diff -u "$srcdir/test2.gold" test2.out

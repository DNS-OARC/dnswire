#!/bin/sh -xe

./reader_read "$srcdir/test.dnstap" > test1.out
diff -u "$srcdir/test1.gold" test1.out

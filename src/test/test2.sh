#!/bin/sh -xe

rm -f test2.dnstap
./test2 test2.dnstap
./test1 test2.dnstap 4096

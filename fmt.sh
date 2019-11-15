#!/bin/sh

clang-format-4.0 \
    -style=file \
    -i \
    src/dnstap.c \
    src/dnstap/dnstap.h \
    src/test/*.c

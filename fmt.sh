#!/bin/sh

clang-format \
    -style=file \
    -i \
    src/*.c \
    src/dnswire/*.h \
    src/test/*.c \
    examples/*.c

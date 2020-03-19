#!/bin/sh

clang-format-4.0 \
    -style=file \
    -i \
    src/*.c \
    src/dnswire/*.h \
    src/test/*.c \
    examples/*.c

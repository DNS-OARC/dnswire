/*
 * Author Jerry Lundström <jerry@dns-oarc.net>
 * Copyright (c) 2019, OARC, Inc.
 * All rights reserved.
 *
 * This file is part of the dnswire library.
 *
 * dnswire library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dnswire library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with dnswire library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dnswire/dnswire.h>
#include <dnswire/encoder.h>

#include <stdlib.h>

#ifndef __dnswire_h_writer
#define __dnswire_h_writer 1

struct dnswire_writer {
    struct dnswire_encoder encoder;
    uint8_t*               buf;
    size_t                 size, inc, max, at, left;
};

#define DNSWIRE_WRITER_INITIALIZER                   \
    {                                                \
        .encoder = DNSWIRE_ENCODER_INITIALIZER,      \
        .buf     = malloc(DNSWIRE_DEFAULT_BUF_SIZE), \
        .size    = DNSWIRE_DEFAULT_BUF_SIZE,         \
        .inc     = DNSWIRE_DEFAULT_BUF_SIZE,         \
        .max     = DNSWIRE_MAXIMUM_BUF_SIZE,         \
        .at      = 0,                                \
        .left    = 0,                                \
    }

#define dnswire_writer_set_dnstap(w, d) (w).encoder.dnstap = d
#define dnswire_writer_cleanup(w) free((w).buf)

enum dnswire_result dnswire_writer_set_bufsize(struct dnswire_writer*, size_t);
enum dnswire_result dnswire_writer_set_bufinc(struct dnswire_writer*, size_t);
enum dnswire_result dnswire_writer_set_bufmax(struct dnswire_writer*, size_t);

enum dnswire_result dnswire_writer_get(struct dnswire_writer*);
enum dnswire_result dnswire_writer_write(struct dnswire_writer*, int);
enum dnswire_result dnswire_writer_stop(struct dnswire_writer*);

static inline enum dnswire_result dnswire_writer_fwrite(struct dnswire_writer* handle, FILE* fp)
{
    return dnswire_writer_write(handle, fileno(fp));
}

#endif

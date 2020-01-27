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
#include <dnswire/decoder.h>
#include <dnswire/encoder.h>

#include <stdlib.h>

#ifndef __dnswire_h_reader
#define __dnswire_h_reader 1

enum dnswire_reader_state {
    dnswire_reader_reading_control  = 0,
    dnswire_reader_decoding_control = 1,
    dnswire_reader_encoding_accept  = 2,
    dnswire_reader_writing_accept   = 3,
    dnswire_reader_reading          = 4,
    dnswire_reader_decoding         = 5,
    dnswire_reader_encoding_finish  = 6,
    dnswire_reader_writing_finish   = 7,
    dnswire_reader_done             = 8,
};
extern const char* const dnswire_reader_state_string[];

/*
 * Attributes:
 * - size: The current size of the buffer
 * - inc: How much the buffer will be increased by if more spare is needed
 * - max: The maximum size the buffer is allowed to have
 * - at: Where in the buffer we are decoding (start of data)
 * - left: How much data that is still left in the buffer from `at`
 * - pushed: How much data that was pushed to the buffer by `dnswire_reader_push()`
 */
struct dnswire_reader {
    enum dnswire_reader_state state;

    struct dnswire_decoder decoder;
    uint8_t*               buf;
    size_t                 size, inc, max, at, left, pushed;

    struct dnswire_encoder encoder;
    uint8_t*               write_buf;
    size_t                 write_size, write_inc, write_max, write_at, write_left;

    bool allow_bidirectional, is_bidirectional;
};

enum dnswire_result dnswire_reader_init(struct dnswire_reader*);

#define dnswire_reader_pushed(r) (r).pushed
#define dnswire_reader_dnstap(r) (&(r).decoder.dnstap)
#define dnswire_reader_cleanup(r) \
    dnswire_decoder_cleanup((r).decoder)
#define dnswire_reader_destroy(r) \
    free((r).buf);                \
    free((r).write_buf);          \
    dnswire_decoder_cleanup((r).decoder)
#define dnswire_reader_is_bidirectional(r) (r).is_bidirectional

enum dnswire_result dnswire_reader_allow_bidirectional(struct dnswire_reader*, bool);
enum dnswire_result dnswire_reader_set_bufsize(struct dnswire_reader*, size_t);
enum dnswire_result dnswire_reader_set_bufinc(struct dnswire_reader*, size_t);
enum dnswire_result dnswire_reader_set_bufmax(struct dnswire_reader*, size_t);

enum dnswire_result               dnswire_reader_push(struct dnswire_reader*, const uint8_t*, size_t, uint8_t*, size_t*);
enum dnswire_result               dnswire_reader_read(struct dnswire_reader*, int);
static inline enum dnswire_result dnswire_reader_fread(struct dnswire_reader* handle, FILE* fp)
{
    return dnswire_reader_read(handle, fileno(fp));
}

#endif

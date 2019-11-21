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
#include <dnswire/dnstap.h>

#include <tinyframe/tinyframe.h>

#ifndef __dnswire_h_decoder
#define __dnswire_h_decoder 1

enum dnswire_decoder_state {
    dnswire_decoder_check_control_start,
    dnswire_decoder_check_control_field,
    dnswire_decoder_reading_frames,
    dnswire_decoder_done,
    dnswire_decoder_error,
};

struct dnswire_decoder {
    enum dnswire_decoder_state state;
    struct tinyframe_reader    reader;
    struct dnstap              dnstap;
};

#define DNSWIRE_DECODER_INITIALIZER                    \
    {                                                  \
        .state  = dnswire_decoder_check_control_start, \
        .reader = TINYFRAME_READER_INITIALIZER,        \
        .dnstap = DNSTAP_INITIALIZER,                  \
    }

#define dnswire_decoder_decoded(d) (d).reader.bytes_read
#define dnswire_decoder_dnstap(d) (&(d).dnstap)
#define dnswire_decoder_cleanup(d) dnstap_cleanup(&(d).dnstap)

enum dnswire_result dnswire_decoder_decode(struct dnswire_decoder*, const uint8_t*, size_t);

#endif

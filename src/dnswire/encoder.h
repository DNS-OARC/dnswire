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

#ifndef __dnswire_h_encoder
#define __dnswire_h_encoder 1

enum dnswire_encoder_state {
    dnswire_encoder_control_ready  = 0,
    dnswire_encoder_control_start  = 1,
    dnswire_encoder_control_accept = 2,
    dnswire_encoder_control_finish = 3,
    dnswire_encoder_frames         = 4,
    dnswire_encoder_control_stop   = 5,
    dnswire_encoder_done           = 6,
};
extern const char* const dnswire_encoder_state_string[];

struct dnswire_encoder {
    enum dnswire_encoder_state state;
    struct tinyframe_writer    writer;
    const struct dnstap*       dnstap;
};

#define DNSWIRE_ENCODER_INITIALIZER              \
    {                                            \
        .state  = dnswire_encoder_control_start, \
        .writer = TINYFRAME_WRITER_INITIALIZER,  \
        .dnstap = 0,                             \
    }

#define dnswire_encoder_set_dnstap(e, d) (e).dnstap = d
#define dnswire_encoder_encoded(e) (e).writer.bytes_wrote

enum dnswire_result dnswire_encoder_encode(struct dnswire_encoder*, uint8_t*, size_t);
enum dnswire_result dnswire_encoder_stop(struct dnswire_encoder*);

#endif

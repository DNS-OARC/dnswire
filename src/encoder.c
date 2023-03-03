/*
 * Author Jerry Lundström <jerry@dns-oarc.net>
 * Copyright (c) 2019-2023, OARC, Inc.
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

#include "config.h"

#include "dnswire/encoder.h"
#include "dnswire/trace.h"

#include <assert.h>

const char* const dnswire_encoder_state_string[] = {
    "control_ready",
    "control_start",
    "control_accept",
    "control_finish",
    "frames",
    "control_stop",
    "done",
};

#define __state(h, s)                                                                                     \
    __trace("state %s => %s", dnswire_encoder_state_string[(h)->state], dnswire_encoder_state_string[s]); \
    (h)->state = s;

static struct tinyframe_control_field _content_type = {
    TINYFRAME_CONTROL_FIELD_CONTENT_TYPE,
    DNSTAP_PROTOBUF_CONTENT_TYPE_LENGTH,
    (uint8_t*)DNSTAP_PROTOBUF_CONTENT_TYPE,
};

enum dnswire_result dnswire_encoder_encode(struct dnswire_encoder* handle, uint8_t* out, size_t len)
{
    assert(handle);
    assert(out);
    assert(len);

    switch (handle->state) {
    case dnswire_encoder_control_ready:
        switch (tinyframe_write_control(&handle->writer, out, len, TINYFRAME_CONTROL_READY, &_content_type, 1)) {
        case tinyframe_ok:
            __state(handle, dnswire_encoder_control_start);
            return dnswire_again;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_encoder_control_start:
        switch (tinyframe_write_control_start(&handle->writer, out, len, DNSTAP_PROTOBUF_CONTENT_TYPE, DNSTAP_PROTOBUF_CONTENT_TYPE_LENGTH)) {
        case tinyframe_ok:
            __state(handle, dnswire_encoder_frames);
            return dnswire_again;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_encoder_control_accept:
        switch (tinyframe_write_control(&handle->writer, out, len, TINYFRAME_CONTROL_ACCEPT, &_content_type, 1)) {
        case tinyframe_ok:
            __state(handle, dnswire_encoder_control_finish);
            return dnswire_again;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_encoder_control_finish:
        switch (tinyframe_write_control(&handle->writer, out, len, TINYFRAME_CONTROL_FINISH, 0, 0)) {
        case tinyframe_ok:
            __state(handle, dnswire_encoder_done);
            return dnswire_endofdata;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_encoder_frames: {
        if (!handle->dnstap) {
            return dnswire_error;
        }

        size_t frame_len = dnstap_encode_protobuf_size(handle->dnstap);
        if (len < tinyframe_frame_size(frame_len)) {
            return dnswire_need_more;
        }
        uint8_t frame[frame_len];
        dnstap_encode_protobuf(handle->dnstap, frame);

        switch (tinyframe_write_frame(&handle->writer, out, len, frame, sizeof(frame))) {
        case tinyframe_ok:
            return dnswire_ok;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;
    }

    case dnswire_encoder_control_stop:
        switch (tinyframe_write_control_stop(&handle->writer, out, len)) {
        case tinyframe_ok:
            __state(handle, dnswire_encoder_done);
            return dnswire_endofdata;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_encoder_done:
        break;
    }

    return dnswire_error;
}

enum dnswire_result dnswire_encoder_stop(struct dnswire_encoder* handle)
{
    assert(handle);

    switch (handle->state) {
    case dnswire_encoder_frames:
        __state(handle, dnswire_encoder_control_stop);
        return dnswire_ok;

    default:
        break;
    }

    return dnswire_error;
}

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

#include "config.h"

#include "dnswire/encoder.h"

#include <assert.h>

enum dnswire_result dnswire_encoder_encode(struct dnswire_encoder* handle, uint8_t* out, size_t len)
{
    assert(handle);
    assert(out);
    assert(len);

    switch (handle->state) {
    case dnswire_encoder_done:
        return dnswire_error;

    case dnswire_encoder_error:
        return dnswire_error;

    case dnswire_encoder_control_start:
        switch (tinyframe_write_control_start(&handle->writer, out, len, DNSTAP_PROTOBUF_CONTENT_TYPE, DNSTAP_PROTOBUF_CONTENT_TYPE_LENGTH)) {
        case tinyframe_ok:
            handle->state = dnswire_encoder_frames;
            return dnswire_again;

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

        size_t  frame_len = dnstap_encode_protobuf_size(handle->dnstap);
        uint8_t frame[frame_len];
        dnstap_encode_protobuf(handle->dnstap, frame);

        switch (tinyframe_write_frame(&handle->writer, out, len, frame, sizeof(frame))) {
        case tinyframe_ok:
            handle->state = dnswire_encoder_frames;
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
            handle->state = dnswire_encoder_done;
            return dnswire_endofdata;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;
    }

    return dnswire_error;
}

enum dnswire_result dnswire_encoder_stop(struct dnswire_encoder* handle)
{
    assert(handle);

    switch (handle->state) {
    case dnswire_encoder_frames:
        handle->state = dnswire_encoder_control_stop;
        return dnswire_ok;

    default:
        break;
    }

    return dnswire_error;
}

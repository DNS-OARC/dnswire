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

#include "dnswire/decoder.h"

#include <assert.h>

enum dnswire_result dnswire_decoder_decode(struct dnswire_decoder* handle, const uint8_t* data, size_t len)
{
    assert(handle);
    assert(data);
    assert(len);

    switch (handle->state) {
    case dnswire_decoder_done:
        return dnswire_endofdata;

    case dnswire_decoder_error:
        return dnswire_error;

    default:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_have_control:
            if (handle->state != dnswire_decoder_check_control_start) {
                return dnswire_error;
            }
            if (handle->reader.control.type != TINYFRAME_CONTROL_START) {
                return dnswire_error;
            }
            handle->state = dnswire_decoder_check_control_field;
            return dnswire_again;

        case tinyframe_have_control_field:
            if (handle->state != dnswire_decoder_check_control_field) {
                return dnswire_error;
            }
            if (handle->reader.control_field.type != TINYFRAME_CONTROL_FIELD_CONTENT_TYPE
                || strncmp(DNSTAP_PROTOBUF_CONTENT_TYPE, (const char*)handle->reader.control_field.data, handle->reader.control_field.length)) {
                return dnswire_error;
            }
            handle->state = dnswire_decoder_reading_frames;
            return dnswire_again;

        case tinyframe_have_frame:
            if (handle->state != dnswire_decoder_reading_frames) {
                return dnswire_error;
            }
            dnstap_cleanup(&handle->dnstap);
            if (dnstap_decode_protobuf(&handle->dnstap, handle->reader.frame.data, handle->reader.frame.length)) {
                return dnswire_error;
            }
            return dnswire_have_dnstap;

        case tinyframe_need_more:
            return dnswire_need_more;

        case tinyframe_stopped:
        case tinyframe_finished:
            handle->state = dnswire_decoder_done;
            return dnswire_endofdata;

        default:
            break;
        }
    }

    handle->state = dnswire_decoder_done;
    return dnswire_error;
}

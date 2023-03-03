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

#include "dnswire/decoder.h"
#include "dnswire/trace.h"

#include <assert.h>

const char* const dnswire_decoder_state_string[] = {
    "reading_control",
    "checking_ready",
    "checking_accept",
    "reading_start",
    "checking_start",
    "reading_frames",
    "checking_finish",
    "done",
};

#define __state(h, s)                                                                                     \
    __trace("state %s => %s", dnswire_decoder_state_string[(h)->state], dnswire_decoder_state_string[s]); \
    (h)->state = s;

enum dnswire_result dnswire_decoder_decode(struct dnswire_decoder* handle, const uint8_t* data, size_t len)
{
    assert(handle);
    assert(data);
    assert(len);

    switch (handle->state) {
    case dnswire_decoder_reading_control:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_have_control:
            switch (handle->reader.control.type) {
            case TINYFRAME_CONTROL_READY:
                // indicate that we have bidirectional communications
                __state(handle, dnswire_decoder_checking_ready);
                return dnswire_again;

            case TINYFRAME_CONTROL_ACCEPT:
                // indicate that we have bidirectional communications
                __state(handle, dnswire_decoder_checking_accept);
                return dnswire_again;

            case TINYFRAME_CONTROL_START:
                __state(handle, dnswire_decoder_checking_start);
                return dnswire_again;

            default:
                break;
            }
            break;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_decoder_checking_ready:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_have_control_field:
            if (handle->reader.control_field.type != TINYFRAME_CONTROL_FIELD_CONTENT_TYPE) {
                return dnswire_error;
            }
            if (!strncmp(DNSTAP_PROTOBUF_CONTENT_TYPE, (const char*)handle->reader.control_field.data, handle->reader.control_field.length)) {
                handle->ready_support_dnstap_protobuf = 1;
            }
            if (!handle->reader.control_length_left) {
                __state(handle, dnswire_decoder_reading_start);
                return dnswire_bidirectional;
            }
            return dnswire_again;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            return dnswire_error;
        }

    case dnswire_decoder_checking_accept:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_have_control_field:
            if (handle->reader.control_field.type != TINYFRAME_CONTROL_FIELD_CONTENT_TYPE) {
                return dnswire_error;
            }
            if (!strncmp(DNSTAP_PROTOBUF_CONTENT_TYPE, (const char*)handle->reader.control_field.data, handle->reader.control_field.length)) {
                handle->accept_support_dnstap_protobuf = 1;
            }
            if (!handle->reader.control_length_left) {
                __state(handle, dnswire_decoder_checking_finish);
                return dnswire_bidirectional;
            }
            return dnswire_again;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            return dnswire_error;
        }

    case dnswire_decoder_reading_start:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_have_control:
            switch (handle->reader.control.type) {
            case TINYFRAME_CONTROL_START:
                __state(handle, dnswire_decoder_checking_start);
                return dnswire_again;

            default:
                break;
            }
            return dnswire_error;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            return dnswire_error;
        }

    case dnswire_decoder_checking_start:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_have_control_field:
            if (handle->reader.control_field.type != TINYFRAME_CONTROL_FIELD_CONTENT_TYPE) {
                return dnswire_error;
            }
            if (strncmp(DNSTAP_PROTOBUF_CONTENT_TYPE, (const char*)handle->reader.control_field.data, handle->reader.control_field.length)) {
                return dnswire_error;
            }
            __state(handle, dnswire_decoder_reading_frames);
            return dnswire_again;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            return dnswire_error;
        }

    case dnswire_decoder_reading_frames:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_have_frame:
            dnstap_cleanup(&handle->dnstap);
            if (dnstap_decode_protobuf(&handle->dnstap, handle->reader.frame.data, handle->reader.frame.length)) {
                return dnswire_error;
            }
            return dnswire_have_dnstap;

        case tinyframe_need_more:
            return dnswire_need_more;

        case tinyframe_stopped:
            __state(handle, dnswire_decoder_done);
            return dnswire_endofdata;

        default:
            return dnswire_error;
        }

    case dnswire_decoder_checking_finish:
        switch (tinyframe_read(&handle->reader, data, len)) {
        case tinyframe_finished:
            __state(handle, dnswire_decoder_done);
            return dnswire_endofdata;

        case tinyframe_need_more:
            return dnswire_need_more;

        default:
            return dnswire_error;
        }

    case dnswire_decoder_done:
        break;
    }

    return dnswire_error;
}

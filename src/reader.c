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

#include "dnswire/reader.h"
#include "dnswire/trace.h"

#include <assert.h>
#include <stdlib.h>

const char* const dnswire_reader_state_string[] = {
    "reading_control",
    "decoding_control",
    "encoding_accept",
    "writing_accept",
    "reading",
    "decoding",
    "encoding_finish",
    "writing_finish",
    "done",
};

#define __state(h, s)                                                                                   \
    __trace("state %s => %s", dnswire_reader_state_string[(h)->state], dnswire_reader_state_string[s]); \
    (h)->state = s;

static struct dnswire_reader _defaults = {
    .state = dnswire_reader_reading_control,

    .decoder = DNSWIRE_DECODER_INITIALIZER,
    .buf     = 0,
    .size    = DNSWIRE_DEFAULT_BUF_SIZE,
    .inc     = DNSWIRE_DEFAULT_BUF_SIZE,
    .max     = DNSWIRE_MAXIMUM_BUF_SIZE,
    .at      = 0,
    .left    = 0,
    .pushed  = 0,

    .encoder    = DNSWIRE_ENCODER_INITIALIZER,
    .write_buf  = 0,
    .write_size = DNSWIRE_DEFAULT_BUF_SIZE,
    .write_inc  = DNSWIRE_DEFAULT_BUF_SIZE,
    .write_max  = DNSWIRE_MAXIMUM_BUF_SIZE,
    .write_at   = 0,
    .write_left = 0,

    .allow_bidirectional = false,
    .is_bidirectional    = false,
};

enum dnswire_result dnswire_reader_init(struct dnswire_reader* handle)
{
    assert(handle);

    *handle = _defaults;

    if (!(handle->buf = malloc(handle->size))) {
        return dnswire_error;
    }

    return dnswire_ok;
}

enum dnswire_result dnswire_reader_allow_bidirectional(struct dnswire_reader* handle, bool allow_bidirectional)
{
    assert(handle);

    if (allow_bidirectional && !handle->write_buf) {
        if (!(handle->write_buf = malloc(handle->write_size))) {
            return dnswire_error;
        }

        handle->encoder.state = dnswire_encoder_control_accept;
    }

    handle->allow_bidirectional = allow_bidirectional;

    return dnswire_ok;
}

enum dnswire_result dnswire_reader_set_bufsize(struct dnswire_reader* handle, size_t size)
{
    assert(handle);
    assert(size);
    assert(handle->buf);

    if (handle->left > size) {
        // we got data and it doesn't fit in the new size
        return dnswire_error;
    }
    if (size > handle->max) {
        // don't expand over max
        return dnswire_error;
    }

    if (handle->at + handle->left > size) {
        // move what's left to the start
        if (handle->left) {
            memmove(handle->buf, &handle->buf[handle->at], handle->left);
        }
        handle->at = 0;
    }

    uint8_t* buf = realloc(handle->buf, size);
    if (!buf) {
        return dnswire_error;
    }

    handle->buf  = buf;
    handle->size = size;

    return dnswire_ok;
}

enum dnswire_result dnswire_reader_set_bufinc(struct dnswire_reader* handle, size_t inc)
{
    assert(handle);
    assert(inc);

    handle->inc = inc;

    return dnswire_ok;
}

enum dnswire_result dnswire_reader_set_bufmax(struct dnswire_reader* handle, size_t max)
{
    assert(handle);
    assert(max);

    if (max < handle->size) {
        return dnswire_error;
    }

    handle->max = max;

    return dnswire_ok;
}

static enum dnswire_result _encoding(struct dnswire_reader* handle)
{
    enum dnswire_result res;

    while (1) {
        res = dnswire_encoder_encode(&handle->encoder, &handle->write_buf[handle->write_at], handle->write_size - handle->write_at);
        __trace("encode %s", dnswire_result_string[res]);

        switch (res) {
        case dnswire_ok:
        case dnswire_again:
        case dnswire_endofdata:
            handle->write_at += dnswire_encoder_encoded(handle->encoder);
            handle->write_left += dnswire_encoder_encoded(handle->encoder);
            break;

        case dnswire_need_more: {
            if (handle->write_size >= handle->write_max) {
                // already at max size and it's not enough
                return dnswire_error;
            }

            // no space left, expand
            size_t   size = handle->write_size + handle->write_inc > handle->write_max ? handle->write_max : handle->write_size + handle->write_inc;
            uint8_t* buf  = realloc(handle->write_buf, size);
            if (!buf) {
                return dnswire_error;
            }
            handle->write_buf  = buf;
            handle->write_size = size;
            continue;
        }
        default:
            break;
        }
        break;
    }
    return res;
}

enum dnswire_result dnswire_reader_push(struct dnswire_reader* handle, const uint8_t* data, size_t len, uint8_t* out_data, size_t* out_len)
{
    assert(handle);
    assert(data);
    assert(handle->buf);
    assert(!handle->allow_bidirectional || out_data);
    assert(!handle->allow_bidirectional || out_len);

    handle->pushed      = 0;
    size_t out_len_orig = 0;
    if (out_len) {
        out_len_orig = *out_len;
        *out_len     = 0;
    }

    switch (handle->state) {
    case dnswire_reader_reading_control:
        if (!len) {
            return dnswire_need_more;
        }
        // if the space we have left is less then len we only move that amount
        handle->pushed = handle->size - handle->at - handle->left < len ? handle->size - handle->at - handle->left : len;
        // if we can't add more we fallthrough and let it decode some or expand the buffer
        if (handle->pushed) {
            memcpy(&handle->buf[handle->at + handle->left], data, handle->pushed);
            __trace("%s", __printable_string(&handle->buf[handle->at + handle->left], handle->pushed));
            handle->left += handle->pushed;
        }
        __state(handle, dnswire_reader_decoding_control);
    // fallthrough

    case dnswire_reader_decoding_control:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->buf[handle->at], handle->left)) {
        case dnswire_bidirectional:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading_control);
            }

            if (!handle->allow_bidirectional) {
                return dnswire_error;
            }

            handle->is_bidirectional = true;

            if (!handle->decoder.ready_support_dnstap_protobuf) {
                return dnswire_error;
            }

            __state(handle, dnswire_reader_encoding_accept);
            return dnswire_again;

        case dnswire_again:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading_control);
            }
            return dnswire_again;

        case dnswire_need_more:
            if (handle->left < handle->size) {
                // still space left to fill
                if (handle->at) {
                    // move what's left to the start
                    if (handle->left) {
                        memmove(handle->buf, &handle->buf[handle->at], handle->left);
                    }
                    handle->at = 0;
                }
            } else if (handle->size < handle->max) {
                // no space left, expand
                size_t   size = handle->size + handle->inc > handle->max ? handle->max : handle->size + handle->inc;
                uint8_t* buf  = realloc(handle->buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->buf  = buf;
                handle->size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_reader_reading_control);
            if (len && handle->pushed < len) {
                // we had more to push but won't loop, call again please
                return dnswire_again;
            }
            return dnswire_need_more;

        case dnswire_have_dnstap:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (handle->left) {
                __state(handle, dnswire_reader_decoding);
            } else {
                handle->at = 0;
                __state(handle, dnswire_reader_reading);
            }
            return dnswire_have_dnstap;

        case dnswire_endofdata:
            if (handle->is_bidirectional) {
                __state(handle, dnswire_reader_encoding_finish);
                return dnswire_again;
            }
            __state(handle, dnswire_reader_done);
            return dnswire_endofdata;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_reader_encoding_accept:
        switch (_encoding(handle)) {
        case dnswire_again:
            __state(handle, dnswire_reader_writing_accept);
            break;
        // fallthrough

        default:
            return dnswire_error;
        }

    case dnswire_reader_writing_accept:
        // if what we have left to write is less then the space available, we write it all
        assert(out_len);
        *out_len = handle->write_left < out_len_orig ? handle->write_left : out_len_orig;
        memcpy(out_data, &handle->write_buf[handle->write_at - handle->write_left], *out_len);
        __trace("pushed %zu", *out_len);
        handle->write_left -= *out_len;
        __trace("left %zu", handle->write_left);
        if (!handle->write_left) {
            handle->write_at = 0;
            __state(handle, dnswire_reader_reading_control);
        }
        return dnswire_again;

    case dnswire_reader_reading:
        if (!len) {
            return dnswire_need_more;
        }
        // if the space we have left is less then len we only move that amount
        handle->pushed = handle->size - handle->at - handle->left < len ? handle->size - handle->at - handle->left : len;
        // if we can't add more we fallthrough and let it decode some or expand the buffer
        if (handle->pushed) {
            memcpy(&handle->buf[handle->at + handle->left], data, handle->pushed);
            __trace("%s", __printable_string(&handle->buf[handle->at + handle->left], handle->pushed));
            handle->left += handle->pushed;
        }
        __state(handle, dnswire_reader_decoding);
    // fallthrough

    case dnswire_reader_decoding:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->buf[handle->at], handle->left)) {
        case dnswire_again:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading);
            }
            return dnswire_again;

        case dnswire_need_more:
            if (handle->left < handle->size) {
                // still space left to fill
                if (handle->at) {
                    // move what's left to the start
                    if (handle->left) {
                        memmove(handle->buf, &handle->buf[handle->at], handle->left);
                    }
                    handle->at = 0;
                }
            } else if (handle->size < handle->max) {
                // no space left, expand
                size_t   size = handle->size + handle->inc > handle->max ? handle->max : handle->size + handle->inc;
                uint8_t* buf  = realloc(handle->buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->buf  = buf;
                handle->size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_reader_reading);
            if (len && handle->pushed < len) {
                // we had more to push but won't loop, call again please
                return dnswire_again;
            }
            return dnswire_need_more;

        case dnswire_have_dnstap:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading);
            }
            return dnswire_have_dnstap;

        case dnswire_endofdata:
            if (handle->is_bidirectional) {
                __state(handle, dnswire_reader_encoding_finish);
                return dnswire_again;
            }
            __state(handle, dnswire_reader_done);
            return dnswire_endofdata;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_reader_encoding_finish:
        switch (_encoding(handle)) {
        case dnswire_endofdata:
            __state(handle, dnswire_reader_writing_finish);
            break;
        // fallthrough

        default:
            return dnswire_error;
        }

    case dnswire_reader_writing_finish:
        // if what we have left to write is less then the space available, we write it all
        assert(out_len);
        *out_len = handle->write_left < out_len_orig ? handle->write_left : out_len_orig;
        memcpy(out_data, &handle->write_buf[handle->write_at - handle->write_left], *out_len);
        __trace("pushed %zu", *out_len);
        handle->write_left -= *out_len;
        __trace("left %zu", handle->write_left);
        if (!handle->write_left) {
            handle->write_at = 0;
            __state(handle, dnswire_reader_done);
            return dnswire_endofdata;
        }
        return dnswire_again;

    case dnswire_reader_done:
        return dnswire_error;
    }

    return dnswire_error;
}

enum dnswire_result dnswire_reader_read(struct dnswire_reader* handle, int fd)
{
    assert(handle);
    assert(handle->buf);

    switch (handle->state) {
    case dnswire_reader_reading_control: {
        ssize_t nread = read(fd, &handle->buf[handle->at + handle->left], handle->size - handle->at - handle->left);
        if (nread < 0) {
            // TODO
            return dnswire_error;
        } else if (!nread) {
            // TODO
            return dnswire_error;
        }
        __trace("%s", __printable_string(&handle->buf[handle->at + handle->left], nread));
        handle->left += nread;
        __state(handle, dnswire_reader_decoding_control);
        // fallthrough
    }

    case dnswire_reader_decoding_control:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->buf[handle->at], handle->left)) {
        case dnswire_bidirectional:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading_control);
            }

            if (!handle->allow_bidirectional) {
                return dnswire_error;
            }

            handle->is_bidirectional = true;

            if (!handle->decoder.ready_support_dnstap_protobuf) {
                return dnswire_error;
            }

            __state(handle, dnswire_reader_encoding_accept);
            return dnswire_again;

        case dnswire_again:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading_control);
            }
            return dnswire_again;

        case dnswire_need_more:
            if (handle->left < handle->size) {
                // still space left to fill
                if (handle->at) {
                    // move what's left to the start
                    if (handle->left) {
                        memmove(handle->buf, &handle->buf[handle->at], handle->left);
                    }
                    handle->at = 0;
                }
            } else if (handle->size < handle->max) {
                // no space left, expand
                size_t   size = handle->size + handle->inc > handle->max ? handle->max : handle->size + handle->inc;
                uint8_t* buf  = realloc(handle->buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->buf  = buf;
                handle->size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_reader_reading_control);
            return dnswire_need_more;

        case dnswire_have_dnstap:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (handle->left) {
                __state(handle, dnswire_reader_decoding);
            } else {
                handle->at = 0;
                __state(handle, dnswire_reader_reading);
            }
            return dnswire_have_dnstap;

        case dnswire_endofdata:
            if (handle->is_bidirectional) {
                __state(handle, dnswire_reader_encoding_finish);
                return dnswire_again;
            }
            __state(handle, dnswire_reader_done);
            return dnswire_endofdata;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_reader_encoding_accept:
        switch (_encoding(handle)) {
        case dnswire_again:
            __state(handle, dnswire_reader_writing_accept);
            break;
        // fallthrough

        default:
            return dnswire_error;
        }

    case dnswire_reader_writing_accept: {
        ssize_t nwrote = write(fd, &handle->write_buf[handle->write_at - handle->write_left], handle->write_left);
        __trace("wrote %zd", nwrote);
        if (nwrote < 0) {
            // TODO
            return dnswire_error;
        } else if (!nwrote) {
            // TODO
            return dnswire_error;
        }

        handle->write_left -= nwrote;
        __trace("left %zu", handle->write_left);
        if (!handle->write_left) {
            handle->write_at = 0;
            __state(handle, dnswire_reader_reading_control);
        }
        return dnswire_again;
    }

    case dnswire_reader_reading: {
        ssize_t nread = read(fd, &handle->buf[handle->at + handle->left], handle->size - handle->at - handle->left);
        if (nread < 0) {
            // TODO
            return dnswire_error;
        } else if (!nread) {
            // TODO
            return dnswire_error;
        }
        __trace("%s", __printable_string(&handle->buf[handle->at + handle->left], nread));
        handle->left += nread;
        __state(handle, dnswire_reader_decoding);
        // fallthrough
    }

    case dnswire_reader_decoding:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->buf[handle->at], handle->left)) {
        case dnswire_again:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading);
            }
            return dnswire_again;

        case dnswire_need_more:
            if (handle->left < handle->size) {
                // still space left to fill
                if (handle->at) {
                    // move what's left to the start
                    if (handle->left) {
                        memmove(handle->buf, &handle->buf[handle->at], handle->left);
                    }
                    handle->at = 0;
                }
            } else if (handle->size < handle->max) {
                // no space left, expand
                size_t   size = handle->size + handle->inc > handle->max ? handle->max : handle->size + handle->inc;
                uint8_t* buf  = realloc(handle->buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->buf  = buf;
                handle->size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_reader_reading);
            return dnswire_need_more;

        case dnswire_have_dnstap:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->left) {
                handle->at = 0;
                __state(handle, dnswire_reader_reading);
            }
            return dnswire_have_dnstap;

        case dnswire_endofdata:
            if (handle->is_bidirectional) {
                __state(handle, dnswire_reader_encoding_finish);
                return dnswire_again;
            }
            __state(handle, dnswire_reader_done);
            return dnswire_endofdata;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_reader_encoding_finish:
        switch (_encoding(handle)) {
        case dnswire_endofdata:
            __state(handle, dnswire_reader_writing_finish);
            break;
        // fallthrough

        default:
            return dnswire_error;
        }

    case dnswire_reader_writing_finish: {
        ssize_t nwrote = write(fd, &handle->write_buf[handle->write_at - handle->write_left], handle->write_left);
        __trace("wrote %zd", nwrote);
        if (nwrote < 0) {
            // TODO
            return dnswire_error;
        } else if (!nwrote) {
            // TODO
            return dnswire_error;
        }

        handle->write_left -= nwrote;
        __trace("left %zu", handle->write_left);
        if (!handle->write_left) {
            handle->write_at = 0;
            __state(handle, dnswire_reader_done);
            return dnswire_endofdata;
        }
        return dnswire_again;
    }

    case dnswire_reader_done:
        return dnswire_error;
    }

    return dnswire_error;
}

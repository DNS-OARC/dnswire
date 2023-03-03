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

#include "dnswire/writer.h"
#include "dnswire/trace.h"
#include "dnswire/dnswire.h"

#include <assert.h>
#include <stdlib.h>

const char* const dnswire_writer_state_string[] = {
    "encoding_ready",
    "writing_ready",
    "reading_accept",
    "decoding_accept",
    "encoding",
    "writing",
    "stopping",
    "encoding_stop",
    "writing_stop",
    "reading_finish",
    "decoding_finish",
    "done",
};

#define __state(h, s)                                                                                   \
    __trace("state %s => %s", dnswire_writer_state_string[(h)->state], dnswire_writer_state_string[s]); \
    (h)->state = s;

static struct dnswire_writer _defaults = {
    .state = dnswire_writer_encoding,

    .encoder = DNSWIRE_ENCODER_INITIALIZER,
    .buf     = 0,
    .size    = DNSWIRE_DEFAULT_BUF_SIZE,
    .inc     = DNSWIRE_DEFAULT_BUF_SIZE,
    .max     = DNSWIRE_MAXIMUM_BUF_SIZE,
    .at      = 0,
    .left    = 0,
    .popped  = 0,

    .decoder     = DNSWIRE_DECODER_INITIALIZER,
    .read_buf    = 0,
    .read_size   = DNSWIRE_DEFAULT_BUF_SIZE,
    .read_inc    = DNSWIRE_DEFAULT_BUF_SIZE,
    .read_max    = DNSWIRE_MAXIMUM_BUF_SIZE,
    .read_at     = 0,
    .read_left   = 0,
    .read_pushed = 0,

    .bidirectional = false,
};

enum dnswire_result dnswire_writer_init(struct dnswire_writer* handle)
{
    assert(handle);

    *handle = _defaults;

    if (!(handle->buf = malloc(handle->size))) {
        return dnswire_error;
    }

    return dnswire_ok;
}

enum dnswire_result dnswire_writer_set_bidirectional(struct dnswire_writer* handle, bool bidirectional)
{
    assert(handle);

    if (bidirectional) {
        if (!handle->read_buf) {
            if (!(handle->read_buf = malloc(handle->read_size))) {
                return dnswire_error;
            }
        }

        handle->encoder.state = dnswire_encoder_control_ready;
        __state(handle, dnswire_writer_encoding_ready);
    } else {
        handle->encoder.state = dnswire_encoder_control_start;
        __state(handle, dnswire_writer_encoding);
    }

    handle->bidirectional = bidirectional;

    return dnswire_ok;
}

enum dnswire_result dnswire_writer_set_bufsize(struct dnswire_writer* handle, size_t size)
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

enum dnswire_result dnswire_writer_set_bufinc(struct dnswire_writer* handle, size_t inc)
{
    assert(handle);
    assert(inc);

    handle->inc = inc;

    return dnswire_ok;
}

enum dnswire_result dnswire_writer_set_bufmax(struct dnswire_writer* handle, size_t max)
{
    assert(handle);
    assert(max);

    if (max < handle->size) {
        return dnswire_error;
    }

    handle->max = max;

    return dnswire_ok;
}

static enum dnswire_result _encoding(struct dnswire_writer* handle)
{
    enum dnswire_result res;

    while (1) {
        res = dnswire_encoder_encode(&handle->encoder, &handle->buf[handle->at], handle->size - handle->at);
        __trace("encode %s", dnswire_result_string[res]);

        switch (res) {
        case dnswire_ok:
        case dnswire_again:
        case dnswire_endofdata:
            handle->at += dnswire_encoder_encoded(handle->encoder);
            handle->left += dnswire_encoder_encoded(handle->encoder);
            break;

        case dnswire_need_more: {
            if (handle->size >= handle->max) {
                // already at max size and it's not enough
                return dnswire_error;
            }

            // no space left, expand
            size_t   size = handle->size + handle->inc > handle->max ? handle->max : handle->size + handle->inc;
            uint8_t* buf  = realloc(handle->buf, size);
            if (!buf) {
                return dnswire_error;
            }
            handle->buf  = buf;
            handle->size = size;
            continue;
        }
        default:
            break;
        }
        break;
    }
    return res;
}

enum dnswire_result dnswire_writer_pop(struct dnswire_writer* handle, uint8_t* data, size_t len, uint8_t* in_data, size_t* in_len)
{
    assert(handle);
    assert(data);
    assert(len);
    assert(handle->buf);
    assert(!handle->bidirectional || in_data);

    handle->popped     = 0;
    size_t in_len_orig = 0;
    if (in_len) {
        in_len_orig = *in_len;
        *in_len     = 0;
    }

    enum dnswire_result res = dnswire_again;

    __trace("state %s", dnswire_writer_state_string[handle->state]);

    switch (handle->state) {
    case dnswire_writer_encoding_ready:
        res = _encoding(handle);
        __trace("left %zu", handle->left);
        if (res != dnswire_error && handle->left) {
            __state(handle, dnswire_writer_writing);
            // fallthrough
        } else {
            break;
        }

    case dnswire_writer_writing_ready:
        handle->popped = len < handle->left ? len : handle->left;
        memcpy(data, &handle->buf[handle->at - handle->left], handle->popped);
        __trace("wrote %zd", handle->popped);
        handle->left -= handle->popped;
        __trace("left %zu", handle->left);
        if (handle->left) {
            break;
        }
        handle->at = 0;
        __state(handle, dnswire_writer_reading_accept);

    case dnswire_writer_reading_accept:
        if (!in_len_orig) {
            return dnswire_need_more;
        }
        *in_len = handle->read_size - handle->read_at - handle->read_left < in_len_orig ? handle->read_size - handle->read_at - handle->read_left : in_len_orig;
        if (*in_len) {
            memcpy(&handle->read_buf[handle->read_at + handle->read_left], in_data, *in_len);
            __trace("%s", __printable_string(&handle->read_buf[handle->read_at + handle->read_left], *in_len));
            handle->left += *in_len;
        }
        __state(handle, dnswire_writer_decoding_accept);
        // fallthrough

    case dnswire_writer_decoding_accept:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->read_buf[handle->read_at], handle->read_left)) {
        case dnswire_bidirectional:
            handle->read_at += dnswire_decoder_decoded(handle->decoder);
            handle->read_left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->read_left) {
                handle->read_at = 0;
            }

            if (!handle->decoder.accept_support_dnstap_protobuf) {
                return dnswire_error;
            }

            __state(handle, dnswire_writer_encoding);
            return dnswire_again;

        case dnswire_again:
            handle->read_at += dnswire_decoder_decoded(handle->decoder);
            handle->read_left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->read_left) {
                handle->read_at = 0;
                __state(handle, dnswire_writer_reading_accept);
            }
            return dnswire_again;

        case dnswire_need_more:
            if (handle->read_left < handle->read_size) {
                // still space left to fill
                if (handle->read_at) {
                    // move what's left to the start
                    if (handle->read_left) {
                        memmove(handle->read_buf, &handle->read_buf[handle->read_at], handle->read_left);
                    }
                    handle->read_at = 0;
                }
            } else if (handle->read_size < handle->read_max) {
                // no space left, expand
                size_t   size = handle->read_size + handle->read_inc > handle->read_max ? handle->read_max : handle->read_size + handle->read_inc;
                uint8_t* buf  = realloc(handle->read_buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->read_buf  = buf;
                handle->read_size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_writer_reading_accept);
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_writer_encoding:
        res = _encoding(handle);
        __trace("left %zu", handle->left);
        if (res != dnswire_error && handle->left) {
            __state(handle, dnswire_writer_writing);
            // fallthrough
        } else {
            break;
        }

    case dnswire_writer_writing:
        handle->popped = len < handle->left ? len : handle->left;
        memcpy(data, &handle->buf[handle->at - handle->left], handle->popped);
        __trace("wrote %zd", handle->popped);
        handle->left -= handle->popped;
        __trace("left %zu", handle->left);
        if (!handle->left) {
            handle->at = 0;
            __state(handle, dnswire_writer_encoding);
        }
        break;

    case dnswire_writer_stopping:
        if (handle->left) {
            handle->popped = len < handle->left ? len : handle->left;
            memcpy(data, &handle->buf[handle->at - handle->left], handle->popped);
            __trace("wrote %zd", handle->popped);
            handle->left -= handle->popped;
            if (handle->left) {
                __trace("left %zu", handle->left);
                return dnswire_again;
            }
            handle->at = 0;
        }
        __state(handle, dnswire_writer_encoding_stop);
        // fallthrough

    case dnswire_writer_encoding_stop:
        res = _encoding(handle);
        if (res == dnswire_endofdata) {
            __state(handle, dnswire_writer_writing_stop);
            // fallthrough
        } else {
            break;
        }

    case dnswire_writer_writing_stop:
        if (handle->left) {
            handle->popped = len < handle->left ? len : handle->left;
            memcpy(data, &handle->buf[handle->at - handle->left], handle->popped);
            __trace("wrote %zd", handle->popped);
            handle->left -= handle->popped;
            if (handle->left) {
                __trace("left %zu", handle->left);
                return dnswire_again;
            }
            handle->at = 0;
        }
        if (handle->bidirectional) {
            __state(handle, dnswire_writer_reading_finish);
            return dnswire_again;
        }
        __state(handle, dnswire_writer_done);
        return dnswire_endofdata;

    case dnswire_writer_reading_finish:
        if (!in_len_orig) {
            return dnswire_need_more;
        }
        *in_len = handle->read_size - handle->read_at - handle->read_left < in_len_orig ? handle->read_size - handle->read_at - handle->read_left : in_len_orig;
        if (*in_len) {
            memcpy(&handle->read_buf[handle->read_at + handle->read_left], in_data, *in_len);
            __trace("%s", __printable_string(&handle->read_buf[handle->read_at + handle->read_left], *in_len));
            handle->left += *in_len;
        }
        __state(handle, dnswire_writer_decoding_finish);
        // fallthrough

    case dnswire_writer_decoding_finish:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->read_buf[handle->read_at], handle->read_left)) {
        case dnswire_endofdata:
            __state(handle, dnswire_writer_done);
            return dnswire_endofdata;

        case dnswire_need_more:
            if (handle->read_left < handle->read_size) {
                // still space left to fill
                if (handle->read_at) {
                    // move what's left to the start
                    if (handle->read_left) {
                        memmove(handle->read_buf, &handle->read_buf[handle->read_at], handle->read_left);
                    }
                    handle->read_at = 0;
                }
            } else if (handle->read_size < handle->read_max) {
                // no space left, expand
                size_t   size = handle->read_size + handle->read_inc > handle->read_max ? handle->read_max : handle->read_size + handle->read_inc;
                uint8_t* buf  = realloc(handle->read_buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->read_buf  = buf;
                handle->read_size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_writer_reading_accept);
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_writer_done:
        return dnswire_error;
    }

    return res;
}

enum dnswire_result dnswire_writer_write(struct dnswire_writer* handle, int fd)
{
    assert(handle);
    assert(handle->buf);

    enum dnswire_result res = dnswire_again;

    __trace("state %s", dnswire_writer_state_string[handle->state]);

    switch (handle->state) {
    case dnswire_writer_encoding_ready:
        res = _encoding(handle);
        __trace("left %zu", handle->left);
        if (res != dnswire_error && handle->left) {
            __state(handle, dnswire_writer_writing);
            // fallthrough
        } else {
            break;
        }

    case dnswire_writer_writing_ready: {
        ssize_t nwrote = write(fd, &handle->buf[handle->at - handle->left], handle->left);
        __trace("wrote %zd", nwrote);
        if (nwrote < 0) {
            // TODO
            return dnswire_error;
        } else if (!nwrote) {
            // TODO
            return dnswire_error;
        }

        handle->left -= nwrote;
        __trace("left %zu", handle->left);
        if (handle->left) {
            break;
        }
        handle->at = 0;
        __state(handle, dnswire_writer_reading_accept);
        // fallthrough
    }

    case dnswire_writer_reading_accept: {
        ssize_t nread = read(fd, &handle->read_buf[handle->read_at + handle->read_left], handle->read_size - handle->read_at - handle->read_left);
        if (nread < 0) {
            // TODO
            return dnswire_error;
        } else if (!nread) {
            // TODO
            return dnswire_error;
        }
        __trace("%s", __printable_string(&handle->read_buf[handle->read_at + handle->read_left], nread));
        handle->read_left += nread;
        __state(handle, dnswire_writer_decoding_accept);
        // fallthrough
    }

    case dnswire_writer_decoding_accept:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->read_buf[handle->read_at], handle->read_left)) {
        case dnswire_bidirectional:
            handle->read_at += dnswire_decoder_decoded(handle->decoder);
            handle->read_left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->read_left) {
                handle->read_at = 0;
            }

            if (!handle->decoder.accept_support_dnstap_protobuf) {
                return dnswire_error;
            }

            __state(handle, dnswire_writer_encoding);
            return dnswire_again;

        case dnswire_again:
            handle->read_at += dnswire_decoder_decoded(handle->decoder);
            handle->read_left -= dnswire_decoder_decoded(handle->decoder);
            if (!handle->read_left) {
                handle->read_at = 0;
                __state(handle, dnswire_writer_reading_accept);
            }
            return dnswire_again;

        case dnswire_need_more:
            if (handle->read_left < handle->read_size) {
                // still space left to fill
                if (handle->read_at) {
                    // move what's left to the start
                    if (handle->read_left) {
                        memmove(handle->read_buf, &handle->read_buf[handle->read_at], handle->read_left);
                    }
                    handle->read_at = 0;
                }
            } else if (handle->read_size < handle->read_max) {
                // no space left, expand
                size_t   size = handle->read_size + handle->read_inc > handle->read_max ? handle->read_max : handle->read_size + handle->read_inc;
                uint8_t* buf  = realloc(handle->read_buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->read_buf  = buf;
                handle->read_size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_writer_reading_accept);
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_writer_encoding:
        res = _encoding(handle);
        __trace("left %zu", handle->left);
        if (res != dnswire_error && handle->left) {
            __state(handle, dnswire_writer_writing);
            // fallthrough
        } else {
            break;
        }

    case dnswire_writer_writing: {
        ssize_t nwrote = write(fd, &handle->buf[handle->at - handle->left], handle->left);
        __trace("wrote %zd", nwrote);
        if (nwrote < 0) {
            // TODO
            return dnswire_error;
        } else if (!nwrote) {
            // TODO
            return dnswire_error;
        }

        handle->left -= nwrote;
        __trace("left %zu", handle->left);
        if (!handle->left) {
            handle->at = 0;
            __state(handle, dnswire_writer_encoding);
        }
        break;
    }

    case dnswire_writer_stopping:
        if (handle->left) {
            ssize_t nwrote = write(fd, &handle->buf[handle->at - handle->left], handle->left);
            __trace("wrote %zd", nwrote);
            if (nwrote < 0) {
                // TODO
                return dnswire_error;
            } else if (!nwrote) {
                // TODO
                return dnswire_error;
            }

            handle->left -= nwrote;
            if (handle->left) {
                __trace("left %zu", handle->left);
                return dnswire_again;
            }
            handle->at = 0;
        }
        __state(handle, dnswire_writer_encoding_stop);
        // fallthrough

    case dnswire_writer_encoding_stop:
        res = _encoding(handle);
        if (res == dnswire_endofdata) {
            __state(handle, dnswire_writer_writing_stop);
            // fallthrough
        } else {
            break;
        }

    case dnswire_writer_writing_stop:
        if (handle->left) {
            ssize_t nwrote = write(fd, &handle->buf[handle->at - handle->left], handle->left);
            __trace("wrote %zd", nwrote);
            if (nwrote < 0) {
                // TODO
                return dnswire_error;
            } else if (!nwrote) {
                // TODO
                return dnswire_error;
            }

            handle->left -= nwrote;
            if (handle->left) {
                __trace("left %zu", handle->left);
                return dnswire_again;
            }
            handle->at = 0;
        }
        if (handle->bidirectional) {
            __state(handle, dnswire_writer_reading_finish);
            return dnswire_again;
        }
        __state(handle, dnswire_writer_done);
        return dnswire_endofdata;

    case dnswire_writer_reading_finish: {
        ssize_t nread = read(fd, &handle->read_buf[handle->read_at + handle->read_left], handle->read_size - handle->read_at - handle->read_left);
        if (nread < 0) {
            // TODO
            return dnswire_error;
        } else if (!nread) {
            // TODO
            return dnswire_error;
        }
        __trace("%s", __printable_string(&handle->read_buf[handle->read_at + handle->read_left], nread));
        handle->read_left += nread;
        __state(handle, dnswire_writer_decoding_finish);
        // fallthrough
    }

    case dnswire_writer_decoding_finish:
        switch (dnswire_decoder_decode(&handle->decoder, &handle->read_buf[handle->read_at], handle->read_left)) {
        case dnswire_endofdata:
            __state(handle, dnswire_writer_done);
            return dnswire_endofdata;

        case dnswire_need_more:
            if (handle->read_left < handle->read_size) {
                // still space left to fill
                if (handle->read_at) {
                    // move what's left to the start
                    if (handle->read_left) {
                        memmove(handle->read_buf, &handle->read_buf[handle->read_at], handle->read_left);
                    }
                    handle->read_at = 0;
                }
            } else if (handle->read_size < handle->read_max) {
                // no space left, expand
                size_t   size = handle->read_size + handle->read_inc > handle->read_max ? handle->read_max : handle->read_size + handle->read_inc;
                uint8_t* buf  = realloc(handle->read_buf, size);
                if (!buf) {
                    return dnswire_error;
                }
                handle->read_buf  = buf;
                handle->read_size = size;
            } else {
                // already at max size, and full
                return dnswire_error;
            }
            __state(handle, dnswire_writer_reading_accept);
            return dnswire_need_more;

        default:
            break;
        }
        return dnswire_error;

    case dnswire_writer_done:
        return dnswire_error;
    }

    return res;
}

enum dnswire_result dnswire_writer_stop(struct dnswire_writer* handle)
{
    assert(handle);

    enum dnswire_result res = dnswire_encoder_stop(&handle->encoder);

    if (res == dnswire_ok) {
        __state(handle, dnswire_writer_stopping);
    }

    return res;
}

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

#include <assert.h>
#include <stdlib.h>

enum dnswire_result dnswire_reader_init(struct dnswire_reader* handle)
{
    assert(handle);

    if (!handle->buf) {
        if (handle->buf = malloc(handle->size)) {
            return dnswire_ok;
        }
    }

    return dnswire_error;
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

enum dnswire_result dnswire_reader_push(struct dnswire_reader* handle, const uint8_t* data, size_t len)
{
    assert(handle);
    assert(data);
    assert(handle->buf);

    if (handle->left < handle->size) {
        if (handle->at) {
            // move what's left to the start
            if (handle->left) {
                memmove(handle->buf, &handle->buf[handle->at], handle->left);
            }
            handle->at = 0;
        }

        if (len) {
            handle->pushed = handle->size - handle->left < len ? handle->size - handle->left : len;
            memcpy(&handle->buf[handle->left], data, handle->pushed);
            handle->left += handle->pushed;
        }
    } else {
        handle->pushed = 0;
    }

    enum dnswire_result res;

    while (handle->left) {
        res = dnswire_decoder_decode(&handle->decoder, &handle->buf[handle->at], handle->left);

        switch (res) {
        case dnswire_again:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            break;

        case dnswire_need_more:
            if (handle->pushed < len) {
                // we have more

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

                size_t add = len - handle->pushed;
                if (handle->size - handle->left < add) {
                    add = handle->size - handle->left;
                }
                memcpy(&handle->buf[handle->left], &data[handle->pushed], add);
                handle->left += add;
                handle->pushed += add;
                break;
            }
            return res;

        case dnswire_have_dnstap:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
        // fallthrough

        default:
            return res;
        }
    }

    return dnswire_need_more;
}

enum dnswire_result dnswire_reader_read(struct dnswire_reader* handle, int fd)
{
    assert(handle);
    assert(handle->buf);

    enum dnswire_result res;

    if (handle->left) {
        res = dnswire_decoder_decode(&handle->decoder, &handle->buf[handle->at], handle->left);
    } else {
        res = dnswire_need_more;
    }

    switch (res) {
    case dnswire_again:
        handle->at += dnswire_decoder_decoded(handle->decoder);
        handle->left -= dnswire_decoder_decoded(handle->decoder);
        break;

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

        ssize_t nread = read(fd, &handle->buf[handle->at + handle->left], handle->size - handle->at - handle->left);
        if (nread > 0) {
            handle->left += nread;
            break;
        }
        // both error and eof is error, should have control stop before eof
        return dnswire_error;

    case dnswire_have_dnstap:
        handle->at += dnswire_decoder_decoded(handle->decoder);
        handle->left -= dnswire_decoder_decoded(handle->decoder);
    // fallthrough

    default:
        return res;
    }

    while (1) {
        res = dnswire_decoder_decode(&handle->decoder, &handle->buf[handle->at], handle->left);

        switch (res) {
        case dnswire_again:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
            break;

        case dnswire_have_dnstap:
            handle->at += dnswire_decoder_decoded(handle->decoder);
            handle->left -= dnswire_decoder_decoded(handle->decoder);
        // fallthrough

        default:
            return res;
        }
    }

    return dnswire_error;
}

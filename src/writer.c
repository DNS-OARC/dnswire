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

#include "dnswire/writer.h"

#include <assert.h>
#include <stdlib.h>

enum dnswire_result dnswire_writer_init(struct dnswire_writer* handle)
{
    assert(handle);

    if (!handle->buf) {
        if ((handle->buf = malloc(handle->size))) {
            return dnswire_ok;
        }
    }

    return dnswire_error;
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

    if (handle->at > size) {
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

enum dnswire_result dnswire_writer_pop(struct dnswire_writer* handle, uint8_t** data, size_t* len)
{
    assert(handle);
    assert(data);
    assert(len);
    assert(handle->buf);

    enum dnswire_result res;

    if (handle->left) {
        // if we have something left, we don't expand the buffer if more
        // is needed until we have drained it

        if (handle->left < handle->at) {
            // move what we have to start, make more space
            memmove(handle->buf, &handle->buf[handle->at - handle->left], handle->left);
            handle->at = handle->left;
        }

        res = dnswire_encoder_encode(&handle->encoder, &handle->buf[handle->at], handle->size - handle->at);

        switch (res) {
        case dnswire_ok:
        case dnswire_again:
        case dnswire_endofdata:
            handle->at += dnswire_encoder_encoded(handle->encoder);
            handle->left += dnswire_encoder_encoded(handle->encoder);
            break;

        case dnswire_need_more:
            break;

        default:
            return res;
        }

        *data = handle->buf;
        *len  = handle->left;

        return res;
    }

    while (1) {
        res = dnswire_encoder_encode(&handle->encoder, &handle->buf[handle->at], handle->size - handle->at);

        switch (res) {
        case dnswire_ok:
        case dnswire_again:
        case dnswire_endofdata:
            handle->at += dnswire_encoder_encoded(handle->encoder);
            handle->left += dnswire_encoder_encoded(handle->encoder);
            break;

        case dnswire_need_more:
            break;

        default:
            return res;
        }

        if (res == dnswire_need_more) {
            if (handle->size < handle->max) {
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
            // already at max size and it's not enough
            return dnswire_error;
        }
        break;
    }

    *data = &handle->buf[handle->at - handle->left];
    *len  = handle->left;

    return res;
}

enum dnswire_result dnswire_writer_popped(struct dnswire_writer* handle, size_t len)
{
    assert(handle);

    if (len > handle->left) {
        return dnswire_error;
    }

    handle->left -= len;
    if (!handle->left) {
        handle->at = 0;
    }

    return dnswire_ok;
}

enum dnswire_result dnswire_writer_write(struct dnswire_writer* handle, int fd)
{
    assert(handle);
    assert(handle->buf);

    enum dnswire_result res;

    if (handle->left) {
        // if we have something left, we don't expand the buffer if more
        // is needed until we have drained it

        if (handle->left < handle->at) {
            // move what we have to start, make more space
            memmove(handle->buf, &handle->buf[handle->at - handle->left], handle->left);
            handle->at = handle->left;
        }

        res = dnswire_encoder_encode(&handle->encoder, &handle->buf[handle->at], handle->size - handle->at);

        switch (res) {
        case dnswire_ok:
        case dnswire_again:
        case dnswire_endofdata:
            handle->at += dnswire_encoder_encoded(handle->encoder);
            handle->left += dnswire_encoder_encoded(handle->encoder);
            break;

        case dnswire_need_more:
            break;

        default:
            return res;
        }

        ssize_t nwrote = write(fd, handle->buf, handle->left);
        if (nwrote > 0) {
            handle->left -= nwrote;
            if (!handle->left) {
                handle->at = 0;
            }
            return res;
        }
        return dnswire_error;
    }

    while (1) {
        res = dnswire_encoder_encode(&handle->encoder, &handle->buf[handle->at], handle->size - handle->at);

        switch (res) {
        case dnswire_ok:
        case dnswire_again:
        case dnswire_endofdata:
            handle->at += dnswire_encoder_encoded(handle->encoder);
            handle->left += dnswire_encoder_encoded(handle->encoder);
            break;

        case dnswire_need_more:
            break;

        default:
            return res;
        }

        if (res == dnswire_need_more) {
            if (handle->size < handle->max) {
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
            // already at max size and it's not enough
            return dnswire_error;
        }
        break;
    }

    ssize_t nwrote = write(fd, &handle->buf[handle->at - handle->left], handle->left);
    if (nwrote > 0) {
        handle->left -= nwrote;
        if (!handle->left) {
            handle->at = 0;
        }
        return res;
    }
    return dnswire_error;
}

enum dnswire_result dnswire_writer_stop(struct dnswire_writer* handle)
{
    assert(handle);

    return dnswire_encoder_stop(&handle->encoder);
}

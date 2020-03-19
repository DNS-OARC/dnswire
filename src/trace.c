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

#include "dnswire/trace.h"

#if DNSWIRE_TRACE
const char* __printable_string(const uint8_t* data, size_t len)
{
    static char buf[512];
    size_t      r = 0, w = 0;

    while (r < len && w < sizeof(buf) - 1) {
        if (isprint(data[r])) {
            buf[w++] = data[r++];
        } else {
            if (w + 4 >= sizeof(buf) - 1) {
                break;
            }

            sprintf(&buf[w], "\\x%02x", data[r++]);
            w += 4;
        }
    }
    if (w >= sizeof(buf)) {
        buf[sizeof(buf) - 1] = 0;
    } else {
        buf[w] = 0;
    }

    return buf;
}
#endif

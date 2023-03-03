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

#ifndef __dnswire_h_dnswire
#define __dnswire_h_dnswire 1

#define DNSWIRE_DEFAULT_BUF_SIZE (4 * 1024)
#define DNSWIRE_MAXIMUM_BUF_SIZE (64 * 1024)

enum dnswire_result {
    dnswire_ok            = 0,
    dnswire_error         = 1,
    dnswire_again         = 2,
    dnswire_need_more     = 3,
    dnswire_have_dnstap   = 4,
    dnswire_endofdata     = 5,
    dnswire_bidirectional = 6,
};
extern const char* const dnswire_result_string[];

#endif

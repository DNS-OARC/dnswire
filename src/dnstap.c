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

#include "dnswire/dnstap.h"

const char* const DNSTAP_TYPE_STRING[] = {
    "UNKNOWN",
    "MESSAGE",
};
const char* const DNSTAP_MESSAGE_TYPE_STRING[] = {
    "UNKNOWN",
    "AUTH_QUERY",
    "AUTH_RESPONSE",
    "RESOLVER_QUERY",
    "RESOLVER_RESPONSE",
    "CLIENT_QUERY",
    "CLIENT_RESPONSE",
    "FORWARDER_QUERY",
    "FORWARDER_RESPONSE",
    "STUB_QUERY",
    "STUB_RESPONSE",
    "TOOL_QUERY",
    "TOOL_RESPONSE",
};
const char* const DNSTAP_SOCKET_FAMILY_STRING[] = {
    "UNKNOWN",
    "INET",
    "INET6",
};
const char* const DNSTAP_SOCKET_PROTOCOL_STRING[] = {
    "UNKNOWN",
    "UDP",
    "TCP",
};

int dnstap_decode_protobuf(struct dnstap* dnstap, const uint8_t* data, size_t len)
{
    assert(dnstap);
    assert(data);
    assert(!dnstap->unpacked_dnstap);

    if (!(dnstap->unpacked_dnstap = dnstap__dnstap__unpack(NULL, len, data))) {
        return 1;
    }

    dnstap->dnstap = *dnstap->unpacked_dnstap;
    if (dnstap->dnstap.message) {
        dnstap->message = *dnstap->dnstap.message;
    }

    return 0;
}

void dnstap_cleanup(struct dnstap* dnstap)
{
    assert(dnstap);

    if (dnstap->unpacked_dnstap) {
        dnstap__dnstap__free_unpacked(dnstap->unpacked_dnstap, NULL);
        dnstap->unpacked_dnstap = 0;
    }
}

size_t dnstap_encode_protobuf_size(const struct dnstap* dnstap)
{
    assert(dnstap);

    return dnstap__dnstap__get_packed_size(&dnstap->dnstap);
}

size_t dnstap_encode_protobuf(const struct dnstap* dnstap, uint8_t* data)
{
    assert(dnstap);
    assert(data);

    return dnstap__dnstap__pack(&dnstap->dnstap, data);
}

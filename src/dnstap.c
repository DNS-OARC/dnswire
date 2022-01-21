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
    "UPDATE_QUERY",
    "UPDATE_RESPONSE",
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
    "DOT",
    "DOH",
    "DNSCryptUDP",
    "DNSCryptTCP",
};
const char* const DNSTAP_POLICY_ACTION_STRING[] = {
    "UNKNOWN",
    "NXDOMAIN",
    "NODATA",
    "PASS",
    "DROP",
    "TRUNCATE",
    "LOCAL_DATA",
};
const char* const DNSTAP_POLICY_MATCH_STRING[] = {
    "UNKNOWN",
    "QNAME",
    "CLIENT_IP",
    "RESPONSE_IP",
    "NS_NAME",
    "NS_IP",
};

void dnstap_message_clear_policy(struct dnstap* dnstap)
{
    static const Dnstap__Policy policy = DNSTAP__POLICY__INIT;
    assert(dnstap);

    dnstap->message.policy = 0;
    dnstap->policy         = policy;
}

int dnstap_decode_protobuf(struct dnstap* dnstap, const uint8_t* data, size_t len)
{
    assert(dnstap);
    assert(data);
    assert(!dnstap->unpacked_dnstap);

    if (!(dnstap->unpacked_dnstap = dnstap__dnstap__unpack(NULL, len, data))) {
        return 1;
    }

    dnstap->dnstap = *dnstap->unpacked_dnstap;

    switch (dnstap->dnstap.type) {
    case DNSTAP_TYPE_MESSAGE:
        break;
    default:
        dnstap->dnstap.type = (enum _Dnstap__Dnstap__Type)DNSTAP_TYPE_UNKNOWN;
    }

    if (dnstap->dnstap.message) {
        dnstap->message = *dnstap->dnstap.message;

        switch (dnstap->message.type) {
        case DNSTAP_MESSAGE_TYPE_AUTH_QUERY:
        case DNSTAP_MESSAGE_TYPE_AUTH_RESPONSE:
        case DNSTAP_MESSAGE_TYPE_RESOLVER_QUERY:
        case DNSTAP_MESSAGE_TYPE_RESOLVER_RESPONSE:
        case DNSTAP_MESSAGE_TYPE_CLIENT_QUERY:
        case DNSTAP_MESSAGE_TYPE_CLIENT_RESPONSE:
        case DNSTAP_MESSAGE_TYPE_FORWARDER_QUERY:
        case DNSTAP_MESSAGE_TYPE_FORWARDER_RESPONSE:
        case DNSTAP_MESSAGE_TYPE_STUB_QUERY:
        case DNSTAP_MESSAGE_TYPE_STUB_RESPONSE:
        case DNSTAP_MESSAGE_TYPE_TOOL_QUERY:
        case DNSTAP_MESSAGE_TYPE_TOOL_RESPONSE:
        case DNSTAP_MESSAGE_TYPE_UPDATE_QUERY:
        case DNSTAP_MESSAGE_TYPE_UPDATE_RESPONSE:
            break;
        default:
            dnstap->message.type = (enum _Dnstap__Message__Type)DNSTAP_MESSAGE_TYPE_UNKNOWN;
        }

        switch (dnstap->message.socket_family) {
        case DNSTAP_SOCKET_FAMILY_INET:
        case DNSTAP_SOCKET_FAMILY_INET6:
            break;
        default:
            dnstap->message.has_socket_family = false;
            dnstap->message.socket_family     = (enum _Dnstap__SocketFamily)DNSTAP_SOCKET_FAMILY_UNKNOWN;
        }

        switch (dnstap->message.socket_protocol) {
        case DNSTAP_SOCKET_PROTOCOL_UDP:
        case DNSTAP_SOCKET_PROTOCOL_TCP:
        case DNSTAP_SOCKET_PROTOCOL_DOT:
        case DNSTAP_SOCKET_PROTOCOL_DOH:
        case DNSTAP_SOCKET_PROTOCOL_DNSCryptUDP:
        case DNSTAP_SOCKET_PROTOCOL_DNSCryptTCP:
            break;
        default:
            dnstap->message.has_socket_protocol = false;
            dnstap->message.socket_protocol     = (enum _Dnstap__SocketProtocol)DNSTAP_SOCKET_PROTOCOL_UNKNOWN;
        }

        if (dnstap->message.policy) {
            dnstap->policy = *dnstap->message.policy;

            switch (dnstap->policy.action) {
            case DNSTAP_POLICY_ACTION_NXDOMAIN:
            case DNSTAP_POLICY_ACTION_NODATA:
            case DNSTAP_POLICY_ACTION_PASS:
            case DNSTAP_POLICY_ACTION_DROP:
            case DNSTAP_POLICY_ACTION_TRUNCATE:
            case DNSTAP_POLICY_ACTION_LOCAL_DATA:
                break;
            default:
                dnstap->policy.has_action = false;
                dnstap->policy.action     = (enum _Dnstap__Policy__Action)DNSTAP_POLICY_ACTION_UNKNOWN;
            }

            switch (dnstap->policy.match) {
            case DNSTAP_POLICY_MATCH_QNAME:
            case DNSTAP_POLICY_MATCH_CLIENT_IP:
            case DNSTAP_POLICY_MATCH_RESPONSE_IP:
            case DNSTAP_POLICY_MATCH_NS_NAME:
            case DNSTAP_POLICY_MATCH_NS_IP:
                break;
            default:
                dnstap->policy.has_match = false;
                dnstap->policy.match     = (enum _Dnstap__Policy__Match)DNSTAP_POLICY_MATCH_UNKNOWN;
            }
        }
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

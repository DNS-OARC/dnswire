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

#include <dnswire/dnstap.pb-c.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifndef __dnswire_h_dnstap
#define __dnswire_h_dnstap 1

#define DNSTAP_PROTOBUF_CONTENT_TYPE "protobuf:dnstap.Dnstap"
#define DNSTAP_PROTOBUF_CONTENT_TYPE_LENGTH 22

enum dnstap_type {
    DNSTAP_TYPE_UNKNOWN = 0,
    DNSTAP_TYPE_MESSAGE = 1,
};
extern const char* const DNSTAP_TYPE_STRING[];

enum dnstap_message_type {
    DNSTAP_MESSAGE_TYPE_UNKNOWN            = 0,
    DNSTAP_MESSAGE_TYPE_AUTH_QUERY         = 1,
    DNSTAP_MESSAGE_TYPE_AUTH_RESPONSE      = 2,
    DNSTAP_MESSAGE_TYPE_RESOLVER_QUERY     = 3,
    DNSTAP_MESSAGE_TYPE_RESOLVER_RESPONSE  = 4,
    DNSTAP_MESSAGE_TYPE_CLIENT_QUERY       = 5,
    DNSTAP_MESSAGE_TYPE_CLIENT_RESPONSE    = 6,
    DNSTAP_MESSAGE_TYPE_FORWARDER_QUERY    = 7,
    DNSTAP_MESSAGE_TYPE_FORWARDER_RESPONSE = 8,
    DNSTAP_MESSAGE_TYPE_STUB_QUERY         = 9,
    DNSTAP_MESSAGE_TYPE_STUB_RESPONSE      = 10,
    DNSTAP_MESSAGE_TYPE_TOOL_QUERY         = 11,
    DNSTAP_MESSAGE_TYPE_TOOL_RESPONSE      = 12,
    DNSTAP_MESSAGE_TYPE_UPDATE_QUERY       = 13,
    DNSTAP_MESSAGE_TYPE_UPDATE_RESPONSE    = 14,
};
extern const char* const DNSTAP_MESSAGE_TYPE_STRING[];

enum dnstap_socket_family {
    DNSTAP_SOCKET_FAMILY_UNKNOWN = 0,
    DNSTAP_SOCKET_FAMILY_INET    = 1,
    DNSTAP_SOCKET_FAMILY_INET6   = 2,
};
extern const char* const DNSTAP_SOCKET_FAMILY_STRING[];

enum dnstap_socket_protocol {
    DNSTAP_SOCKET_PROTOCOL_UNKNOWN     = 0,
    DNSTAP_SOCKET_PROTOCOL_UDP         = 1,
    DNSTAP_SOCKET_PROTOCOL_TCP         = 2,
    DNSTAP_SOCKET_PROTOCOL_DOT         = 3,
    DNSTAP_SOCKET_PROTOCOL_DOH         = 4,
    DNSTAP_SOCKET_PROTOCOL_DNSCryptUDP = 5,
    DNSTAP_SOCKET_PROTOCOL_DNSCryptTCP = 6,
    DNSTAP_SOCKET_PROTOCOL_DOQ         = 7,
};
extern const char* const DNSTAP_SOCKET_PROTOCOL_STRING[];

enum dnstap_policy_action {
    DNSTAP_POLICY_ACTION_UNKNOWN    = 0,
    DNSTAP_POLICY_ACTION_NXDOMAIN   = 1,
    DNSTAP_POLICY_ACTION_NODATA     = 2,
    DNSTAP_POLICY_ACTION_PASS       = 3,
    DNSTAP_POLICY_ACTION_DROP       = 4,
    DNSTAP_POLICY_ACTION_TRUNCATE   = 5,
    DNSTAP_POLICY_ACTION_LOCAL_DATA = 6,
};
extern const char* const DNSTAP_POLICY_ACTION_STRING[];

enum dnstap_policy_match {
    DNSTAP_POLICY_MATCH_UNKNOWN     = 0,
    DNSTAP_POLICY_MATCH_QNAME       = 1,
    DNSTAP_POLICY_MATCH_CLIENT_IP   = 2,
    DNSTAP_POLICY_MATCH_RESPONSE_IP = 3,
    DNSTAP_POLICY_MATCH_NS_NAME     = 4,
    DNSTAP_POLICY_MATCH_NS_IP       = 5,
};
extern const char* const DNSTAP_POLICY_MATCH_STRING[];

struct dnstap {
    Dnstap__Dnstap  dnstap;
    Dnstap__Message message;
    Dnstap__Policy  policy;
    bool            _policy_type_alloced;

    Dnstap__Dnstap* unpacked_dnstap;
};

#define DNSTAP_INITIALIZER                        \
    {                                             \
        .dnstap          = DNSTAP__DNSTAP__INIT,  \
        .message         = DNSTAP__MESSAGE__INIT, \
        .policy          = DNSTAP__POLICY__INIT,  \
        .unpacked_dnstap = 0,                     \
    }

#include <dnswire/dnstap-macros.h>
#define dnstap_type(d) (enum dnstap_type)((d).dnstap.type)
#define dnstap_set_type(d, v)                                                 \
    switch (v) {                                                              \
    case DNSTAP_TYPE_MESSAGE:                                                 \
        (d).dnstap.type    = (enum _Dnstap__Dnstap__Type)v;                   \
        (d).dnstap.message = &(d).message;                                    \
        break;                                                                \
    default:                                                                  \
        (d).dnstap.type    = (enum _Dnstap__Dnstap__Type)DNSTAP_TYPE_UNKNOWN; \
        (d).dnstap.message = 0;                                               \
    }
#define dnstap_has_message(d) ((d).dnstap.message != 0)

#define dnstap_message_type(d) (enum dnstap_message_type)((d).message.type)
#define dnstap_message_set_type(d, v)                                                \
    switch (v) {                                                                     \
    case DNSTAP_MESSAGE_TYPE_AUTH_QUERY:                                             \
    case DNSTAP_MESSAGE_TYPE_AUTH_RESPONSE:                                          \
    case DNSTAP_MESSAGE_TYPE_RESOLVER_QUERY:                                         \
    case DNSTAP_MESSAGE_TYPE_RESOLVER_RESPONSE:                                      \
    case DNSTAP_MESSAGE_TYPE_CLIENT_QUERY:                                           \
    case DNSTAP_MESSAGE_TYPE_CLIENT_RESPONSE:                                        \
    case DNSTAP_MESSAGE_TYPE_FORWARDER_QUERY:                                        \
    case DNSTAP_MESSAGE_TYPE_FORWARDER_RESPONSE:                                     \
    case DNSTAP_MESSAGE_TYPE_STUB_QUERY:                                             \
    case DNSTAP_MESSAGE_TYPE_STUB_RESPONSE:                                          \
    case DNSTAP_MESSAGE_TYPE_TOOL_QUERY:                                             \
    case DNSTAP_MESSAGE_TYPE_TOOL_RESPONSE:                                          \
        (d).message.type = (enum _Dnstap__Message__Type)v;                           \
        break;                                                                       \
    default:                                                                         \
        (d).message.type = (enum _Dnstap__Message__Type)DNSTAP_MESSAGE_TYPE_UNKNOWN; \
    }
#define dnstap_message_set_socket_family(d, v)                                                   \
    switch (v) {                                                                                 \
    case DNSTAP_SOCKET_FAMILY_INET:                                                              \
    case DNSTAP_SOCKET_FAMILY_INET6:                                                             \
        (d).message.has_socket_family = true;                                                    \
        (d).message.socket_family     = (enum _Dnstap__SocketFamily)v;                           \
        break;                                                                                   \
    default:                                                                                     \
        (d).message.has_socket_family = false;                                                   \
        (d).message.socket_family     = (enum _Dnstap__SocketFamily)DNSTAP_MESSAGE_TYPE_UNKNOWN; \
    }
#define dnstap_message_set_socket_protocol(d, v)                                                     \
    switch (v) {                                                                                     \
    case DNSTAP_SOCKET_PROTOCOL_UDP:                                                                 \
    case DNSTAP_SOCKET_PROTOCOL_TCP:                                                                 \
        (d).message.has_socket_protocol = true;                                                      \
        (d).message.socket_protocol     = (enum _Dnstap__SocketProtocol)v;                           \
        break;                                                                                       \
    default:                                                                                         \
        (d).message.has_socket_protocol = false;                                                     \
        (d).message.socket_protocol     = (enum _Dnstap__SocketProtocol)DNSTAP_MESSAGE_TYPE_UNKNOWN; \
    }

#define dnstap_message_has_policy(d) ((d).dnstap.message->policy != 0)
#define dnstap_message_use_policy(d) (d).dnstap.message->policy = &(d).policy
void dnstap_message_clear_policy(struct dnstap*);
#define dnstap_message_policy_set_action(d, v)                                              \
    switch (v) {                                                                            \
    case DNSTAP_POLICY_ACTION_NXDOMAIN:                                                     \
    case DNSTAP_POLICY_ACTION_NODATA:                                                       \
    case DNSTAP_POLICY_ACTION_PASS:                                                         \
    case DNSTAP_POLICY_ACTION_DROP:                                                         \
    case DNSTAP_POLICY_ACTION_TRUNCATE:                                                     \
    case DNSTAP_POLICY_ACTION_LOCAL_DATA:                                                   \
        (d).policy.has_action = true;                                                       \
        (d).policy.action     = (enum _Dnstap__Policy__Action)v;                            \
        break;                                                                              \
    default:                                                                                \
        (d).policy.has_action = false;                                                      \
        (d).policy.action     = (enum _Dnstap__Policy__Action)DNSTAP_POLICY_ACTION_UNKNOWN; \
    }
#define dnstap_message_policy_set_match(d, v)                                            \
    switch (v) {                                                                         \
    case DNSTAP_POLICY_MATCH_QNAME:                                                      \
    case DNSTAP_POLICY_MATCH_CLIENT_IP:                                                  \
    case DNSTAP_POLICY_MATCH_RESPONSE_IP:                                                \
    case DNSTAP_POLICY_MATCH_NS_NAME:                                                    \
    case DNSTAP_POLICY_MATCH_NS_IP:                                                      \
        (d).policy.has_match = true;                                                     \
        (d).policy.match     = (enum _Dnstap__Policy__Match)v;                           \
        break;                                                                           \
    default:                                                                             \
        (d).policy.has_match = false;                                                    \
        (d).policy.match     = (enum _Dnstap__Policy__Match)DNSTAP_POLICY_MATCH_UNKNOWN; \
    }

int dnstap_decode_protobuf(struct dnstap*, const uint8_t*, size_t);
// int dnstap_decode_cbor(struct dnstap*, const uint8_t*, size_t);

size_t dnstap_encode_protobuf_size(const struct dnstap*);
size_t dnstap_encode_protobuf(const struct dnstap*, uint8_t*);

void dnstap_cleanup(struct dnstap*);

#endif

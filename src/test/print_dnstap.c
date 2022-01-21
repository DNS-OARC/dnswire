#include <dnswire/dnstap.h>

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

static const char* printable_string(const uint8_t* data, size_t len)
{
    static char buf[512], hex;
    size_t      r = 0, w = 0;

    while (r < len && w < sizeof(buf) - 1) {
        if (isprint(data[r])) {
            buf[w++] = data[r++];
        } else {
            if (w + 4 >= sizeof(buf) - 1) {
                break;
            }

            buf[w++] = '\\';
            buf[w++] = 'x';
            hex      = (data[r] & 0xf0) >> 4;
            if (hex > 9) {
                buf[w++] = 'a' + (hex - 10);
            } else {
                buf[w++] = '0' + hex;
            }
            hex = data[r++] & 0xf;
            if (hex > 9) {
                buf[w++] = 'a' + (hex - 10);
            } else {
                buf[w++] = '0' + hex;
            }
        }
    }
    if (w >= sizeof(buf)) {
        buf[sizeof(buf) - 1] = 0;
    } else {
        buf[w] = 0;
    }

    return buf;
}

static const char* printable_ip_address(const uint8_t* data, size_t len)
{
    static char buf[INET6_ADDRSTRLEN];

    buf[0] = 0;
    if (len == 4) {
        inet_ntop(AF_INET, data, buf, sizeof(buf));
    } else if (len == 16) {
        inet_ntop(AF_INET6, data, buf, sizeof(buf));
    }

    return buf;
}

static void print_dnstap(const struct dnstap* d)
{
    printf("---- dnstap\n");
    if (dnstap_has_identity(*d)) {
        printf("identity: %s\n", printable_string(dnstap_identity(*d), dnstap_identity_length(*d)));
    }
    if (dnstap_has_version(*d)) {
        printf("version: %s\n", printable_string(dnstap_version(*d), dnstap_version_length(*d)));
    }
    if (dnstap_has_extra(*d)) {
        printf("extra: %s\n", printable_string(dnstap_extra(*d), dnstap_extra_length(*d)));
    }

    if (dnstap_type(*d) == DNSTAP_TYPE_MESSAGE && dnstap_has_message(*d)) {
        printf("message:\n  type: %s\n", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*d)]);

        if (dnstap_message_has_query_time_sec(*d) && dnstap_message_has_query_time_nsec(*d)) {
            printf("  query_time: %" PRIu64 ".%" PRIu32 "\n", dnstap_message_query_time_sec(*d), dnstap_message_query_time_nsec(*d));
        }
        if (dnstap_message_has_response_time_sec(*d) && dnstap_message_has_response_time_nsec(*d)) {
            printf("  response_time: %" PRIu64 ".%" PRIu32 "\n", dnstap_message_response_time_sec(*d), dnstap_message_response_time_nsec(*d));
        }
        if (dnstap_message_has_socket_family(*d)) {
            printf("  socket_family: %s\n", DNSTAP_SOCKET_FAMILY_STRING[dnstap_message_socket_family(*d)]);
        }
        if (dnstap_message_has_socket_protocol(*d)) {
            printf("  socket_protocol: %s\n", DNSTAP_SOCKET_PROTOCOL_STRING[dnstap_message_socket_protocol(*d)]);
        }
        if (dnstap_message_has_query_address(*d)) {
            printf("  query_address: %s\n", printable_ip_address(dnstap_message_query_address(*d), dnstap_message_query_address_length(*d)));
        }
        if (dnstap_message_has_query_port(*d)) {
            printf("  query_port: %u\n", dnstap_message_query_port(*d));
        }
        if (dnstap_message_has_response_address(*d)) {
            printf("  response_address: %s\n", printable_ip_address(dnstap_message_response_address(*d), dnstap_message_response_address_length(*d)));
        }
        if (dnstap_message_has_response_port(*d)) {
            printf("  response_port: %u\n", dnstap_message_response_port(*d));
        }
        if (dnstap_message_has_query_zone(*d)) {
            printf("  query_zone: %s\n", printable_string(dnstap_message_query_zone(*d), dnstap_message_query_zone_length(*d)));
        }
        if (dnstap_message_has_query_message(*d)) {
            printf("  query_message_length: %zu\n", dnstap_message_query_message_length(*d));
            printf("  query_message: %s\n", printable_string(dnstap_message_query_message(*d), dnstap_message_query_message_length(*d)));
        }
        if (dnstap_message_has_response_message(*d)) {
            printf("  response_message_length: %zu\n", dnstap_message_response_message_length(*d));
            printf("  response_message: %s\n", printable_string(dnstap_message_response_message(*d), dnstap_message_response_message_length(*d)));
        }

        if (dnstap_message_has_policy(*d)) {
            printf("  policy:\n");

            if (dnstap_message_policy_has_type(*d)) {
                printf("    type: %s\n", dnstap_message_policy_type(*d));
            }
            if (dnstap_message_policy_has_rule(*d)) {
                printf("    rule: %s\n", printable_string(dnstap_message_policy_rule(*d), dnstap_message_policy_rule_length(*d)));
            }
            if (dnstap_message_policy_has_action(*d)) {
                printf("    action: %s\n", DNSTAP_POLICY_ACTION_STRING[dnstap_message_policy_action(*d)]);
            }
            if (dnstap_message_policy_has_match(*d)) {
                printf("    match: %s\n", DNSTAP_POLICY_MATCH_STRING[dnstap_message_policy_match(*d)]);
            }
            if (dnstap_message_policy_has_value(*d)) {
                printf("    value: %s\n", printable_string(dnstap_message_policy_value(*d), dnstap_message_policy_value_length(*d)));
            }
        }
    }

    printf("----\n");
}

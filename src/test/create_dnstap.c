#include <dnswire/version.h>
#include <dnswire/dnstap.h>

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

static char          dns_wire_format_placeholder[] = "dns_wire_format_placeholder";
static unsigned char query_address[sizeof(struct in_addr)];
static unsigned char response_address[sizeof(struct in_addr)];
static char          policy_value[] = "bad.ns.name";

static inline void create_dnstap(struct dnstap* d, const char* identity)
{
    dnstap_set_identity_string(*d, identity);
    dnstap_set_version_string(*d, DNSWIRE_VERSION_STRING);
    dnstap_set_type(*d, DNSTAP_TYPE_MESSAGE);
    dnstap_message_set_type(*d, DNSTAP_MESSAGE_TYPE_TOOL_QUERY);
    dnstap_message_set_socket_family(*d, DNSTAP_SOCKET_FAMILY_INET);
    dnstap_message_set_socket_protocol(*d, DNSTAP_SOCKET_PROTOCOL_UDP);

    if (inet_pton(AF_INET, "127.0.0.1", query_address) != 1) {
        fprintf(stderr, "inet_pton(127.0.0.1) failed: %s\n", strerror(errno));
    } else {
        dnstap_message_set_query_address(*d, query_address, sizeof(query_address));
    }
    dnstap_message_set_query_port(*d, 12345);

    struct timespec query_time = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &query_time);
    dnstap_message_set_query_time_sec(*d, query_time.tv_sec);
    dnstap_message_set_query_time_nsec(*d, query_time.tv_nsec);

    if (inet_pton(AF_INET, "127.0.0.1", response_address) != 1) {
        fprintf(stderr, "inet_pton(127.0.0.1) failed: %s\n", strerror(errno));
    } else {
        dnstap_message_set_response_address(*d, response_address, sizeof(response_address));
    }
    dnstap_message_set_response_port(*d, 53);

    struct timespec response_time = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &response_time);
    dnstap_message_set_response_time_sec(*d, response_time.tv_sec);
    dnstap_message_set_response_time_nsec(*d, response_time.tv_nsec);

    dnstap_message_set_query_message(*d, dns_wire_format_placeholder, sizeof(dns_wire_format_placeholder) - 1);
    dnstap_message_set_response_message(*d, dns_wire_format_placeholder, sizeof(dns_wire_format_placeholder) - 1);

    dnstap_message_use_policy(*d);
    dnstap_message_policy_set_type(*d, "RPZ");
    dnstap_message_policy_set_action(*d, DNSTAP_POLICY_ACTION_DROP);
    dnstap_message_policy_set_match(*d, DNSTAP_POLICY_MATCH_NS_NAME);
    dnstap_message_policy_set_value(*d, policy_value, sizeof(policy_value) - 1);
}

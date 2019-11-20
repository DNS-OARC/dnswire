#include <dnswire/dnswire.h>
#include <dnswire/dnstap.h>

#include <arpa/inet.h>
#include <time.h>

static char dns_wire_format_placeholder[] = "dns_wire_format_placeholder";

static inline struct dnstap create_dnstap(const char* identity)
{
    /*
     * Now we initialize a DNSTAP message.
     */

    struct dnstap d = DNSTAP_INITIALIZER;

    /*
     * DNSTAP has a header with information about who constructed the
     * message.
     */

    dnstap_set_identity_string(d, identity);
    dnstap_set_version_string(d, DNSWIRE_VERSION_STRING);

    /*
     * Now we specify that this is a DNSTAP message, this is the one that
     * holds the DNS.
     */

    dnstap_set_type(d, DNSTAP_TYPE_MESSAGE);

    /*
     * The message can have different types, we specify this is a query
     * made by a tool.
     */

    dnstap_message_set_type(d, DNSTAP_MESSAGE_TYPE_TOOL_QUERY);

    /*
     * As most DNS comes over the network we can specify over what protocol,
     * where from, to whom it came to and when it happened.
     *
     * Even if all fields are optional and there is no restriction on how
     * many or how few you set, there is a recommended way of filling in
     * the messages based on the message type.
     *
     * Please see the description of this at the bottom of
     * `src/dnstap.pb/dnstap.proto` or
     * <https://github.com/dnstap/dnstap.pb/blob/master/dnstap.proto>.
     */

    dnstap_message_set_socket_family(d, DNSTAP_SOCKET_FAMILY_INET);
    dnstap_message_set_socket_protocol(d, DNSTAP_SOCKET_PROTOCOL_UDP);

    unsigned char query_address[sizeof(struct in_addr)];
    if (inet_pton(AF_INET, "127.0.0.1", query_address) != 1) {
        fprintf(stderr, "inet_pton(127.0.0.1) failed: %s\n", strerror(errno));
    } else {
        dnstap_message_set_query_address(d, query_address, sizeof(query_address));
    }
    dnstap_message_set_query_port(d, 12345);

    struct timespec query_time = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &query_time);
    dnstap_message_set_query_time_sec(d, query_time.tv_sec);
    dnstap_message_set_query_time_nsec(d, query_time.tv_nsec);

    unsigned char response_address[sizeof(struct in_addr)];
    if (inet_pton(AF_INET, "127.0.0.1", response_address) != 1) {
        fprintf(stderr, "inet_pton(127.0.0.1) failed: %s\n", strerror(errno));
    } else {
        dnstap_message_set_response_address(d, response_address, sizeof(response_address));
    }
    dnstap_message_set_response_port(d, 53);

    struct timespec response_time = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &response_time);
    dnstap_message_set_response_time_sec(d, response_time.tv_sec);
    dnstap_message_set_response_time_nsec(d, response_time.tv_nsec);

    /*
     * If we also had the DNS wire format we could use it, now we fill it
     * with a placeholder text.
     *
     * NOTE: This will be invalid DNS if the output file is used with a
     *       tool that uses the DNS messages.
     */

    dnstap_message_set_query_message(d, dns_wire_format_placeholder, sizeof(dns_wire_format_placeholder) - 1);

    dnstap_message_set_response_message(d, dns_wire_format_placeholder, sizeof(dns_wire_format_placeholder) - 1);

    return d;
}

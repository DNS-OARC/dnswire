#include <dnswire/dnswire.h>
#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static char dns_wire_format_placeholder[] = "dns_wire_format_placeholder";
static char content_type[]                = "protobuf:dnstap.Dnstap";

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: simple_writer <file>\n");
        return 1;
    }

    /*
     * We start by opening the output file for writing.
     */

    FILE* fp = fopen(argv[1], "w");
    if (!fp) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    /*
     * Now we initialize a DNSTAP message.
     */

    struct dnstap d = DNSTAP_INITIALIZER;

    /*
     * DNSTAP has a header with information about who constructed the
     * message.
     */

    dnstap_set_identity_string(d, "simple_writer");
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
        return 1;
    }
    dnstap_message_set_query_address(d, query_address, sizeof(query_address));
    dnstap_message_set_query_port(d, 12345);

    struct timespec query_time = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &query_time);
    dnstap_message_set_query_time_sec(d, query_time.tv_sec);
    dnstap_message_set_query_time_nsec(d, query_time.tv_nsec);

    unsigned char response_address[sizeof(struct in_addr)];
    if (inet_pton(AF_INET, "127.0.0.1", response_address) != 1) {
        fprintf(stderr, "inet_pton(127.0.0.1) failed: %s\n", strerror(errno));
        return 1;
    }
    dnstap_message_set_response_address(d, response_address, sizeof(response_address));
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

    /*
     * Now that the message is prepared we can begin encapsulating it in
     * protobuf and Frame Streams.
     *
     * First we ask what the encoded size of the protobuf message would be
     * to allocate a buffer for it, then we encode it.
     */

    size_t  len = dnstap_encode_protobuf_size(&d);
    uint8_t frame[len];
    dnstap_encode_protobuf(&d, frame);

    /*
     * Next we initialize a tinyframe writer to write out the Frame Streams.
     */

    struct tinyframe_writer h = TINYFRAME_WRITER_INITIALIZER;

    /*
     * We create a buffer that holds the control frames and the data frame,
     * tinyframe can indicate when it needs more buffer space but we skip
     * that here to simplify the example.
     */

    uint8_t out[len + (TINYFRAME_CONTROL_FRAME_LENGTH_MAX * 3)];
    len          = sizeof(out);
    size_t wrote = 0;

    /*
     * First we write, to the buffer, a control start with a content type
     * control field for the DNSTAP protobuf content type.
     */

    if (tinyframe_write_control_start(&h, &out[wrote], len, content_type, sizeof(content_type) - 1) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_control_start() failed\n");
        return 1;
    }
    wrote += h.bytes_wrote;
    len -= h.bytes_wrote;

    /*
     * The we write, to the buffer, a data frame with the encoded DNSTAP
     * message.
     */

    if (tinyframe_write_frame(&h, &out[wrote], len, frame, sizeof(frame)) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_frame() failed\n");
        return 1;
    }
    wrote += h.bytes_wrote;
    len -= h.bytes_wrote;

    /*
     * Lastly we write, to the buffer, a control stop to indicate the end.
     */

    if (tinyframe_write_control_stop(&h, &out[wrote], len) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_control_stop() failed\n");
        return 1;
    }
    wrote += h.bytes_wrote;
    len -= h.bytes_wrote;

    /*
     * Now we write the buffer to the file we opened previously.
     */

    if (fwrite(out, 1, wrote, fp) != wrote) {
        fprintf(stderr, "fwrite() failed\n");
        return 1;
    }
    printf("wrote %zu bytes\n", wrote);

    fclose(fp);
    return 0;
}

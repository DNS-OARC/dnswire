#include <dnswire/dnswire.h>
#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char dns_wire_format_placeholder[] = "dns_wire_format_placeholder";
static char content_type[]                = "protobuf:dnstap.Dnstap";

int send_frame(int fd, const char* buf, size_t len)
{
    size_t  sent = 0;
    ssize_t w;

    while (sent < len) {
        if ((w = send(fd, &buf[sent], len - sent, 0)) < 1) {
            if (w < 0) {
                fprintf(stderr, "write() failed: %s\n", strerror(errno));
            }
            return 1;
        }

        printf("sent %zd bytes\n", w);

        sent += w;
    }

    return 0;
}

void handle_client(int fd)
{
    /*
     * We now create each of the needed frames and sends them one by one.
     */

    struct tinyframe_writer writer = TINYFRAME_WRITER_INITIALIZER;

    /*
     * We create a buffer that holds a control frame, we do not need to
     * hold the data in this buffer because we will use the data buffer
     * for that.
     */

    uint8_t out[TINYFRAME_CONTROL_FRAME_LENGTH_MAX];
    size_t  len = sizeof(out);

    /*
     * First we write, to the buffer, a control start with a content type
     * control field for the DNSTAP protobuf content type.
     *
     * Then we send it.
     */

    if (tinyframe_write_control_start(&writer, out, len, content_type, sizeof(content_type) - 1) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_control_start() failed\n");
        return;
    }
    printf("sending control start and content type\n");
    if (send_frame(fd, out, writer.bytes_wrote)) {
        return;
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
        return;
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
        return;
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
     * and then we allocate a buffer with of that size plus the size of
     * a Frame Streams frame header.
     *
     * Then we encode the DNSTAP message and put it after the frame header
     * and call `tinyframe_set_header()` to set the header.
     */

    size_t  frame_len = dnstap_encode_protobuf_size(&d);
    uint8_t frame[TINYFRAME_HEADER_SIZE + frame_len];
    dnstap_encode_protobuf(&d, &frame[TINYFRAME_HEADER_SIZE]);
    tinyframe_set_header(frame, frame_len);

    /*
     * Then we write, to the buffer, the data frame with the encoded DNSTAP
     * message and the included Frame Streams header.
     *
     * Then we send it.
     */

    printf("sending DNSTAP\n");
    if (send_frame(fd, frame, sizeof(frame))) {
        return;
    }

    /*
     * Lastly we write, to the buffer, a control stop to indicate the end.
     *
     * Then we send it.
     */

    if (tinyframe_write_control_stop(&writer, out, len) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_control_stop() failed\n");
        return;
    }
    printf("sending control stop\n");
    if (send_frame(fd, out, writer.bytes_wrote)) {
        return;
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: simple_receiver <IP> <port>\n");
        return 1;
    }

    /*
     * We setup and connect to the IP and port given on command line.
     */

    struct sockaddr_storage addr_store;
    struct sockaddr_in*     addr = (struct sockaddr_in*)&addr_store;
    socklen_t               addrlen;

    if (strchr(argv[1], ':')) {
        addr->sin_family = AF_INET6;
        addrlen          = sizeof(struct sockaddr_in6);
    } else {
        addr->sin_family = AF_INET;
        addrlen          = sizeof(struct sockaddr_in);
    }

    if (inet_pton(addr->sin_family, argv[1], &addr->sin_addr) != 1) {
        fprintf(stderr, "inet_pton(%s) failed: %s\n", argv[1], strerror(errno));
        return 1;
    }

    addr->sin_port = atoi(argv[2]);

    int sockfd = socket(addr->sin_family, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        fprintf(stderr, "socket() failed: %s\n", strerror(errno));
        return 1;
    }
    printf("socket\n");

    if (connect(sockfd, (struct sockaddr*)addr, addrlen)) {
        fprintf(stderr, "connect() failed: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }
    printf("connect\n");

    /*
     * We are now connected!
     */

    handle_client(sockfd);

    /*
     * Time to exit, let's shutdown and close the sockets.
     */

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    return 0;
}

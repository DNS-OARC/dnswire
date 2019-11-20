#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "create_dnstap.c"

static char content_type[] = "protobuf:dnstap.Dnstap";

int send_frame(int fd, const uint8_t* buf, size_t len)
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
     * Now we create a DNSTAP message.
     */

    struct dnstap d = create_dnstap();

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

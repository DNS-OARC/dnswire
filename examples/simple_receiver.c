#include <tinyframe/tinyframe.h>

#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "print_dnstap.c"

void handle_client(int fd)
{
    /*
     * As we will be receiving we need to support getting half a frame
     * and continuing on that so we do a loop where we call
     * `tinyframe_read()` and when it says it needs more bytes we do
     * a `recv()` on the socket.
     *
     * As we call `tinyframe_read()` the first time without any data
     * (`have` == 0) it will return that it needs more.
     */

    struct tinyframe_reader reader = TINYFRAME_READER_INITIALIZER;

    ssize_t               r;
    size_t                have = 0;
    uint8_t               buf[4096];
    enum tinyframe_result res;

    while (1) {
        switch ((res = tinyframe_read(&reader, buf, have))) {
        case tinyframe_have_control:
            /*
             * If we get a control frame we check that it's a start one,
             * this code can be extended with checks that you only get one
             * start frame and it's the first you get.
             */
            if (reader.control.type != TINYFRAME_CONTROL_START) {
                fprintf(stderr, "Not a control type start\n");
                return;
            }
            printf("got control start\n");
            break;

        case tinyframe_have_control_field:
            /*
             * We have a control field, so lets check that it's a
             * `content type` and the content we want is
             * `protobuf:dnstap.Dnstap`.
             */
            if (reader.control_field.type != TINYFRAME_CONTROL_FIELD_CONTENT_TYPE
                || strncmp("protobuf:dnstap.Dnstap", (const char*)reader.control_field.data, reader.control_field.length)) {
                fprintf(stderr, "Not content type dnstap\n");
                return;
            }
            printf("got content type DNSTAP\n");
            break;

        case tinyframe_have_frame: {
            /*
             * We got a frame, lets decode and print the DNSTAP content.
             */

            struct dnstap d = DNSTAP_INITIALIZER;

            /*
             * First we decode the DNSTAP protobuf message.
             */

            if (dnstap_decode_protobuf(&d, reader.frame.data, reader.frame.length)) {
                fprintf(stderr, "dnstap_decode_protobuf() failed\n");
                return;
            }

            /*
             * Now we print the DNSTAP message.
             */

            print_dnstap(&d);

            /*
             * And finally we cleanup protobuf allocations.
             */

            dnstap_cleanup(&d);
            break;
        }

        case tinyframe_need_more:
            /*
             * We need more bytes! Let's try and receive some.
             *
             * NOTE: This will add bytes to those we already have.
             */
            if (have >= sizeof(buf)) {
                fprintf(stderr, "needed more but buffer was full\n");
                return;
            }

            if ((r = recv(fd, &buf[have], sizeof(buf) - have, 0)) < 1) {
                if (r < 0) {
                    fprintf(stderr, "read() failed: %s\n", strerror(errno));
                }
                return;
            }

            printf("received %zd bytes\n", r);
            have += r;
            break;

        case tinyframe_error:
            fprintf(stderr, "tinyframe_read() error\n");
            return;

        case tinyframe_stopped:
            printf("stopped\n");
            return;

        case tinyframe_finished:
            printf("finished\n");
            return;

        default:
            fprintf(stderr, "tinyframe_read() unexpected result returned\n");
            return;
        }

        /*
         * If we processed any content in the buffer we might have some left
         * so we move what's left to the start of the buffer.
         */
        if (res != tinyframe_need_more) {
            if (reader.bytes_read > have) {
                fprintf(stderr, "tinyframe problem, bytes processed more then we had\n");
                return;
            }

            have -= reader.bytes_read;

            if (have) {
                memmove(buf, &buf[reader.bytes_read], have);
            }
        }
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: simple_receiver <IP> <port>\n");
        return 1;
    }

    /*
     * We setup and listen on the IP and port given on command line.
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

    if (bind(sockfd, (struct sockaddr*)addr, addrlen)) {
        fprintf(stderr, "bind() failed: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }
    printf("bind\n");

    if (listen(sockfd, 0)) {
        fprintf(stderr, "listen() failed: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }
    printf("listen\n");

    int clifd = accept(sockfd, 0, 0);
    if (clifd < 0) {
        fprintf(stderr, "accept() failed: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }
    printf("accept\n");

    /*
     * We have accepted connection from the sender!
     */

    handle_client(clifd);

    /*
     * Time to exit, let's shutdown and close the sockets.
     */

    shutdown(clifd, SHUT_RDWR);
    close(clifd);
    close(sockfd);

    return 0;
}

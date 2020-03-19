#include <dnswire/writer.h>

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "create_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: sender [ <IP> <port> | <unix socket path> ]\n");
        return 1;
    }

    /*
     * We first initialize the writer and check that it can allocate the
     * buffers it needs.
     */

    struct dnswire_writer writer;

    if (dnswire_writer_init(&writer) != dnswire_ok) {
        fprintf(stderr, "Unable to initialize dnswire writer\n");
        return 1;
    }

    /*
     * Use bidirectional communication over the TCP or UNIX socket.
     */
    if (dnswire_writer_set_bidirectional(&writer, true) != dnswire_ok) {
        fprintf(stderr, "Unable to set dnswire writer to bidirectional mode\n");
        return 1;
    }

    int sockfd;

    if (argc == 2) {
        /*
         * We setup and connect to a unix socket at the given path on command line.
         */

        struct sockaddr_un path;

        memset(&path, 0, sizeof(struct sockaddr_un));
        path.sun_family = AF_UNIX;
        strncpy(path.sun_path, argv[1], sizeof(path.sun_path) - 1);

        sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd == -1) {
            fprintf(stderr, "socket() failed: %s\n", strerror(errno));
            return 1;
        }
        printf("socket\n");

        if (connect(sockfd, (struct sockaddr*)&path, sizeof(struct sockaddr_un))) {
            fprintf(stderr, "connect() failed: %s\n", strerror(errno));
            close(sockfd);
            return 1;
        }
        printf("connect\n");
    } else {
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

        addr->sin_port = ntohs(atoi(argv[2]));

        sockfd = socket(addr->sin_family, SOCK_STREAM, IPPROTO_TCP);
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
    }

    /*
     * We are now connected!
     *
     * Now we create a DNSTAP message.
     */

    struct dnstap d = create_dnstap("sender");

    /*
     * We set the DNSTAP message the writer should write.
     */

    dnswire_writer_set_dnstap(writer, &d);

    /*
     * We now loop and wait for the DNSTAP message to be written.
     */

    int done = 0;

    printf("sending...\n");
    while (!done) {
        switch (dnswire_writer_write(&writer, sockfd)) {
        case dnswire_ok:
            /*
             * The DNSTAP message was written successfully, we can now set
             * a new DNSTAP message for the writer or stop the stream.
             *
             * This stops the stream, loop again until it's stopped.
             */
            if (dnswire_writer_stop(&writer) != dnswire_ok) {
                printf("stopping failed!\n");
                done = 1;
            }
            break;
        case dnswire_again:
            /*
             * This indicates that we need to call the writer again as it
             * will only do one pass in a non-blocking fasion.
             */
            break;
        case dnswire_endofdata:
            /*
             * The stream is stopped, we're done!
             */
            printf("stopped\n");
            done = 1;
            break;
        default:
            fprintf(stderr, "dnswire_writer_write() error\n");
            done = 1;
        }
    }

    dnswire_writer_destroy(writer);

    /*
     * Time to exit, let's shutdown and close the sockets.
     */

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    return 0;
}

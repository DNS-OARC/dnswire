#include <dnswire/reader.h>

#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "print_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: simple_receiver <IP> <port>\n");
        return 1;
    }

    /*
     * We first initialize the reader and check that it can allocate the
     * buffers it needs.
     */

    struct dnswire_reader reader = DNSWIRE_READER_INITIALIZER;

    if (dnswire_reader_init(&reader) != dnswire_ok) {
        fprintf(stderr, "Unable to initialize dnswire reader\n");
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
     *
     * We now loop until we have a DNSTAP message, the stream was stopped
     * or we got an error.
     */

    int done = 0;

    printf("receiving...\n");
    while (!done) {
        switch (dnswire_reader_read(&reader, clifd)) {
        case dnswire_have_dnstap:
            print_dnstap(dnswire_reader_dnstap(reader));
            break;
        case dnswire_again:
        case dnswire_need_more:
            break;
        case dnswire_endofdata:
            printf("stopped\n");
            done = 1;
            break;
        default:
            fprintf(stderr, "dnswire_reader_read() error\n");
            done = 1;
        }
    }

    dnswire_reader_destroy(reader);

    /*
     * Time to exit, let's shutdown and close the sockets.
     */

    shutdown(clifd, SHUT_RDWR);
    close(clifd);
    close(sockfd);

    return 0;
}

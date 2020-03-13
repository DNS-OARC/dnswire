#include <dnswire/reader.h>
#include <dnswire/writer.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 * This is a combination of reader and simpler_sender, comments may
 * be a bit off since they haven't been updated for this.
 */

int main(int argc, const char* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "usage: reader_sender <file> <IP> <port>\n");
        return 1;
    }

    /*
     * First we open the given file and read all of the content.
     */

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    /*
     * We now initialize the reader and check that it can allocate the
     * buffers it needs.
     */

    struct dnswire_reader reader;
    int                   done = 0;

    if (dnswire_reader_init(&reader) != dnswire_ok) {
        fprintf(stderr, "Unable to initialize dnswire reader\n");
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
     * We setup and connect to the IP and port given on command line.
     */

    struct sockaddr_storage addr_store;
    struct sockaddr_in*     addr = (struct sockaddr_in*)&addr_store;
    socklen_t               addrlen;

    if (strchr(argv[2], ':')) {
        addr->sin_family = AF_INET6;
        addrlen          = sizeof(struct sockaddr_in6);
    } else {
        addr->sin_family = AF_INET;
        addrlen          = sizeof(struct sockaddr_in);
    }

    if (inet_pton(addr->sin_family, argv[2], &addr->sin_addr) != 1) {
        fprintf(stderr, "inet_pton(%s) failed: %s\n", argv[2], strerror(errno));
        return 1;
    }

    addr->sin_port = ntohs(atoi(argv[3]));

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
     * We now loop until we have a DNSTAP message, the stream was stopped
     * or we got an error.
     */

    int done2 = 0;

    while (!done) {
        switch (dnswire_reader_fread(&reader, fp)) {
        case dnswire_have_dnstap:
            dnswire_writer_set_dnstap(writer, dnswire_reader_dnstap(reader));

            printf("sending...\n");
            done2 = 0;
            while (!done2) {
                switch (dnswire_writer_write(&writer, sockfd)) {
                case dnswire_ok:
                    /*
                     * The DNSTAP message was written successfully, we can now set
                     * a new DNSTAP message for the writer or stop the stream.
                     */
                    printf("sent\n");
                    done2 = 1;
                    break;
                case dnswire_again:
                    break;
                default:
                    fprintf(stderr, "dnswire_writer_write() error\n");
                    done  = 1;
                    done2 = 2;
                }
            }
            break;
        case dnswire_again:
        case dnswire_need_more:
            break;
        case dnswire_endofdata:
            done = 1;
            break;
        default:
            fprintf(stderr, "dnswire_reader_fread() error\n");
            done = 1;
        }
    }

    if (done != 2) {
        /*
         * This stops the stream, loop again until it's stopped.
         */
        printf("stopping...\n");
        dnswire_writer_stop(&writer);
        done2 = 0;
        while (!done2) {
            switch (dnswire_writer_write(&writer, sockfd)) {
            case dnswire_again:
                break;
            case dnswire_endofdata:
                /*
                 * The stream is stopped, we're done!
                 */
                printf("stopped\n");
                done2 = 1;
                break;
            default:
                fprintf(stderr, "dnswire_writer_write() error\n");
                done2 = 1;
            }
        }
    }

    dnswire_writer_destroy(writer);

    /*
     * Time to exit, let's shutdown and close the sockets.
     */

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    dnswire_reader_destroy(reader);
    fclose(fp);

    return 0;
}

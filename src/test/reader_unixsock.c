#include <dnswire/writer.h>
#include <dnswire/reader.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include "print_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        return 1;
    }

    struct sockaddr_un path;
    memset(&path, 0, sizeof(struct sockaddr_un));
    path.sun_family = AF_UNIX;
    strncpy(path.sun_path, argv[1], sizeof(path.sun_path) - 1);

    int readfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (readfd == -1
        || bind(readfd, (struct sockaddr*)&path, sizeof(struct sockaddr_un))
        || listen(readfd, 1)) {
        close(readfd);
        return 1;
    }
    fprintf(stderr, "bind & listen\n");

    int fd, ret = 1;
    alarm(5);
    if ((fd = accept(readfd, 0, 0))) {
        fprintf(stderr, "accept\n");

        struct dnswire_reader reader;
        if (dnswire_reader_init(&reader) != dnswire_ok) {
            return 1;
        }
        dnswire_reader_allow_bidirectional(&reader, true);

        int done = 0;

        while (!done) {
            switch (dnswire_reader_read(&reader, fd)) {
            case dnswire_have_dnstap:
                print_dnstap(dnswire_reader_dnstap(reader));
                fflush(stdout);
                break;
            case dnswire_again:
            case dnswire_need_more:
                break;
            case dnswire_endofdata:
                done = 1;
                break;
            default:
                fprintf(stderr, "dnswire_reader_read() error\n");
                shutdown(fd, SHUT_RDWR);
                close(fd);
                close(readfd);
                return 1;
            }
        }

        dnswire_reader_destroy(reader);
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    close(readfd);
    return ret;
}

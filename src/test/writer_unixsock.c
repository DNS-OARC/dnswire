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

#include "create_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        return 1;
    }

    struct sockaddr_un path;
    memset(&path, 0, sizeof(struct sockaddr_un));
    path.sun_family = AF_UNIX;
    strncpy(path.sun_path, argv[1], sizeof(path.sun_path) - 1);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        return 1;
    }

    int ret = 1, attempts = 5;
    while (attempts--) {
        if (!(ret = connect(fd, (struct sockaddr*)&path, sizeof(struct sockaddr_un)))) {
            break;
        } else {
            fprintf(stderr, "sleep\n");
            sleep(1);
        }
    }
    if (ret) {
        close(fd);
        return ret;
    }

    struct dnswire_writer writer;
    if (dnswire_writer_init(&writer) != dnswire_ok) {
        return 1;
    }
    if (dnswire_writer_set_bidirectional(&writer, true) != dnswire_ok) {
        return 1;
    }

    // force small buffers to trigger buf resizing
    writer.read_size = 4;
    writer.read_inc  = 4;
    if (dnswire_writer_set_bufsize(&writer, 4) != dnswire_ok) {
        return 1;
    }
    if (dnswire_writer_set_bufinc(&writer, 4) != dnswire_ok) {
        return 1;
    }

    struct dnstap d = DNSTAP_INITIALIZER;
    create_dnstap(&d, "writer_reader_unixsock-1");

    dnswire_writer_set_dnstap(writer, &d);

    while (1) {
        enum dnswire_result res = dnswire_writer_write(&writer, fd);
        switch (res) {
        case dnswire_ok:
            break;

        case dnswire_again:
        case dnswire_need_more:
            continue;

        default:
            fprintf(stderr, "dnswire_writer_write() error\n");
            return 1;
        }
        break;
    }

    create_dnstap(&d, "writer_reader_unixsock-2");

    while (1) {
        enum dnswire_result res = dnswire_writer_write(&writer, fd);
        switch (res) {
        case dnswire_ok:
            break;

        case dnswire_again:
        case dnswire_need_more:
            continue;

        default:
            fprintf(stderr, "dnswire_writer_write() error\n");
            return 1;
        }
        break;
    }

    if (dnswire_writer_stop(&writer) != dnswire_ok) {
        fprintf(stderr, "dnswire_writer_stop() failed\n");
        return 1;
    }

    while (1) {
        enum dnswire_result res = dnswire_writer_write(&writer, fd);
        switch (res) {
        case dnswire_ok:
        case dnswire_endofdata:
            break;

        case dnswire_again:
        case dnswire_need_more:
            continue;

        default:
            fprintf(stderr, "dnswire_writer_write() error\n");
            return 1;
        }
        break;
    }

    dnswire_writer_destroy(writer);

    shutdown(fd, SHUT_RDWR);
    close(fd);

    return ret;
}

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
#include "print_dnstap.c"

int _write(int fd)
{
    struct dnswire_writer writer;
    if (dnswire_writer_init(&writer) != dnswire_ok) {
        return 1;
    }
    if (dnswire_writer_set_bidirectional(&writer, true) != dnswire_ok) {
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
    return 0;
}

int _read(int fd)
{
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
            return 1;
        }
    }

    dnswire_reader_destroy(reader);
    return 0;
}

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

    int writefd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (writefd == -1) {
        close(readfd);
        return 1;
    }

    int pid;
    if (!(pid = fork())) {
        int fd, ret = 1;
        fprintf(stderr, "forked\n");
        fflush(stderr);
        if ((fd = accept(readfd, 0, 0))) {
            fprintf(stderr, "accept\n");
            fflush(stderr);
            ret = _read(fd);
            shutdown(fd, SHUT_RDWR);
            close(fd);
        }
        fprintf(stderr, "fork exit\n");
        fflush(stderr);
        close(readfd);
        return ret;
    }

    int ret = 1;
    if (!connect(writefd, (struct sockaddr*)&path, sizeof(struct sockaddr_un))) {
        ret = _write(writefd);
    }

    shutdown(writefd, SHUT_RDWR);
    close(writefd);

    fprintf(stderr, "kill\n");
    fflush(stderr);
    kill(pid, SIGTERM);
    int wstat;
    fprintf(stderr, "wait\n");
    fflush(stderr);
    wait(&wstat);
    if (!ret) {
        ret = WEXITSTATUS(wstat);
    }
    fprintf(stderr, "exit\n");
    fflush(stderr);

    return ret;
}

#include <dnswire/reader.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "print_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 3) {
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        return 1;
    }

    int rbuf_len = atoi(argv[2]);

    struct dnswire_reader reader = DNSWIRE_READER_INITIALIZER;
    if (dnswire_reader_init(&reader) != dnswire_ok) {
        return 1;
    }

    size_t  r;
    uint8_t rbuf[rbuf_len];
    int     done = 0;

    enum dnswire_result res = dnswire_need_more;

    while (!done) {
        switch (res) {
        case dnswire_have_dnstap:
            print_dnstap(dnswire_reader_dnstap(reader));
            res = dnswire_need_more;
            break;
        case dnswire_again:
            res = dnswire_reader_push(&reader, rbuf, 0);
            break;
        case dnswire_need_more:
            r = fread(rbuf, 1, sizeof(rbuf), fp);
            if (r > 0) {
                printf("read %zu\n", r);
            }
            res = dnswire_reader_push(&reader, rbuf, r);
            break;
        case dnswire_endofdata:
            done = 1;
            break;
        default:
            fprintf(stderr, "dnswire_reader_add() error\n");
            done = 1;
        }
    }

    dnswire_reader_destroy(reader);
    fclose(fp);
    return 0;
}

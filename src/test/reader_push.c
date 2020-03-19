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

    struct dnswire_reader reader;
    if (dnswire_reader_init(&reader) != dnswire_ok) {
        return 1;
    }
    dnswire_reader_allow_bidirectional(&reader, false);

    size_t  have = 0, at = 0;
    uint8_t rbuf[rbuf_len];
    int     done = 0;

    enum dnswire_result res = dnswire_need_more;

    while (!done) {
        switch (res) {
        case dnswire_have_dnstap:
            print_dnstap(dnswire_reader_dnstap(reader));
        // fallthrough
        case dnswire_again:
            res = dnswire_reader_push(&reader, &rbuf[at], have, 0, 0);
            have -= dnswire_reader_pushed(reader);
            at += dnswire_reader_pushed(reader);
            break;
        case dnswire_need_more:
            if (!have) {
                have = fread(rbuf, 1, sizeof(rbuf), fp);
                printf("read %zu\n", have);
                at = 0;
            }
            res = dnswire_reader_push(&reader, &rbuf[at], have, 0, 0);
            have -= dnswire_reader_pushed(reader);
            at += dnswire_reader_pushed(reader);
            break;
        case dnswire_endofdata:
            done = 1;
            break;
        default:
            fprintf(stderr, "dnswire_reader_push() error\n");
            return 1;
        }
    }

    dnswire_reader_destroy(reader);
    fclose(fp);
    return 0;
}

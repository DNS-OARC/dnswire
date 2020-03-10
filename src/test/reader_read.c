#include <dnswire/reader.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "print_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        return 1;
    }

    struct dnswire_reader reader;
    if (dnswire_reader_init(&reader) != dnswire_ok) {
        return 1;
    }
    dnswire_reader_allow_bidirectional(&reader, false);

    int done = 0;

    while (!done) {
        switch (dnswire_reader_fread(&reader, fp)) {
        case dnswire_have_dnstap:
            print_dnstap(dnswire_reader_dnstap(reader));
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
    fclose(fp);
    return 0;
}

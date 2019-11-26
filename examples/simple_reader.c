#include <dnswire/reader.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "print_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: simple_reader <file>\n");
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

    struct dnswire_reader reader = DNSWIRE_READER_INITIALIZER;
    int                   done   = 0;

    if (dnswire_reader_init(&reader) != dnswire_ok) {
        fprintf(stderr, "Unable to initialize dnswire reader\n");
        return 1;
    }

    /*
     * We now loop until we have a DNSTAP message, the stream was stopped
     * or we got an error.
     */

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
            fprintf(stderr, "dnswire_reader_fread() error\n");
            done = 1;
        }
    }

    dnswire_reader_destroy(reader);
    fclose(fp);

    return 0;
}

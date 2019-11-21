#include <dnswire/writer.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "create_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: simple_writer <file>\n");
        return 1;
    }

    /*
     * We start by opening the output file for writing.
     */

    FILE* fp = fopen(argv[1], "w");
    if (!fp) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    /*
     * Now we create a DNSTAP message.
     */

    struct dnstap d = create_dnstap("simple_writer");

    struct dnswire_writer writer = DNSWIRE_WRITER_INITIALIZER;
    int                   done   = 0;

    /*
     * We set the DNSTAP message the writer should write.
     */
    dnswire_writer_set_dnstap(writer, &d);

    /*
     * We now loop and wait for the DNSTAP message to be written.
     */

    while (!done) {
        switch (dnswire_writer_fwrite(&writer, fp)) {
        case dnswire_ok:
            /*
             * The DNSTAP message was written successfully, we can now set
             * a new DNSTAP message for the writer or stop the stream.
             *
             * This stops the stream, loop again until it's stopped.
             */
            dnswire_writer_stop(&writer);
            break;
        case dnswire_again:
            break;
        case dnswire_endofdata:
            /*
             * The stream is stopped, we're done!
             */
            done = 1;
            break;
        default:
            fprintf(stderr, "dnswire_writer_fwrite() error\n");
            done = 1;
        }
    }

    dnswire_writer_cleanup(writer);
    fclose(fp);
    return 0;
}

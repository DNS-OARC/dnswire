#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "print_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: simple_reader <file>\n");
        return 1;
    }

    /*
     * First we open the given file and read all of the content.
     *
     * At the end of this section you will have the file's content in
     * an allocated buffer `content`, the variable `content_length` will
     * have the size of the content.
     *
     */

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    long content_length;

    if (fseek(fp, 0, SEEK_END)) {
        fprintf(stderr, "fseek(0, SEEK_END) failed: %s\n", strerror(errno));
        return 1;
    }
    content_length = ftell(fp);
    if (fseek(fp, 0, SEEK_SET)) {
        fprintf(stderr, "fseek(0, SEEK_SET) failed: %s\n", strerror(errno));
        return 1;
    }

    uint8_t* content = malloc(content_length);
    if (!content) {
        fprintf(stderr, "malloc() failed, out of memory\n");
        return 1;
    }

    if (fread(content, 1, content_length, fp) != content_length) {
        fprintf(stderr, "fread() failed, did not read all of file\n");
        return 1;
    }

    fclose(fp);

    printf("read %ld bytes\n", content_length);

    /*
     * Now we decode the Frame Streams using tinyframe.
     *
     * We use the variable `decoding` to point where we are in the process
     * of decoding the content and the variable `left` to indicate how much
     * more content we have.
     *
     */

    struct tinyframe_reader reader   = TINYFRAME_READER_INITIALIZER;
    uint8_t*                decoding = content;
    size_t                  left     = content_length;

    /*
     * We should first have a control frame with the type of `start`.
     */

    if (tinyframe_read(&reader, decoding, left) != tinyframe_have_control
        || reader.control.type != TINYFRAME_CONTROL_START) {
        fprintf(stderr, "Not a control, or not type start\n");
        return 1;
    }
    decoding += reader.bytes_read;
    left -= reader.bytes_read;

    /*
     * We now read the control fields and `start` should have one
     * `content type` and the content we want is `protobuf:dnstap.Dnstap`.
     */

    if (tinyframe_read(&reader, decoding, left) != tinyframe_have_control_field
        || reader.control_field.type != TINYFRAME_CONTROL_FIELD_CONTENT_TYPE
        || strncmp("protobuf:dnstap.Dnstap", (const char*)reader.control_field.data, reader.control_field.length)) {
        fprintf(stderr, "Not a control field, or not content type dnstap\n");
        return 1;
    }
    decoding += reader.bytes_read;
    left -= reader.bytes_read;

    /*
     * Now we read all frames and use `print_dnstap()` to decode the
     * DNSTAP content.
     */

    while (1) {
        switch (tinyframe_read(&reader, decoding, left)) {
        case tinyframe_have_frame:
            print_dnstap(reader.frame.data, reader.frame.length);
            break;
        case tinyframe_stopped:
        case tinyframe_finished:
            return 0;
        default:
            fprintf(stderr, "tinyframe_read() error or unexpected frame read\n");
            return 1;
        }

        decoding += reader.bytes_read;
        left -= reader.bytes_read;
    }

    return 0;
}

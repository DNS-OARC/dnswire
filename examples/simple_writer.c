#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "create_dnstap.c"

static char content_type[] = "protobuf:dnstap.Dnstap";

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

    /*
     * Now that the message is prepared we can begin encapsulating it in
     * protobuf and Frame Streams.
     *
     * First we ask what the encoded size of the protobuf message would be
     * to allocate a buffer for it, then we encode it.
     */

    size_t  len = dnstap_encode_protobuf_size(&d);
    uint8_t frame[len];
    dnstap_encode_protobuf(&d, frame);

    /*
     * Next we initialize a tinyframe writer to write out the Frame Streams.
     */

    struct tinyframe_writer writer = TINYFRAME_WRITER_INITIALIZER;

    /*
     * We create a buffer that holds the control frames and the data frame,
     * tinyframe can indicate when it needs more buffer space but we skip
     * that here to simplify the example.
     */

    uint8_t out[len + (TINYFRAME_CONTROL_FRAME_LENGTH_MAX * 3)];
    len          = sizeof(out);
    size_t wrote = 0;

    /*
     * First we write, to the buffer, a control start with a content type
     * control field for the DNSTAP protobuf content type.
     */

    if (tinyframe_write_control_start(&writer, &out[wrote], len, content_type, sizeof(content_type) - 1) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_control_start() failed\n");
        return 1;
    }
    wrote += writer.bytes_wrote;
    len -= writer.bytes_wrote;

    /*
     * Then we write, to the buffer, a data frame with the encoded DNSTAP
     * message.
     */

    if (tinyframe_write_frame(&writer, &out[wrote], len, frame, sizeof(frame)) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_frame() failed\n");
        return 1;
    }
    wrote += writer.bytes_wrote;
    len -= writer.bytes_wrote;

    /*
     * Lastly we write, to the buffer, a control stop to indicate the end.
     */

    if (tinyframe_write_control_stop(&writer, &out[wrote], len) != tinyframe_ok) {
        fprintf(stderr, "tinyframe_write_control_stop() failed\n");
        return 1;
    }
    wrote += writer.bytes_wrote;
    len -= writer.bytes_wrote;

    /*
     * Now we write the buffer to the file we opened previously.
     */

    if (fwrite(out, 1, wrote, fp) != wrote) {
        fprintf(stderr, "fwrite() failed\n");
        return 1;
    }
    printf("wrote %zu bytes\n", wrote);

    fclose(fp);
    return 0;
}

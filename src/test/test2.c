#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include "create_dnstap.c"

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        return 1;
    }

    FILE* fp = fopen(argv[1], "w");
    if (!fp) {
        return 2;
    }

    struct dnstap d = DNSTAP_INITIALIZER;
    create_dnstap(&d, "test2");

    size_t  len = dnstap_encode_protobuf_size(&d);
    uint8_t frame[len];
    dnstap_encode_protobuf(&d, frame);

    uint8_t out[4096];
    size_t  wrote                          = 0;
    len                                    = sizeof(out);
    struct tinyframe_writer h              = TINYFRAME_WRITER_INITIALIZER;
    static char             content_type[] = "protobuf:dnstap.Dnstap";

    if (tinyframe_write_control_start(&h, &out[wrote], len, content_type, sizeof(content_type) - 1) != tinyframe_ok) {
        printf("tinyframe_write_control_start() failed\n");
        return 1;
    }
    wrote += h.bytes_wrote;
    len -= h.bytes_wrote;

    if (tinyframe_write_frame(&h, &out[wrote], len, frame, sizeof(frame)) != tinyframe_ok) {
        printf("tinyframe_write_frame() failed\n");
        return 1;
    }
    wrote += h.bytes_wrote;
    len -= h.bytes_wrote;

    if (tinyframe_write_control_stop(&h, &out[wrote], len) != tinyframe_ok) {
        printf("tinyframe_write_control_stop() failed\n");
        return 1;
    }
    wrote += h.bytes_wrote;
    len -= h.bytes_wrote;

    if (fwrite(out, 1, wrote, fp) != wrote) {
        printf("fwrite() failed\n");
        return 1;
    }

    fclose(fp);
    return 0;
}

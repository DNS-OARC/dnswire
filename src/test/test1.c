#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "print_dnstap.c"

static bool print_dnstap_frame(const uint8_t* data, size_t len_data)
{
    struct dnstap d = DNSTAP_INITIALIZER;

    if (dnstap_decode_protobuf(&d, data, len_data)) {
        fprintf(stderr, "%s: dnstap_decode_protobuf() failed.\n", __func__);
        return false;
    }

    print_dnstap(&d);
    dnstap_cleanup(&d);

    return true;
}

int main(int argc, const char* argv[])
{
    if (argc < 3) {
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        return 2;
    }

    int rbuf_len = atoi(argv[2]);

    struct tinyframe_reader h = TINYFRAME_READER_INITIALIZER;

    size_t  s = 0, r;
    uint8_t buf[4096], rbuf[rbuf_len];
    while ((r = fread(rbuf, 1, sizeof(rbuf), fp)) > 0) {
        printf("read %zu\n", r);

        if (s + r > sizeof(buf)) {
            printf("overflow\n");
            break;
        }
        memcpy(&buf[s], rbuf, r);
        s += r;

        int r = 1;
        while (r) {
            switch (tinyframe_read(&h, buf, s)) {
            case tinyframe_have_control:
                printf("control type %" PRIu32 " len %" PRIu32 "\n", h.control.type, h.control.length);
                break;
            case tinyframe_have_control_field:
                printf("control_field type %" PRIu32 " len %" PRIu32 " data: %*s\n", h.control_field.type, h.control_field.length, h.control_field.length, h.control_field.data);
                if (strncmp("protobuf:dnstap.Dnstap", (const char*)h.control_field.data, h.control_field.length)) {
                    return 3;
                }
                break;
            case tinyframe_have_frame:
                printf("frame len %" PRIu32 "\n", h.frame.length);
                print_dnstap_frame(h.frame.data, h.frame.length);
                break;
            case tinyframe_need_more:
                printf("need more\n");
                r = 0;
                break;
            case tinyframe_error:
                printf("error\n");
                return 2;
            case tinyframe_stopped:
                printf("stopped\n");
                fclose(fp);
                r = 0;
                break;
            case tinyframe_finished:
                printf("finished\n");
                fclose(fp);
                r = 0;
                break;
            default:
                break;
            }

            if (r && h.bytes_read && h.bytes_read <= s) {
                s -= h.bytes_read;
                if (s) {
                    memmove(buf, &buf[h.bytes_read], s);
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

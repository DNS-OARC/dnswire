#include <dnstap/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

static const char* printable_string(const uint8_t* data, size_t len)
{
    static char buf[512];
    size_t      r = 0, w = 0;

    while (r < len && w < sizeof(buf) - 1) {
        if (isprint(data[r])) {
            buf[w++] = data[r++];
        } else {
            if (w + 4 >= sizeof(buf) - 1) {
                break;
            }

            sprintf(&buf[w], "\\x%02x", data[r++]);
            w += 4;
        }
    }
    if (w >= sizeof(buf)) {
        buf[sizeof(buf) - 1] = 0;
    } else {
        buf[w] = 0;
    }

    return buf;
}

static const char* printable_ip_address(const uint8_t* data, size_t len)
{
    static char buf[INET6_ADDRSTRLEN];

    buf[0] = 0;
    if (len == 4) {
        inet_ntop(AF_INET, data, buf, sizeof(buf));
    } else if (len == 16) {
        inet_ntop(AF_INET6, data, buf, sizeof(buf));
    }

    return buf;
}

static void print_dnstap(const uint8_t* data, size_t len_data)
{
    struct dnstap d = DNSTAP_INITIALIZER;

    /*
     * First we decode the DNSTAP protobuf message.
     */

    if (dnstap_decode_protobuf(&d, data, len_data)) {
        fprintf(stderr, "dnstap_decode_protobuf() failed\n");
        return;
    }

    printf("---- dnstap\n");

    /*
     * Now we can print the available information in the message.
     */

    if (dnstap_has_identity(d)) {
        printf("identity: %s\n", printable_string(dnstap_identity(d), dnstap_identity_length(d)));
    }
    if (dnstap_has_version(d)) {
        printf("version: %s\n", printable_string(dnstap_version(d), dnstap_version_length(d)));
    }
    if (dnstap_has_extra(d)) {
        printf("extra: %s\n", printable_string(dnstap_extra(d), dnstap_extra_length(d)));
    }

    if (dnstap_type(d) == DNSTAP_TYPE_MESSAGE && dnstap_has_message(d)) {
        printf("message:\n  type: %s\n", DNSTAP_TYPE_STRING[dnstap_type(d)]);

        if (dnstap_message_has_query_time_sec(d) && dnstap_message_has_query_time_nsec(d)) {
            printf("  query_time: %" PRIu64 ".%" PRIu32 "\n", dnstap_message_query_time_sec(d), dnstap_message_query_time_nsec(d));
        }
        if (dnstap_message_has_response_time_sec(d) && dnstap_message_has_response_time_nsec(d)) {
            printf("  response_time: %" PRIu64 ".%" PRIu32 "\n", dnstap_message_response_time_sec(d), dnstap_message_response_time_nsec(d));
        }
        if (dnstap_message_has_socket_family(d)) {
            printf("  socket_family: %s\n", DNSTAP_SOCKET_FAMILY_STRING[dnstap_message_socket_family(d)]);
        }
        if (dnstap_message_has_socket_protocol(d)) {
            printf("  socket_protocol: %s\n", DNSTAP_SOCKET_PROTOCOL_STRING[dnstap_message_socket_protocol(d)]);
        }
        if (dnstap_message_has_query_address(d)) {
            printf("  query_address: %s\n", printable_ip_address(dnstap_message_query_address(d), dnstap_message_query_address_length(d)));
        }
        if (dnstap_message_has_query_port(d)) {
            printf("  query_port: %u\n", dnstap_message_query_port(d));
        }
        if (dnstap_message_has_response_address(d)) {
            printf("  response_address: %s\n", printable_ip_address(dnstap_message_response_address(d), dnstap_message_response_address_length(d)));
        }
        if (dnstap_message_has_response_port(d)) {
            printf("  response_port: %u\n", dnstap_message_response_port(d));
        }
        if (dnstap_message_has_query_zone(d)) {
            printf("  query_zone: %s\n", printable_string(dnstap_message_query_zone(d), dnstap_message_query_zone_length(d)));
        }
        if (dnstap_message_has_query_message(d)) {
            printf("  query_message_length: %lu\n", dnstap_message_query_message_length(d));
            printf("  query_message: %s\n", printable_string(dnstap_message_query_message(d), dnstap_message_query_message_length(d)));
        }
        if (dnstap_message_has_response_message(d)) {
            printf("  response_message_length: %lu\n", dnstap_message_response_message_length(d));
            printf("  response_message: %s\n", printable_string(dnstap_message_response_message(d), dnstap_message_response_message_length(d)));
        }
    }

    printf("----\n");

    /*
     * And finally we cleanup protobuf allocations.
     */

    dnstap_cleanup(&d);
}

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

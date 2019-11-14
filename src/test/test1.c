#include <dnstap/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

static void
print_string(const void* data, size_t len, FILE* out)
{
    uint8_t* str = (uint8_t*)data;
    fputc('"', out);
    while (len-- != 0) {
        unsigned c = *(str++);
        if (isprint(c)) {
            if (c == '"')
                fputs("\\\"", out);
            else
                fputc(c, out);
        } else {
            fprintf(out, "\\x%02x", c);
        }
    }
    fputc('"', out);
}

static bool
print_ip_address(const uint8_t* data, size_t len, FILE* fp)
{
    char buf[INET6_ADDRSTRLEN] = { 0 };

    if (len == 4) {
        /* Convert IPv4 address. */
        if (!inet_ntop(AF_INET, data, buf, sizeof(buf)))
            return false;
    } else if (len == 16) {
        /* Convert IPv6 address. */
        if (!inet_ntop(AF_INET6, data, buf, sizeof(buf)))
            return false;
    } else {
        /* Unknown address family. */
        return false;
    }

    /* Print the presentation form of the IP address. */
    fputs(buf, fp);

    /* Success. */
    return true;
}

static bool
print_timestamp(uint64_t timestamp_sec, uint32_t timestamp_nsec, FILE* fp)
{
    static const char* fmt = "%F %H:%M:%S";

    char      buf[100] = { 0 };
    struct tm tm;
    time_t    t = (time_t)timestamp_sec;

    /* Convert arguments to broken-down 'struct tm'. */
    if (!gmtime_r(&t, &tm))
        return false;

    /* Format 'tm' into 'buf'. */
    if (strftime(buf, sizeof(buf), fmt, &tm) <= 0)
        return false;

    /* Print the timestamp. */
    fputs(buf, fp);
    fprintf(fp, ".%06u", timestamp_nsec / 1000);

    /* Success. */
    return true;
}

void print_dnstap_dnstap(struct dnstap* d, FILE* fp)
{
    if (dnstap_has_identity(*d)) {
        fputs("identity: ", fp);
        print_string(dnstap_identity(*d), dnstap_identity_length(*d), fp);
        fputc('\n', fp);
    }

    /* Print 'version' field. */
    if (dnstap_has_version(*d)) {
        fputs("version: ", fp);
        print_string(dnstap_version(*d), dnstap_version_length(*d), fp);
        fputc('\n', fp);
    }

    if (dnstap_type(*d) == DNSTAP_TYPE_MESSAGE && dnstap_has_message(*d)) {
        fputs("message:\n", fp);

        fputs("  type: ", fp);
        fputs(DNSTAP_TYPE_STRING[dnstap_type(*d)], fp);
        fputc('\n', fp);

        /* Print 'query_time' field. */
        if (dnstap_message_has_query_time_sec(*d) && dnstap_message_has_query_time_nsec(*d)) {
            fputs("  query_time: !!timestamp ", fp);
            print_timestamp(dnstap_message_query_time_sec(*d), dnstap_message_query_time_nsec(*d), fp);
            fputc('\n', fp);
        }

        /* Print 'response_time' field. */
        if (dnstap_message_has_response_time_sec(*d) && dnstap_message_has_response_time_nsec(*d)) {
            fputs("  response_time: !!timestamp ", fp);
            print_timestamp(dnstap_message_response_time_sec(*d), dnstap_message_response_time_nsec(*d), fp);
            fputc('\n', fp);
        }

        /* Print 'socket_family' field. */
        if (dnstap_message_has_socket_family(*d)) {
            fputs("  socket_family: ", fp);
            fputs(DNSTAP_SOCKET_FAMILY_STRING[dnstap_message_socket_family(*d)], fp);
            fputc('\n', fp);
        }

        /* Print 'socket_protocol' field. */
        if (dnstap_message_has_socket_protocol(*d)) {
            fputs("  socket_protocol: ", fp);
            fputs(DNSTAP_SOCKET_PROTOCOL_STRING[dnstap_message_socket_protocol(*d)], fp);
            fputc('\n', fp);
        }

        /* Print 'query_address' field. */
        if (dnstap_message_has_query_address(*d)) {
            fputs("  query_address: ", fp);
            print_ip_address(dnstap_message_query_address(*d), dnstap_message_query_address_length(*d), fp);
            fputc('\n', fp);
        }

        /* Print 'response_address field. */
        if (dnstap_message_has_response_address(*d)) {
            fputs("  response_address: ", fp);
            print_ip_address(dnstap_message_response_address(*d), dnstap_message_response_address_length(*d), fp);
            fputc('\n', fp);
        }

        /* Print 'query_port' field. */
        if (dnstap_message_has_query_port(*d))
            fprintf(fp, "  query_port: %u\n", dnstap_message_query_port(*d));

        /* Print 'response_port' field. */
        if (dnstap_message_has_response_port(*d))
            fprintf(fp, "  response_port: %u\n", dnstap_message_response_port(*d));

        /* Print 'query_zone' field. */
        if (dnstap_message_has_query_zone(*d)) {
            fputs("  query_zone: ", fp);
            // print_domain_name(&dnstap_message_query_zone(*d), fp);
            fputc('\n', fp);
        }

        /* Print 'query_message' field. */
        if (dnstap_message_has_query_message(*d)) {
            // if (!print_dns_message(&dnstap_message_query_message(*d), "query_message", fp))
            // 	return;
        }

        /* Print 'response_message' field .*/
        if (dnstap_message_has_response_message(*d)) {
            // if (!print_dns_message(&dnstap_message_response_message(*d), "response_message", fp))
            // 	return;
        }

        /* Success. */
        fputs("---\n", fp);
    }
}

static bool print_dnstap_frame(const uint8_t* data, size_t len_data)
{
    struct dnstap d = DNSTAP_INITIALIZER;

    if (dnstap_decode_protobuf(&d, data, len_data)) {
        fprintf(stderr, "%s: dnstap_decode_protobuf() failed.\n", __func__);
        return false;
    }

    print_dnstap_dnstap(&d, stdout);
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

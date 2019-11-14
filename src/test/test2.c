#include <dnstap/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <arpa/inet.h>
#include <time.h>

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

    dnstap_set_identity_string(d, "test2");
    dnstap_set_version_string(d, "0.0.0");
    dnstap_set_extra(d, "12345", 5);

    dnstap_set_type(d, DNSTAP_TYPE_MESSAGE);

    dnstap_message_set_type(d, DNSTAP_MESSAGE_TYPE_TOOL_QUERY);
    dnstap_message_set_socket_family(d, DNSTAP_SOCKET_FAMILY_INET);
    dnstap_message_set_socket_protocol(d, DNSTAP_SOCKET_PROTOCOL_TCP);

    unsigned char query_address[sizeof(struct in_addr)];
    if (inet_pton(AF_INET, "127.0.0.1", query_address) == 1) {
        dnstap_message_set_query_address(d, query_address, sizeof(query_address));
    }
    dnstap_message_set_query_port(d, 12345);

    struct timespec query_time = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &query_time);
    dnstap_message_set_query_time_sec(d, query_time.tv_sec);
    dnstap_message_set_query_time_nsec(d, query_time.tv_nsec);

    unsigned char response_address[sizeof(struct in_addr)];
    if (inet_pton(AF_INET, "127.0.0.1", response_address) == 1) {
        dnstap_message_set_response_address(d, response_address, sizeof(response_address));
    }
    dnstap_message_set_response_port(d, 53);

    struct timespec response_time = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &response_time);
    dnstap_message_set_response_time_sec(d, response_time.tv_sec);
    dnstap_message_set_response_time_nsec(d, response_time.tv_nsec);

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

#include <dnswire/dnswire.h>
#include <dnswire/dnstap.h>
#include <tinyframe/tinyframe.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include "print_dnstap.c"

#define BUF_SIZE 4096

uv_loop_t*              loop;
uv_tcp_t                sock;
uv_connect_t            conn;
uint8_t                 rbuf[BUF_SIZE];
size_t                  have   = 0;
struct tinyframe_reader reader = TINYFRAME_READER_INITIALIZER;

void client_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = (char*)&rbuf[have];
    buf->len  = BUF_SIZE - have;
}

void client_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    if (nread > 0) {
        printf("received %zd bytes\n", nread);
        have += nread;

        /*
         * We now process the frames we have in the buffer.
         */

        while (1) {
            switch (tinyframe_read(&reader, rbuf, have)) {
            case tinyframe_have_control:
                /*
                 * If we get a control frame we check that it's a start one,
                 * this code can be extended with checks that you only get one
                 * start frame and it's the first you get.
                 */
                if (reader.control.type != TINYFRAME_CONTROL_START) {
                    fprintf(stderr, "Not a control type start\n");
                    uv_close((uv_handle_t*)handle, 0);
                    return;
                }
                printf("got control start\n");
                break;

            case tinyframe_have_control_field:
                /*
                 * We have a control field, so lets check that it's a
                 * `content type` and the content we want is
                 * `protobuf:dnstap.Dnstap`.
                 */
                if (reader.control_field.type != TINYFRAME_CONTROL_FIELD_CONTENT_TYPE
                    || strncmp("protobuf:dnstap.Dnstap", (const char*)reader.control_field.data, reader.control_field.length)) {
                    fprintf(stderr, "Not content type dnstap\n");
                    uv_close((uv_handle_t*)handle, 0);
                    return;
                }
                printf("got content type DNSTAP\n");
                break;

            case tinyframe_have_frame:
                /*
                 * We got a frame, lets use `print_dnstap()` to decode the
                 * DNSTAP content.
                 */
                print_dnstap(reader.frame.data, reader.frame.length);
                break;

            case tinyframe_need_more:
                /*
                 * We need more bytes, leave this function to let libuv
                 * handle more incoming data.
                 */
                return;

            case tinyframe_error:
                fprintf(stderr, "tinyframe_read() error\n");
                uv_close((uv_handle_t*)handle, 0);
                return;

            case tinyframe_stopped:
                printf("stopped\n");
                uv_close((uv_handle_t*)handle, 0);
                return;

            case tinyframe_finished:
                printf("finished\n");
                uv_close((uv_handle_t*)handle, 0);
                return;

            default:
                fprintf(stderr, "tinyframe_read() unexpected result returned\n");
                uv_close((uv_handle_t*)handle, 0);
                return;
            }

            /*
             * If we processed any content in the buffer we might have some left
             * so we move what's left to the start of the buffer.
             */

            if (reader.bytes_read > have) {
                fprintf(stderr, "tinyframe problem, bytes processed more then we had\n");
                uv_close((uv_handle_t*)handle, 0);
                return;
            }

            have -= reader.bytes_read;

            if (have) {
                memmove(rbuf, &rbuf[reader.bytes_read], have);
            }
        }
    }
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "client_read() error: %s\n", uv_err_name(nread));
        } else {
            printf("disconnected\n");
        }
        uv_close((uv_handle_t*)handle, 0);
    }
}

void on_connect(uv_connect_t* req, int status)
{
    /*
     * We have connected to the sender, check that there was no errors
     * and start receiving incoming frames.
     */

    if (status < 0) {
        fprintf(stderr, "on_connect() error: %s\n", uv_strerror(status));
        return;
    }

    uv_read_start((uv_stream_t*)&sock, client_alloc_buffer, client_read);
}

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: client_receiver_uv <IP> <port>\n");
        return 1;
    }

    /*
     * We setup a TCP client using libuv and connect to the given server.
     */

    struct sockaddr_storage addr;
    int                     port = atoi(argv[2]);

    if (strchr(argv[1], ':')) {
        uv_ip6_addr(argv[1], port, (struct sockaddr_in6*)&addr);
    } else {
        uv_ip4_addr(argv[1], port, (struct sockaddr_in*)&addr);
    }

    loop = uv_default_loop();

    uv_tcp_init(loop, &sock);
    uv_tcp_connect(&conn, &sock, (const struct sockaddr*)&addr, on_connect);

    return uv_run(loop, UV_RUN_DEFAULT);
}

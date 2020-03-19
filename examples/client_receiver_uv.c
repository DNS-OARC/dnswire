#include <dnswire/reader.h>

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

uv_loop_t*   loop;
uv_tcp_t     sock;
uv_connect_t conn;
char         rbuf[BUF_SIZE];

struct dnswire_reader reader;

void client_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = rbuf;
    buf->len  = sizeof(rbuf);
}

void client_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    if (nread > 0) {
        /*
         * We now push all the data we got to the reader.
         */
        size_t pushed = 0;

        while (pushed < nread) {
            enum dnswire_result res = dnswire_reader_push(&reader, (uint8_t*)&buf->base[pushed], nread - pushed, 0, 0);

            pushed += dnswire_reader_pushed(reader);

            switch (res) {
            case dnswire_have_dnstap:
                /*
                 * We got a DNSTAP message, lets print it!
                 */
                print_dnstap(dnswire_reader_dnstap(reader));
                break;
            case dnswire_again:
            case dnswire_need_more:
                break;
            case dnswire_endofdata:
                /*
                 * The remote end sent a control stop or finish.
                 */
                uv_close((uv_handle_t*)handle, 0);
                return;
            default:
                fprintf(stderr, "dnswire_reader_fread() error\n");
                uv_close((uv_handle_t*)handle, 0);
                return;
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
    if (argc < 3) {
        fprintf(stderr, "usage: client_receiver_uv <IP> <port>\n");
        return 1;
    }

    /*
     * We first initialize the reader and check that it can allocate the
     * buffers it needs.
     */

    if (dnswire_reader_init(&reader) != dnswire_ok) {
        fprintf(stderr, "Unable to initialize dnswire reader\n");
        return 1;
    }

    /*
     * We set this reader to reject bidirectional communication.
     */
    if (dnswire_reader_allow_bidirectional(&reader, false) != dnswire_ok) {
        fprintf(stderr, "Unable to deny bidirectional communication\n");
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

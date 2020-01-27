#include <tinyframe/tinyframe.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <errno.h>
#include <stdbool.h>

#include "create_dnstap.c"

#define BUF_SIZE 4096

enum client_state {
    no_state,
    writing_start,
    started,
    writing_frame,
};

struct client;
struct client {
    struct client*    next;
    size_t            id;
    enum client_state state;
    uv_tcp_t          conn;
    char              rbuf[BUF_SIZE];
    uv_write_t        wreq;
    uv_buf_t          wbuf;
    uint8_t           buf[BUF_SIZE];
};

struct client* clients   = 0;
size_t         client_id = 1;

static char content_type[] = "protobuf:dnstap.Dnstap";

uv_loop_t* loop;

struct client* client_new()
{
    struct client* c = malloc(sizeof(struct client));
    if (c) {
        c->conn.data = c;
        c->next      = clients;
        c->id        = client_id++;
        c->state     = no_state;
        c->wbuf.base = (void*)c->buf;
        clients      = c;
    }
    return c;
}

void client_close(uv_handle_t* handle)
{
    struct client* c = handle->data;

    if (clients == c) {
        clients = c->next;
    } else {
        struct client* prev = clients;

        while (prev) {
            if (prev->next == c) {
                prev->next = c->next;
                break;
            }
            prev = prev->next;
        }
    }

    free(c);
}

void client_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = ((struct client*)handle->data)->rbuf;
    buf->len  = BUF_SIZE;
}

void client_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf)
{
    /*
     * We discard any input from the client and only check for errors or
     * if the connection was closed.
     */

    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "client_read() error: %s\n", uv_err_name(nread));
        } else {
            printf("client %zu disconnected\n", ((struct client*)client->data)->id);
        }
        uv_close((uv_handle_t*)client, client_close);
    }
}

void client_write(uv_write_t* req, int status)
{
    /*
     * After a write we check that there was no errors and then set the
     * client in a state that allows `tick()` to send DNSTAP messages to it.
     */

    if (status) {
        fprintf(stderr, "client_write() error: %s\n", uv_strerror(status));
        uv_close((uv_handle_t*)req->handle, client_close);
        return;
    }

    ((struct client*)req->handle->data)->state = started;
}

void on_new_connection(uv_stream_t* server, int status)
{
    if (status < 0) {
        fprintf(stderr, "on_new_connection() error: %s\n", uv_strerror(status));
        return;
    }

    /*
     * We have a new client connecting, create a client struct to hold the
     * connection, accept it and send the control start frame.
     */

    struct client* client = client_new();
    if (!client) {
        fprintf(stderr, "on_new_connection() out of memory\n");
        return;
    }

    uv_tcp_init(loop, &client->conn);
    if (uv_accept(server, (uv_stream_t*)&client->conn) == 0) {
        printf("client %zu connected\n", client->id);

        uv_read_start((uv_stream_t*)&client->conn, client_alloc_buffer, client_read);

        struct tinyframe_writer writer = TINYFRAME_WRITER_INITIALIZER;

        /*
         * First we write, to the buffer, a control start with a content type
         * control field for the DNSTAP protobuf content type.
         *
         * Then we send it.
         */

        if (tinyframe_write_control_start(&writer, client->buf, BUF_SIZE, content_type, sizeof(content_type) - 1) != tinyframe_ok) {
            fprintf(stderr, "tinyframe_write_control_start() failed\n");
            uv_close((uv_handle_t*)&client->conn, client_close);
            return;
        }
        printf("client %zu: sending control start and content type\n", client->id);

        client->wbuf.len = writer.bytes_wrote;
        uv_write((uv_write_t*)&client->wreq, (uv_stream_t*)&client->conn, &client->wbuf, 1, client_write);
        client->state = writing_start;
    } else {
        uv_close((uv_handle_t*)&client->conn, client_close);
    }
}

/*
 * This function is called every second and will create a DNSTAP message
 * and send it to all available clients.
 */

void tick(uv_timer_t* handle)
{
    /*
     * Now we create a DNSTAP message.
     */

    struct dnstap d = create_dnstap("daemon_sender_uv");

    /*
     * Now that the message is prepared we can begin encapsulating it in
     * protobuf and Frame Streams.
     *
     * First we ask what the encoded size of the protobuf message would be
     * and then we allocate a buffer with of that size plus the size of
     * a Frame Streams frame header.
     *
     * Then we encode the DNSTAP message and put it after the frame header
     * and call `tinyframe_set_header()` to set the header.
     */

    size_t  frame_len = dnstap_encode_protobuf_size(&d);
    uint8_t frame[TINYFRAME_HEADER_SIZE + frame_len];
    dnstap_encode_protobuf(&d, &frame[TINYFRAME_HEADER_SIZE]);
    tinyframe_set_header(frame, frame_len);

    if (sizeof(frame) > BUF_SIZE) {
        fprintf(stderr, "frame larger the client's buffers\n");
        exit(1);
    }

    /*
     * We now loop over all the connected clients and send the message
     * to those that are currently not busy.
     */

    struct client* c = clients;
    while (c) {
        if (c->state == started) {
            c->wbuf.len = sizeof(frame);
            memcpy(c->buf, frame, sizeof(frame));
            uv_write((uv_write_t*)&c->wreq, (uv_stream_t*)&c->conn, &c->wbuf, 1, client_write);
            c->state = writing_frame;

            printf("client %zu: sending DNSTAP\n", c->id);
        }
        c = c->next;
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "usage: daemon_sender_uv <IP> <port>\n");
        return 1;
    }

    /*
     * We setup a TCP server using libuv and listen for connections,
     * along with a timer that calls the function to send DNSTAP messages
     * to all clients.
     */

    struct sockaddr_storage addr;
    int                     port = atoi(argv[2]);

    if (strchr(argv[1], ':')) {
        uv_ip6_addr(argv[1], port, (struct sockaddr_in6*)&addr);
    } else {
        uv_ip4_addr(argv[1], port, (struct sockaddr_in*)&addr);
    }

    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&server, 128, on_new_connection);
    if (r) {
        fprintf(stderr, "uv_listen() failed: %s\n", uv_strerror(r));
        return 1;
    }

    uv_timer_t ticker;
    uv_timer_init(loop, &ticker);
    uv_timer_start(&ticker, tick, 1000, 1000);

    return uv_run(loop, UV_RUN_DEFAULT);
}

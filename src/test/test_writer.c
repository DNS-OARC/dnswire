#include <dnswire/writer.h>

#include <assert.h>

int main(void)
{
    uint8_t               buf[1], buf2[1];
    size_t                s;
    struct dnswire_writer w;
    assert(dnswire_writer_init(&w) == dnswire_ok);

    // set bidirectional
    assert(dnswire_writer_set_bidirectional(&w, true) == dnswire_ok);
    assert(dnswire_writer_set_bidirectional(&w, false) == dnswire_ok);

    // set bufsize
    w.left = 2;
    assert(dnswire_writer_set_bufsize(&w, 1) == dnswire_error);
    w.left = 1;
    w.at   = 1;
    assert(dnswire_writer_set_bufsize(&w, 1) == dnswire_ok);
    w.left = 0;
    assert(dnswire_writer_set_bufsize(&w, DNSWIRE_MAXIMUM_BUF_SIZE + 1) == dnswire_error);
    assert(dnswire_writer_set_bufsize(&w, DNSWIRE_DEFAULT_BUF_SIZE) == dnswire_ok);

    // set bufinc
    assert(dnswire_writer_set_bufinc(&w, DNSWIRE_DEFAULT_BUF_SIZE) == dnswire_ok);

    // set bufmax
    assert(dnswire_writer_set_bufmax(&w, DNSWIRE_DEFAULT_BUF_SIZE - 1) == dnswire_error);
    assert(dnswire_writer_set_bufmax(&w, DNSWIRE_MAXIMUM_BUF_SIZE) == dnswire_ok);

    // writer pop
    w.state = dnswire_writer_encoding_ready;
    s       = sizeof(buf2);
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_again);
    w.state = dnswire_writer_reading_accept;
    s       = 0;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_need_more);
    s           = sizeof(buf2);
    w.read_left = 1;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_need_more);
    w.state     = dnswire_writer_stopping;
    w.read_left = 1;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_again);
    w.read_left = 0;
    w.state     = dnswire_writer_reading_finish;
    s           = 0;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_need_more);
    s           = sizeof(buf2);
    w.read_left = 1;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_need_more);
    w.decoder.state = dnswire_decoder_done;
    w.state         = dnswire_writer_decoding_accept;
    w.read_left     = 1;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_error);
    w.state     = dnswire_writer_decoding_finish;
    w.read_left = 1;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_error);
    w.state = dnswire_writer_done;
    assert(dnswire_writer_pop(&w, buf, 1, buf2, &s) == dnswire_error);

    // reset writer
    dnswire_writer_destroy(w);
    assert(dnswire_writer_init(&w) == dnswire_ok);
    assert(dnswire_writer_set_bidirectional(&w, true) == dnswire_ok);

    // writer write
    w.state = dnswire_writer_writing_ready;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.state = dnswire_writer_reading_accept;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.state = dnswire_writer_writing;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.state = dnswire_writer_stopping;
    w.left  = 1;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.state = dnswire_writer_writing_stop;
    w.left  = 1;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.left  = 0;
    w.state = dnswire_writer_reading_finish;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.read_left     = 1;
    w.decoder.state = dnswire_decoder_done;
    w.state         = dnswire_writer_decoding_accept;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.state = dnswire_writer_decoding_finish;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);
    w.state = dnswire_writer_done;
    assert(dnswire_writer_write(&w, -1) == dnswire_error);

    return 0;
}

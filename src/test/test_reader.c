#include <dnswire/reader.h>

#include <assert.h>

int main(void)
{
    uint8_t               buf[1], buf2[1];
    size_t                s;
    struct dnswire_reader r;
    assert(dnswire_reader_init(&r) == dnswire_ok);
    assert(dnswire_reader_allow_bidirectional(&r, true) == dnswire_ok);

    // set bufsize
    r.left = 2;
    assert(dnswire_reader_set_bufsize(&r, 1) == dnswire_error);
    r.left = 1;
    r.at   = 1;
    assert(dnswire_reader_set_bufsize(&r, 1) == dnswire_ok);
    r.left = 0;
    assert(dnswire_reader_set_bufsize(&r, DNSWIRE_MAXIMUM_BUF_SIZE + 1) == dnswire_error);
    assert(dnswire_reader_set_bufsize(&r, DNSWIRE_DEFAULT_BUF_SIZE) == dnswire_ok);

    // set bufinc
    assert(dnswire_reader_set_bufinc(&r, DNSWIRE_DEFAULT_BUF_SIZE) == dnswire_ok);

    // set bufmax
    assert(dnswire_reader_set_bufmax(&r, DNSWIRE_DEFAULT_BUF_SIZE - 1) == dnswire_error);
    assert(dnswire_reader_set_bufmax(&r, DNSWIRE_MAXIMUM_BUF_SIZE) == dnswire_ok);

    // reader push
    r.state = dnswire_reader_reading_control;
    assert(dnswire_reader_push(&r, buf, 0, buf2, &s) == dnswire_need_more);
    r.state = dnswire_reader_encoding_accept;
    s       = sizeof(buf2);
    assert(dnswire_reader_push(&r, buf, 0, buf2, &s) == dnswire_again);
    r.state = dnswire_reader_reading;
    assert(dnswire_reader_push(&r, buf, 0, buf2, &s) == dnswire_need_more);
    r.state = dnswire_reader_encoding_finish;
    assert(dnswire_reader_push(&r, buf, 0, buf2, &s) == dnswire_again);
    r.decoder.state = dnswire_decoder_done;
    r.left          = 1;
    r.state         = dnswire_reader_decoding_control;
    assert(dnswire_reader_push(&r, buf, 0, buf2, &s) == dnswire_error);
    r.state = dnswire_reader_decoding;
    assert(dnswire_reader_push(&r, buf, 0, buf2, &s) == dnswire_error);
    r.state = dnswire_reader_done;
    assert(dnswire_reader_push(&r, buf, 0, buf2, &s) == dnswire_error);

    // reset reader
    dnswire_reader_destroy(r);
    assert(dnswire_reader_init(&r) == dnswire_ok);
    assert(dnswire_reader_allow_bidirectional(&r, true) == dnswire_ok);

    // reader read
    r.state = dnswire_reader_reading_control;
    assert(dnswire_reader_read(&r, -1) == dnswire_error);
    r.state = dnswire_reader_writing_accept;
    assert(dnswire_reader_read(&r, -1) == dnswire_error);
    r.state = dnswire_reader_reading;
    assert(dnswire_reader_read(&r, -1) == dnswire_error);
    r.state = dnswire_reader_writing_finish;
    assert(dnswire_reader_read(&r, -1) == dnswire_error);
    r.decoder.state = dnswire_decoder_done;
    r.left          = 1;
    r.state         = dnswire_reader_decoding_control;
    assert(dnswire_reader_read(&r, -1) == dnswire_error);
    r.state = dnswire_reader_decoding;
    assert(dnswire_reader_read(&r, -1) == dnswire_error);
    r.state = dnswire_reader_done;
    assert(dnswire_reader_read(&r, -1) == dnswire_error);

    return 0;
}

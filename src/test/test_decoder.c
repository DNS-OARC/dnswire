#include <dnswire/decoder.h>

#include <assert.h>

#include "create_dnstap.c"

int main(void)
{
    uint8_t                buf[1];
    struct dnswire_decoder d = DNSWIRE_DECODER_INITIALIZER;

    // decoder * need more
    d.state = dnswire_decoder_reading_control;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_need_more);
    d.state = dnswire_decoder_checking_ready;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_need_more);
    d.state = dnswire_decoder_checking_accept;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_need_more);
    d.state = dnswire_decoder_reading_start;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_need_more);
    d.state = dnswire_decoder_checking_start;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_need_more);
    d.state = dnswire_decoder_reading_frames;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_need_more);
    d.state = dnswire_decoder_checking_finish;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_need_more);

    // decoder done
    d.state = dnswire_decoder_done;
    assert(dnswire_decoder_decode(&d, buf, 1) == dnswire_error);

    return 0;
}

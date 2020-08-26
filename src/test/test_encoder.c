#include <dnswire/encoder.h>

#include <assert.h>

#include "create_dnstap.c"

int main(void)
{
    uint8_t                buf[1];
    struct dnswire_encoder e = DNSWIRE_ENCODER_INITIALIZER;
    struct dnstap          d = DNSTAP_INITIALIZER;

    create_dnstap(&d, "test_encoder");

    // encoder stop at wrong state
    assert(dnswire_encoder_stop(&e) == dnswire_error);

    // encoder * need more
    e.state = dnswire_encoder_control_ready;
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_need_more);
    e.state = dnswire_encoder_control_start;
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_need_more);
    e.state = dnswire_encoder_control_accept;
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_need_more);
    e.state = dnswire_encoder_control_finish;
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_need_more);
    e.state = dnswire_encoder_control_stop;
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_need_more);

    // encoder frame
    e.state = dnswire_encoder_frames;
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_error);
    dnswire_encoder_set_dnstap(e, &d);
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_need_more);

    // encoder done
    e.state = dnswire_encoder_done;
    assert(dnswire_encoder_encode(&e, buf, 1) == dnswire_error);

    return 0;
}

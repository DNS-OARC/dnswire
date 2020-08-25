#include <dnswire/dnstap.h>

#include <assert.h>

#include "create_dnstap.c"

int main(void)
{
    uint8_t       buf[256 * 1024];
    size_t        s;
    struct dnstap d = DNSTAP_INITIALIZER;
    struct dnstap u = DNSTAP_INITIALIZER;

    create_dnstap(&d, "test_dnstap");

    // failed unpack
    assert(dnstap_decode_protobuf(&u, buf, 1) == 1);

    // invalid dnstap.type
    d.dnstap.type = (enum _Dnstap__Dnstap__Type)(DNSTAP_TYPE_MESSAGE + 1);
    s             = dnstap_encode_protobuf_size(&d);
    assert(s < sizeof(buf));
    assert(dnstap_encode_protobuf(&d, buf) == s);
    assert(dnstap_decode_protobuf(&u, buf, s) == 0);
    assert(u.dnstap.type == (enum _Dnstap__Dnstap__Type)DNSTAP_TYPE_UNKNOWN);
    dnstap_cleanup(&u);
    d.dnstap.type = (enum _Dnstap__Dnstap__Type)DNSTAP_TYPE_MESSAGE;

    // invalid message.type
    d.message.type = (enum _Dnstap__Message__Type)(DNSTAP_MESSAGE_TYPE_TOOL_RESPONSE + 1);
    s              = dnstap_encode_protobuf_size(&d);
    assert(s < sizeof(buf));
    assert(dnstap_encode_protobuf(&d, buf) == s);
    assert(dnstap_decode_protobuf(&u, buf, s) == 0);
    assert(u.message.type == (enum _Dnstap__Message__Type)DNSTAP_MESSAGE_TYPE_UNKNOWN);
    dnstap_cleanup(&u);
    d.message.type = (enum _Dnstap__Message__Type)DNSTAP_MESSAGE_TYPE_TOOL_QUERY;

    // invalid message.socket_family
    d.message.socket_family = (enum _Dnstap__SocketFamily)(DNSTAP_SOCKET_FAMILY_INET6 + 1);
    s                       = dnstap_encode_protobuf_size(&d);
    assert(s < sizeof(buf));
    assert(dnstap_encode_protobuf(&d, buf) == s);
    assert(dnstap_decode_protobuf(&u, buf, s) == 0);
    assert(u.message.socket_family == (enum _Dnstap__SocketFamily)DNSTAP_SOCKET_FAMILY_UNKNOWN);
    dnstap_cleanup(&u);
    d.message.socket_family = (enum _Dnstap__SocketFamily)DNSTAP_SOCKET_FAMILY_INET;

    // invalid message.socket_protocol
    d.message.socket_protocol = (enum _Dnstap__SocketProtocol)(DNSTAP_SOCKET_PROTOCOL_TCP + 1);
    s                         = dnstap_encode_protobuf_size(&d);
    assert(s < sizeof(buf));
    assert(dnstap_encode_protobuf(&d, buf) == s);
    assert(dnstap_decode_protobuf(&u, buf, s) == 0);
    assert(u.message.socket_protocol == (enum _Dnstap__SocketProtocol)DNSTAP_SOCKET_PROTOCOL_UNKNOWN);
    dnstap_cleanup(&u);
    d.message.socket_protocol = (enum _Dnstap__SocketProtocol)DNSTAP_SOCKET_PROTOCOL_UDP;

    return 0;
}

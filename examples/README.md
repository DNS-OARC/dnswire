# Examples

- `simple_reader`: Example of reading DNSTAP from a file and printing it's content, using `dnswire_reader`
- `simple_writer`: Example of constructing a DNSTAP message and writing it to a file, using `dnswire_writer`
- `simple_receiver`: Example of receiving a DNSTAP message over a TCP connection and printing it's content, using `dnswire_reader`
- `simple_sender`: Example of constructing a DNSTAP message and sending it over a TCP connection, using `dnswire_writer`
- `daemon_sender_uv`: Example of a daemon that will continuously send DNSTAP messages to connected clients, using the event engine `libuv` and `dnstap_encode_protobuf` along with `tinyframe` to encode once and send to many
- `client_receiver_uv`: Example of a client that will receive DNSTAP message from the daemon, using the event engine `libuv` and `dnswire_reader`
- `reader`: Example of reading DNSTAP using `tinyframe` to show how Frame Streams work
- `writer`: Example of writing DNSTAP using `tinyframe` to show how Frame Streams work
- `receiver`: Example of receiving a DNSTAP message over a TCP connection and printing it's content, using `tinyframe` to show how Frame Streams work
- `sender`: Example of constructing a DNSTAP message and sending it over a TCP connection, using `tinyframe` to show how Frame Streams work
- `simple_reader_sender`: Example of a reader that read DNSTAP from a file and then sends the DNSTAP messages over a TCP connection

## simple_receiver and simple_sender

These examples uses the way of connecting as implemented in the `fstrm`
library, the receiver listens for connections by the sender and the
sender connects to the receiver.

```
$ ./simple_receiver 127.0.0.1 5353
socket
bind
listen
accept
receiving...
---- dnstap
identity: simple_sender
version: 0.1.0
message:
  type: MESSAGE
  query_time: 1574765731.199945162
  response_time: 1574765731.199945362
  socket_family: INET
  socket_protocol: UDP
  query_address: 127.0.0.1
  query_port: 12345
  response_address: 127.0.0.1
  response_port: 53
  query_message_length: 27
  query_message: dns_wire_format_placeholder
  response_message_length: 27
  response_message: dns_wire_format_placeholder
----
stopped
```

```
$ ./simple_sender 127.0.0.1 5353
socket
connect
sending...
sent, stopping...
stopped
```

## daemon_sender_uv and client_receiver_uv

These examples works in the reverse way compared to `simple_receiver` and
`simple_sender`, and maybe a more traditional way, the daemon listens and
accepts connections from new clients, and will continuously send messages to
established clients that are ready to receive them.

```
$ ./daemon_sender_uv 127.0.0.1 5353
client 1 connected
client 1: sending control start and content type
client 1: sending DNSTAP
client 1: sending DNSTAP
client 1 disconnected
```

```
$ ./client_receiver_uv 127.0.0.1 5353
received 42 bytes
got control start
got content type DNSTAP
received 133 bytes
---- dnstap
identity: daemon_sender_uv
version: 0.1.0
message:
  type: MESSAGE
  query_time: 1574257180.95619354
  response_time: 1574257180.95619490
  socket_family: INET
  socket_protocol: UDP
  query_address: 127.0.0.1
  query_port: 12345
  response_address: 127.0.0.1
  response_port: 53
  query_message_length: 27
  query_message: dns_wire_format_placeholder
  response_message_length: 27
  response_message: dns_wire_format_placeholder
----
received 133 bytes
---- dnstap
identity: daemon_sender_uv
version: 0.1.0
message:
  type: MESSAGE
  query_time: 1574257181.96381443
  response_time: 1574257181.96381557
  socket_family: INET
  socket_protocol: UDP
  query_address: 127.0.0.1
  query_port: 12345
  response_address: 127.0.0.1
  response_port: 53
  query_message_length: 27
  query_message: dns_wire_format_placeholder
  response_message_length: 27
  response_message: dns_wire_format_placeholder
----
^C
```

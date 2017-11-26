# rpi-server
rpi-server implementation in C (Experimental)

- Run `make` to build and test the program.

## APIs
- `pthread_t ems_run_server(int port_server, void (*handler)(char *, int))` - Runs a server that listens to incoming connections on `port_server`. `handler` is invoked when there is an incoming packet.
- `int ems_send(struct rpi_i *p)` - Used to send a packet on all TCP sockets created via the server.
- `int ems_destroy(pthread_t s)` - To release resources and terminate the server.
- `ems_send2()`, `ems_test_run_client()` - Used for unit testing.

## Struct `rpi_i`
```c
struct rpi_i {
  uint8_t group;
  uint8_t node;
  uint8_t type;
  uint16_t data; // Currently assumes that data is 16-bit max.
}
```

## Packet format
### Incoming packet
```
0xBB <NodeID> <GroupID> <Sub-packet length> <Sub-packet Type> <Data 0>...<Data N-1>
```
### Outgoing packet
```
0xAA <Packet Length> \
<FCR1><FCR2><Packet count><PAN ID H><PAN ID L> \
<Dest GroupID><Dest NodeID><Source GroupID><Source NodeID><Packet Type> \
<Data 0>...<Data N-1><Footer 1><Footer 2><Footer 3>
```
For the outgoing packet, only the following fields are read:
`0xAA`, `<Packet Length>`, `<Packet count>`, `<Source GroupID>`, `<Source NodeID>`, `<Packet Type>`, and
`<Data 0><Data 1>`.
All other fields can be set to zero. Lastly, It is assumed that the data fields are always **16-bit** long (2 bytes).

## Packet types
### Incoming packet
- `16` - Priority
- `11` - Power
### Outgoing packet
- `12` - Frequency
- `13` - Power
- `14` - Current
- `15` - Voltage



See `server.h` and `server.c` for more details.

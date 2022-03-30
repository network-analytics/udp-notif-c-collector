# C-Collector for UDP-notif
Library for collecting UDP-notif protocol messages defined in the IETF draft [draft-ietf-netconf-udp-notif-04](https://datatracker.ietf.org/doc/html/draft-ietf-netconf-udp-notif-04).

## Compiling and installing project
See [INSTALL](INSTALL.md)

## Usage
### Usage of the UDP-notif collector
The collector allows to read and parse UDP-notif protocol messages from a ip/port specified on the parameters. It allows to get directly the buffer and the metadata of the message in a struct.

The api is in `unyte_udp_collector.h`:
- `int unyte_udp_create_socket(char *address, char *port, uint64_t buffer_size)` from `unyte_udp_utils.h`: Helper that creates and binds a socket to an address and port.
- `int unyte_udp_create_interface_bound_socket(char *interface, char *address, char *port, uint64_t buffer_size)` from `unyte_udp_utils.h`: Helper that creates a socket, binds it to an interface using SO_BINDTODEVICE option and binds it to an adress and port.
- `unyte_udp_collector_t *unyte_udp_start_collector(unyte_udp_options_t *options)` from `unyte_udp_collector.h`: Initialize the UDP-notif messages collector. It accepts a struct with different options: socketfd of the socket to listen to, recvmmsg_vlen (vlen used on recvmmsg syscall meaning how many messages to receive on every syscall, by default 10)...
- `void *unyte_udp_queue_read(unyte_udp_queue_t *queue)` from `unyte_udp_queue.h` : read from a queue a struct with all the message buffer and metadata.
- `int unyte_udp_free_all(unyte_seg_met_t *seg)` from `unyte_udp_collector.h`: free all struct used on a message received.

Simple example of usage :
```c
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

// include installed library headers
#include <unyte-udp-notif/unyte_udp_collector.h>
#include <unyte-udp-notif/unyte_udp_utils.h>
#include <unyte-udp-notif/unyte_udp_queue.h>
#include <unyte-udp-notif/unyte_udp_defaults.h>

#define PORT "10001"
#define ADDR "192.168.0.17"

int main()
{
  // Initialize socket and bind it to the address
  int socketfd = unyte_udp_create_socket(ADDR, PORT, DEFAULT_SK_BUFF_SIZE);

  // Initialize collector options
  unyte_udp_options_t options = {0};
  // add socket fd reference to options
  options.socket_fd = socketfd
  // if argument set to 0, defaults are used
  options.recvmmsg_vlen = 0;       // vlen parameter for recvmmsg. Default: 10
  options.output_queue_size = 0;   // output queue size. Default: 1000
  options.nb_parsers = 0;          // number of parsers threads to instantiate. Default: 10
  options.socket_buff_size = 0;    // user socket buffer size in bytes. Default: 20971520 (20MB)
  options.parsers_queue_size = 0;  // parser queue size. Default: 500
  options.msg_dst_ip = false;      // destination IP not parsed from IP packet to improve performance. Default: false
  options.legacy = false;          // Use legacy UDP pub channel protocol: draft-ietf-netconf-udp-pub-channel-05. Default: false.
                                   // For legacy UDP pub channel: /!\ Used encoding types identifiers are taken from IANA.
  

  // Initialize collector
  unyte_udp_collector_t *collector = unyte_udp_start_collector(&options);

  // Example with infinity loop, change the break condition to be able to free all gracefully
  while (1)
  {
    // Read message on queue
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_udp_queue_read(collector->queue);

    // TODO: Process the UDP-notif message here
    printf("unyte_udp_get_version: %u\n", unyte_udp_get_version(seg));
    printf("unyte_udp_get_space: %u\n", unyte_udp_get_space(seg));
    printf("unyte_udp_get_encoding_type: %u\n", unyte_udp_get_encoding_type(seg));
    printf("unyte_udp_get_header_length: %u\n", unyte_udp_get_header_length(seg));
    printf("unyte_udp_get_message_length: %u\n", unyte_udp_get_message_length(seg));
    printf("unyte_udp_get_generator_id: %u\n", unyte_udp_get_generator_id(seg));
    printf("unyte_udp_get_message_id: %u\n", unyte_udp_get_message_id(seg));
    printf("unyte_udp_get_src[family]: %u\n", unyte_udp_get_src(seg)->ss_family);               // AF_INET for IPv4 or AF_INET6 for IPv6
    printf("unyte_udp_get_dest_addr[family]: %u\n", unyte_udp_get_dest_addr(seg)->ss_family);   // AF_INET for IPv4 or AF_INET6 for IPv6
    printf("unyte_udp_get_payload: %s\n", unyte_udp_get_payload(seg));
    printf("unyte_udp_get_payload_length: %u\n", unyte_udp_get_payload_length(seg));

    // Free UDP-notif message after
    unyte_udp_free_all(seg);
  }

  // To shut down the collector, just close the socket.
  close(*collector->sockfd);

  // wait for main_tread to finish
  pthread_join(*collector->main_thread, NULL);

  // Freeing collector mallocs and last messages for every queue if there is any message not consumed
  unyte_udp_free_collector(collector);

  return 0;
}
```

#### Segments data
To process the message data, all the headers, meta-data and payload are found on the struct unyte_seg_met_t defined on unyte_udp_utils.h:
```c
typedef struct unyte_segment_with_metadata
{
  unyte_metadata_t *metadata; // source/port
  unyte_header_t *header;     // UDP-notif headers
  char *payload;              // payload of message
} unyte_seg_met_t;
```
##### Getters for segments data
- `uint8_t unyte_udp_get_version(unyte_seg_met_t *message);` : encoding version
- `uint8_t unyte_udp_get_space(unyte_seg_met_t *message);` : space of encoding version
- `uint8_t unyte_udp_get_encoding_type(unyte_seg_met_t *message);` : dentifier to indicate the encoding type used for the Notification Message
- `uint16_t unyte_udp_get_header_length(unyte_seg_met_t *message);` : length of the message header in octets
- `uint16_t unyte_udp_get_message_length(unyte_seg_met_t *message);` : total length of the message within one UDP datagram, measured in octets, including the message header
- `uint32_t unyte_udp_get_generator_id(unyte_seg_met_t *message);` : observation domain id of the message
- `uint32_t unyte_udp_get_message_id(unyte_seg_met_t *message);` : message id of the message
- `struct sockaddr_storage * unyte_udp_get_src(unyte_seg_met_t *message);` : source IP and port of the message. Could be IPv4 or IPv6.
- `struct sockaddr_storage * unyte_udp_get_dest_addr(unyte_seg_met_t *message);` : collector address. Could be IPv4 or IPv6.
- `char *unyte_udp_get_payload(unyte_seg_met_t *message);` : payload buffer
- `uint16_t unyte_udp_get_payload_length(unyte_seg_met_t *message);` : payload length

#### Monitoring of the lib
There is a monitoring thread that could be started to monitor packets loss and packets received in bad order.
To activate this thread, you must initiate the monitoring thread queue size (`monitoring_queue_size`):
```c
typedef struct
{
  int socket_fd;                // socket file descriptor
  ...
  uint monitoring_queue_size;   // monitoring queue size if wanted to activate the monitoring thread. Default: 0. Recommended: 500.
  uint monitoring_delay;        // monitoring queue frequence in seconds. Default: 5 seconds
} unyte_udp_options_t;
```
The thread will every `monitoring_delay` seconds send all generators id's counters.

##### Type of threads
The threads types are defined in `monitoring_worker.h`:
- `PARSER_WORKER`: worker in charge of parsing the segments. Reassembles or saves in memory the segmented messages.
- `LISTENER_WORKER`: worker in charge of receiving the bytes from the socket. It calls `recvmmsg()` syscall to receive multiple messages at once.

##### Packets loss
Two usecases are possible monitoring packets loss:
- Drops on `PARSER_WORKER`: It means the client consuming the parsed messages is not consuming that fast. You may want to multithread the client consuming the `collector->queue` (output_queue) or increase the `output_queue_size` option to avoid packets drops on spikes.
- Drops on `LISTENER_WORKER`: It means the `N` parsers are not consuming that fast and the `LISTENER_WORKER` is pushing to the `input_queue` faster than the parsers could read. You may want to increment the number of parsers instantiated or increase `parsers_queue_size` option to avoid packets drops on spikes.

#### Legacy UDP-notif
The library can support the legacy UDP-notif protocol specified in [draft-ietf-netconf-udp-pub-channel-05](https://datatracker.ietf.org/doc/html/draft-ietf-netconf-udp-pub-channel-05).

There is an example [client_legacy_proto.c](examples/client_legacy_proto.c).

To use this legacy protocol activate the flag legacy in the collector options:
```c
typedef struct
{
  int socket_fd;            // socket file descriptor
  ...
  bool legacy;              // legacy udp-notif: draft-ietf-netconf-udp-pub-channel-05.
} unyte_udp_options_t;
```

Limitations of udp-pub-channel-05:
- Same output `struct unyte_seg_met_t` is given to the user.
- Flags from the protocol are not parsed.
- No options are possible and thus no segmentation is supported
- The encoding type identifiers are taken from the IANA instead of the draft to maintain consistency in the different pipelines. IANA codes could be checked in the main [draft](https://datatracker.ietf.org/doc/html/draft-ietf-netconf-udp-notif-04#section-9).
- Google protobuf is returned as RESERVED(0) encoding type.

### Usage of the sender
The sender allows the user to send UDP-notif protocol to a IP/port specified. It cuts the message into segments of the protocol if it is larger than the MTU specified in parameters.

The api is in `unyte_sender.h`.
The message to send have the following structure:
```c
typedef struct unyte_message
{
  uint used_mtu;              // MTU to use for cutting the message to segments
  void *buffer;               // pointer to buffer to send
  uint buffer_len;            // length of the buffer to send

  // UDP-notif
  uint8_t version : 3;        // UDP-notif protocol version
  uint8_t space : 1;          // UDP-notif protocol space
  uint8_t encoding_type : 4;  // UDP-notif protocol encoding type
  uint32_t generator_id;      // UDP-notif protocol observation domain id
  uint32_t message_id;        // UDP-notif protocol message id
} unyte_message_t;
```

Simple usage of the sender :
```c
#include <stdio.h>
#include <stdlib.h>

#include <unyte-udp-notif/unyte_sender.h>
#include <unyte-udp-notif/unyte_udp_utils.h>

#define PORT "10001"
#define ADDR "192.168.0.17"
#define MTU 1500

int main()
{
  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.default_mtu = MTU;

  // Initializing the sender --> it connect the socket to the address and port in options
  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  // pointer to the buffer to send
  char *string_to_send = "Hello world1! Hello world2! Hello world3! Hello world4! Hello world5! Hello world6! Hello world7!";
  
  // unyte message struct to send
  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  message->buffer = string_to_send;
  message->buffer_len = 97;
  // UDP-notif
  message->version = 0;
  message->space = 0;
  message->encoding_type = UNYTE_ENCODING_JSON; // json but sending string
  message->generator_id = 1000;
  message->message_id = 2147483669;
  message->used_mtu = 200; // If set to 0, the default mtu set on options is used, else, this one is used

  // Send the message
  unyte_send(sender_sk, message);

  // Freeing message and socket
  free(message);
  free_sender_socket(sender_sk);
  return 0;
}
```

### Examples
There are some samples implemented during the development of the project [here](examples).
- `client_sample.c` : simple example for minimal usage of the collector library.
- `client_monitoring.c` : sample implementing the monitoring thread to read packets statistics.
- `client_socket.c` : example using a custom socket instead of creating a new one from the library.
- `client_legacy_proto.c` : example using a collector for legacy UDP-notif protocol: draft-ietf-netconf-udp-pub-channel-05.
- `client_interface_bind_socket.c`: example using a socket bound to an interface, ip and port.
- `sender_sample.c` : simple example for minimal usage of the sender library.
- `sender_json.c` : sample reading a json file and sending the bytes by the library.
- `sender_json_bind_interface.c` : sample reading a json file and sending the bytes by the library to a specific interface.
- `sender_custom_encoding.c` : sample configurating a custom space and encoding type.
- `sender_cbor.c` : sample reading a CBOR (RFC7049) file and sending the bytes by the library.
- `eBPF/client_ebpf_user.c`: example with a custom eBPF load balancer.

## Docker
See [Docker docs](docker)

## License
See [License](LICENSE)

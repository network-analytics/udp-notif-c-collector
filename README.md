# C-Collector for UDP-notif
Library for collecting UDP-notif protocol messages.

## Build & install
To build the project and test example clients, just `make` on root folder. Il will compile with gcc all dependences and the clients.

### Installing
To install the library on a machine, run `make install` with sudo and `export.sh` without sudo. Export script will export the LD_LIBRARY_PATH on user space.
```
$ make
$ sudo make install
$ ./export.sh
```

### Uninstalling
```
$ sudo ./uninstall.sh
```
You should remove the export of the lib in your bashrc manually yourself to fully remove the lib.

## Usage
### Usage of the UDP-notif collector
The collector allows to read and parse UDP-notif protocol messages from a ip/port specified on the parameters. It allows to get directly the buffer and the metadata of the message in a struct.

The api is in `unyte_collector.h` :
- `unyte_collector_t *unyte_start_collector(unyte_options_t *options)` from `unyte_collector.h`: Initialize the UDP-notif messages collector. It accepts a struct with different options: address (the IP address to listen to), port (port to listen to), recvmmsg_vlen (vlen used on recvmmsg syscall meaning how many messages to receive on every syscall, by default 10)
- `void *unyte_queue_read(queue_t *queue)` from `queue.h` : read from a queue a struct with all the message buffer and metadata.
- `int unyte_free_all(unyte_seg_met_t *seg)` from `unyte_collector.h`: free all struct used on a message received.

Simple exemple of usage :
```
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// include installed library headers
#include <unyte-udp-notif/unyte_collector.h>
#include <unyte-udp-notif/unyte_utils.h>
#include <unyte-udp-notif/queue.h>

#define PORT 10001
#define ADDR "192.168.0.17"

int main()
{
  // Initialize collector options
  unyte_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  // if argument set to 0, defaults are used
  options.recvmmsg_vlen = 0;       // vlen parameter for recvmmsg. Default: 10
  options.output_queue_size = 0;   // output queue size. Default: 1000
  options.nb_parsers = 0;          // number of parsers threads to instantiate. Default: 10
  options.socket_buff_size = 0;    // user socket buffer size in bytes. Default: 20971520 (20MB)
  options.parsers_queue_size = 0;  // parser queue size. Default: 500

  // Initialize collector
  unyte_collector_t *collector = unyte_start_collector(&options);

  // Exemple with infinity loop, change the break condition to be able to free all gracefully
  while (1)
  {
    // Read message on queue
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);

    // TODO: Process the UDP-notif message here
    printf("get_version: %u\n", get_version(seg));
    printf("get_space: %u\n", get_space(seg));
    printf("get_encoding_type: %u\n", get_encoding_type(seg));
    printf("get_header_length: %u\n", get_header_length(seg));
    printf("get_message_length: %u\n", get_message_length(seg));
    printf("get_generator_id: %u\n", get_generator_id(seg));
    printf("get_message_id: %u\n", get_message_id(seg));
    printf("get_src_port: %u\n", get_src_port(seg));
    printf("get_src_addr: %u\n", get_src_addr(seg));
    printf("get_dest_addr: %u\n", get_dest_addr(seg));
    printf("get_payload: %s\n", get_payload(seg));
    printf("get_payload_length: %u\n", get_payload_length(seg));

    // Free UDP-notif message after
    unyte_free_all(seg);
  }

  // To shut down the collector, just close the socket.
  close(*collector->sockfd);

  // wait for main_tread to finish
  pthread_join(*collector->main_thread, NULL);

  // Free last packets in the queue
  while (is_queue_empty(collector->queue) != 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    unyte_free_all(seg);
  }

  // freeing collector mallocs
  unyte_free_collector(collector);

  return 0;
}
```

#### Segments data
To process the message data, all the headers, meta-data and payload are found on the struct unyte_seg_met_t defined on unyte_utils.h:
```
typedef struct unyte_segment_with_metadata
{
  unyte_metadata_t *metadata; // source/port
  unyte_header_t *header;     // UDP-notif headers
  char *payload;              // payload of message
} unyte_seg_met_t;
```
##### Getters for segments data
- `uint8_t get_version(unyte_seg_met_t *message);` : encoding version
- `uint8_t get_space(unyte_seg_met_t *message);` : space of encoding version
- `uint8_t get_encoding_type(unyte_seg_met_t *message);` : dentifier to indicate the encoding type used for the Notification Message
- `uint16_t get_header_length(unyte_seg_met_t *message);` : length of the message header in octets
- `uint16_t get_message_length(unyte_seg_met_t *message);` : total length of the message within one UDP datagram, measured in octets, including the message header
- `uint32_t get_generator_id(unyte_seg_met_t *message);` : observation domain id of the message
- `uint32_t get_message_id(unyte_seg_met_t *message);` : message id of the message
- `uint16_t get_src_port(unyte_seg_met_t *message);` : source port of the message
- `uint32_t get_src_addr(unyte_seg_met_t *message);` : source address of the message
- `uint32_t get_dest_addr(unyte_seg_met_t *message);` : collector address
- `char *get_payload(unyte_seg_met_t *message);` : payload buffer
- `uint16_t get_payload_length(unyte_seg_met_t *message);` : payload length

### Usage of the sender
The sender allows the user to send UDP-notif protocol to a IP/port specified. It cuts the message into segments of the protocol if it is larger than the MTU specified in parameters.

The api is in `unyte_sender.h`.
The message to send have the following structure:
```
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
```
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <unyte-udp-notif/unyte_sender.h>
#include <unyte-udp-notif/unyte_utils.h>

#define PORT 10001
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
  message->encoding_type = 1; // json but sending string
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
There are some samples implemented during the development of the project [here](samples).
- `client_sample.c` : simple example for minimal usage of the collector library.
- `sender_sample.c` : simple example for minimal usage of the sender library.
- `sender_json.c` : sample reading a json file and sending the bytes by the library.

## Docker
See [Docker docs](docker)

## License
See [License](LICENSE)

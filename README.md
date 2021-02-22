# C-Collector for UDP-notif
Library for collecting UDP-notif protocol messages.

## Build
To build the project and test example clients, just `make` on root folder. Il will compile with gcc all dependences and the clients.

## Usage of the UDP-notif collector
The api is in `unyte_collector.h` :
- `unyte_collector_t *unyte_start_collector(unyte_options_t *options)` from `unyte_collector.h`: Initialize the UDP-notif messages collector. It accepts a struct with different options: address (the IP address to listen to), port (port to listen to), recvmmsg_vlen (vlen used on recvmmsg syscall meaning how many message to receive every syscall, by default 10)
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
#include "src/unyte_collector.h"
#include "src/unyte_utils.h"
#include "src/queue.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define USED_VLEN 5

int main()
{
  // Initialize collector options
  unyte_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.recvmmsg_vlen = USED_VLEN;

  // Initialize collector
  unyte_collector_t *collector = unyte_start_collector(&options);

  // Exemple with infinity loop, change the break condition to be able to free all gracefully
  while (1)
  {
    // Read message on queue
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    // TODO: Process the UDP-notif message here

    // Free UDP-notif message after
    unyte_free_all(seg);
  }

  // To shut down the collector, just shutdown and close the socket.
  shutdown(*collector->sockfd, SHUT_RDWR);¡
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

### Segments data
To process the message data, all the headers, meta-data and payload are found on the struct unyte_seg_met_t defined on unyte_utils.h:
```
typedef struct unyte_segment_with_metadata
{
  unyte_metadata_t *metadata; // source/port
  unyte_header_t *header;     // UDP-notif headers
  char *payload;              // payload of message
} unyte_seg_met_t;
```

### Exemples
There are some samples implemented during the development of the project [here](samples).
- `client_sample.c` : simple exemple for minimal usage of the library.
- `client_performance.c` : sample to test performance of data collection
- `client_loss.c` : sample to test data loss during data collection

## Docker
See [Docker docs](docker)

## License
See [License](LICENSE)

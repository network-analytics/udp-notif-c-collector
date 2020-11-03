#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "listening_worker.h"
#include "unyte_utils.h"

#define PORT 10000

int main()
{

  /* Sample of unyte_header */
  char test[] = {0x02, 0x0c, 0x00, 0x1b, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x22, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x22, 0x3a, 0x22, 0x54, 0x6f, 0x6d, 0x22, 0x7d};

  struct unyte_segment *s = parse((char *)test);
  printHeader(&(s->header), stdout);
  printPayload(s->payload, 15, stdout);

  printf("\nStruct minimal parsing :\n\n");

  struct unyte_minimal *um = minimal_parse((char *) test);

  printf("generator id : %d\n", um->generator_id);
  printf("message id: %d\n", um->message_id);

  /* Threaded tests */

  int port = PORT;
  pthread_t listener;

  printf("\n######### Starting server #########\n\n");

  printf("Port used %d\n", port);

  /*Threaded UDP listener*/
  pthread_create(&listener, NULL, t_app, &port);
  pthread_join(listener, NULL);

  free(s);
  return 0;
}

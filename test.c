#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "listening_worker.h"
#include "unyte_utils.h"

#define PORT 10000

int main()
{

  /* Threaded tests */

  int port = PORT;
  pthread_t listener;

  printf("\n######### Starting server #########\n\n");

  printf("Port used %d\n", port);

  /*Threaded UDP listener*/
  pthread_create(&listener, NULL, t_listener, &port);
  pthread_join(listener, NULL);

  return 0;
}

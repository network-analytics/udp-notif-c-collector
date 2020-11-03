#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "listening_worker.h"
#include "unyte_utils.h"

#define PORT 10000

int main(int argc, char const *argv[])
{
  int port = PORT;
  pthread_t udpListener;

  printf("\n######### Starting server #########\n\n");

  if (argc == 2)
  {
    port = atoi(argv[1]);
  }

  printf("Port used %d\n", port);

  /*Threaded UDP listener*/
  pthread_create(&udpListener, NULL, t_app, &port);
  pthread_join(udpListener, NULL);

  return 0;
}

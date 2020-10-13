#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "udp-list.h"
#include "unyte_tools.h"

#define PORT 10000

int main(int argc, char const *argv[])
{
  int port = PORT;
  void* p = &port;
  pthread_t udpListener;

  printf("\n######### Starting server #########\n\n");

  if (argc == 2)
  {
    port = atoi(argv[1]);
  }
  /*Threaded UDP listener*/
  pthread_create(&udpListener, NULL, t_app, p);

  pthread_join(udpListener, NULL);
  return 0;
}

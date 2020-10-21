#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include "udp-list.h"
#include "hexdump.h"
#include "unyte_tools.h"

#define RCVSIZE 65565
#define RELEASE 1 /*Instant release of the socket on conn close*/

int app(int port)
{
  int release = RELEASE;
  struct sockaddr_in adresse;
  char buffer[RCVSIZE];

  struct sockaddr_in from = {0};
  unsigned int fromsize = sizeof from;

  int infinity = 1;

  /*create socket on UDP protocol*/
  int server_desc = socket(AF_INET, SOCK_DGRAM, 0);

  /*handle error*/
  if (server_desc < 0)
  {
    perror("Cannot create socket\n");
    return -1;
  }

  setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &release, sizeof(int));

  adresse.sin_family = AF_INET;
  adresse.sin_port = htons((uint16_t)port);
  /* adresse.sin_addr.s_addr = inet_addr("192.0.2.2"); */
  adresse.sin_addr.s_addr = htonl(INADDR_ANY);

  /*initialize socket*/
  if (bind(server_desc, (struct sockaddr *)&adresse, sizeof(adresse)) == -1)
  {
    perror("Bind failed\n");
    close(server_desc);
    return -1;
  }

  while (infinity)
  {
    int n;

    memset(buffer, 0, RCVSIZE);//PFR Do we need this?

    if ((n = recvfrom(server_desc, buffer, RCVSIZE - 1, 0, (struct sockaddr *)&from, &fromsize)) < 0)
    {
      perror("recvfrom failed\n");
      close(server_desc);
      return -1;
    }

    hexdump(buffer, n);
    fflush(stdout);
    /* Parse and push to the unyte_segment queue */
    struct unyte_segment *segment = parse((char *)buffer);
    printHeader(&(segment->header), stdout);

    infinity = 0;
  }

  close(server_desc);
  return 0;
}

void *t_app(void* p) {
  app((int) *((int *) p));
  pthread_exit(NULL);
}

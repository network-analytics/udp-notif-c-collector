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
#include "listening_worker.h"
#include "parsing_worker.h"
#include "hexdump.h"
#include "unyte_utils.h"
#include "queue.h"

#define RCVSIZE 65565
#define RELEASE 1 /*Instant release of the socket on conn close*/
#define PARSER_NUMBER 10
#define QUEUE_SIZE 50

struct parse_worker
{
  queue_t *queue;
  pthread_t *worker;
};

/**
 * Udp listener worker on PORT port.
 */
int app(int port)
{
  int release = RELEASE;
  struct sockaddr_in adresse;
  struct sockaddr_in from = {0};
  unsigned int fromsize = sizeof from;

  /* Iterations number if not commented in the while */
  int infinity = 1;

  /* Create parsing workers */
  struct parse_worker *parsers = malloc(sizeof(struct parse_worker) * PARSER_NUMBER);
  if (parsers == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  for (int i = 0; i < PARSER_NUMBER; i++)
  {
    (parsers + i)->queue = (queue_t *)malloc(sizeof(queue_t));
    if ((parsers + i)->queue == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    (parsers + i)->queue->head = 0;
    (parsers + i)->queue->tail = 0;
    (parsers + i)->queue->size = QUEUE_SIZE;
    (parsers + i)->queue->data = malloc(sizeof(void *) * QUEUE_SIZE);
    sem_init(&(parsers + i)->queue->empty, 0, QUEUE_SIZE);
    sem_init(&(parsers + i)->queue->full, 0, 0);
    pthread_mutex_init(&(parsers + i)->queue->lock, NULL);

    if ((parsers + i)->queue->data == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    (parsers + i)->worker = (pthread_t *)malloc(sizeof(pthread_t));
    if ((parsers + i)->worker == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    pthread_create((parsers + i)->worker, NULL, t_parser, (void *)(parsers + i)->queue);
  }

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

  /* Uncomment if no listening is wanted */
  /* infinity = 0; */

  while (infinity)
  {
    int n;

    char *buffer = (char *)malloc(sizeof(char) * RCVSIZE);
    if (buffer == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    if ((n = recvfrom(server_desc, buffer, RCVSIZE - 1, 0, (struct sockaddr *)&from, &fromsize)) < 0)
    {
      perror("recvfrom failed\n");
      close(server_desc);
      return -1;
    }

    struct unyte_minimal *seg = minimal_parse(buffer, &from, &adresse);

    /* Dispatching by modulo on threads */
    queue_write((parsers + (seg->generator_id % PARSER_NUMBER))->queue, seg);

    /* free(seg); */

    /* Comment if infinity is required */
    infinity = infinity - 1;
  }

  /* Exit threads */

  struct unyte_minimal *um = (struct unyte_minimal *)malloc(sizeof(struct unyte_minimal));
  if (um == NULL)
  {
    printf("Malloc failed.\n");
    exit(-1);
  }

  /* Not sure if this memory slot can't be overwrited somewhere after while I still want to use it */
  char *end = "exit";

  um->buffer = end;

  for (int i = 0; i < PARSER_NUMBER; i++)
  {
    queue_write((parsers + i)->queue, um);
    pthread_join(*(parsers + i)->worker, NULL);
    free((parsers + i)->worker);
    free((parsers + i)->queue->data);
    free((parsers + i)->queue);
  }

  free(parsers);
  free(um);

  close(server_desc);
  return 0;
}

/**
 * Threadified app functin listening on *P port.
 */
void *t_app(void *p)
{
  app((int)*((int *)p));
  pthread_exit(NULL);
}

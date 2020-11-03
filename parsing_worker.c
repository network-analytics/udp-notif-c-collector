#include <stdio.h>
#include <pthread.h>
#include "queue.h"

int parser(queue_t *q)
{
  int *item = (int *)queue_read(q);
  printf("Hello from thread. I received %d from queue\n", *item);
  return 0;
}

void *t_parser(void *q)
{
  parser((queue_t *)q);
  pthread_exit(NULL);
}
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../src/queue.h"

#define SIZE 10

queue_t *init()
{
  queue_t *output_queue = (queue_t *)malloc(sizeof(queue_t));
  if (output_queue == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  /* Filling queue and creating thread mem protections. */
  output_queue->head = 0;
  output_queue->tail = 0;
  output_queue->size = SIZE;
  output_queue->data = malloc(sizeof(void *) * SIZE);
  if (output_queue->data == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }
  sem_init(&output_queue->empty, 0, SIZE);
  sem_init(&output_queue->full, 0, 0);
  pthread_mutex_init(&output_queue->lock, NULL);
  return output_queue;
}

void *t_read(void *queue)
{
  queue_t *output_queue = (queue_t *)queue;
  int read = 10;
  int read_done = 0;
  while(read--) {
    void *res = unyte_queue_read(output_queue);
    if (res == NULL)
    {
      printf("RES is NULL\n");
    }
    printf("Read %d\n", read_done++);
    free(res);
  }
}

int main()
{
  queue_t *output_queue = init();

  pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
  pthread_create(thread, NULL, t_read, (void *)output_queue);

  char *buff = "Hello world!";
  int msg_send = SIZE + 1;
  while(msg_send--) {
    int ret = unyte_queue_write(output_queue, buff);
    printf("RET: %d\n", ret);
  }
  pthread_join(*thread, NULL);
  return 0;
}

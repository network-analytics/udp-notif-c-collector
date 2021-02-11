#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#ifndef H_QUEUE
#define H_QUEUE

typedef struct
{
  size_t head;
  size_t tail;
  size_t size;
  sem_t empty; /*= empty slots - sem_init(&empty, 0 , N);  // buffer vide */
  sem_t full;  /*= used slots  - sem_init(&full, 0 , 0);   // buffer vide */
  pthread_mutex_t lock;
  void **data;
} queue_t;

void *unyte_queue_read(queue_t *queue);
int unyte_queue_write(queue_t *queue, void *handle);
int is_queue_empty(queue_t *queue);

#endif

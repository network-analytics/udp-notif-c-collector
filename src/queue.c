/* A simple fifo queue (or ring buffer) in c.
This implementation \should be\ "thread safe" for single producer/consumer with atomic writes of size_t.
This is because the head and tail "pointers" are only written by the producer and consumer respectively.
Demonstrated with void pointers and no memory management.
Note that empty is head==tail, thus only QUEUE_SIZE-1 entries may be used. */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <semaphore.h>
#include "queue.h"

queue_t *unyte_queue_init(size_t size)
{
  queue_t *queue = (queue_t *)malloc(sizeof(queue_t));
  if (queue == NULL)
  {
    printf("Malloc failed.\n");
    return NULL;
  }

  /* Filling queue and creating thread mem protections. */
  queue->head = 0;
  queue->tail = 0;
  queue->size = size;
  queue->data = malloc(sizeof(void *) * size);
  if (queue->data == NULL)
  {
    printf("Malloc failed.\n");
    return NULL;
  }
  sem_init(&queue->empty, 0, size);
  sem_init(&queue->full, 0, 0);
  pthread_mutex_init(&queue->lock, NULL);
  return queue;
}

void *unyte_queue_read(queue_t *queue)
{
  // printf("unyte_queue_read: sem_wait\n");
  sem_wait(&queue->full);
  // printf("unyte_queue_read: lock\n");
  pthread_mutex_lock(&queue->lock);
  if (queue->tail == queue->head)
  {
    // pthread_mutex_unlock(&queue->lock);
    return NULL;
  }
  void *handle = queue->data[queue->tail];
  queue->data[queue->tail] = NULL;
  queue->tail = (queue->tail + 1) % queue->size;
  // printf("unyte_queue_read: %d|%d\n",queue->head, queue->tail);
  // printf("unyte_queue_read: unlock\n");
  pthread_mutex_unlock(&queue->lock);
  // printf("unyte_queue_read: sem_post\n");
  sem_post(&queue->empty);
  return handle;
}

int unyte_queue_write(queue_t *queue, void *handle)
{
  if (((queue->head + 1) % queue->size) == queue->tail)
  { // queue full
    return -2;
  }
  // printf("unyte_queue_write: sem_wait\n");
  sem_wait(&queue->empty);
  // printf("unyte_queue_write: pthread_mutex_lock\n");
  pthread_mutex_lock(&queue->lock);
  if (((queue->head + 1) % queue->size) == queue->tail)
  {
    printf("should NEVER arrive HERE\n");
    // pthread_mutex_unlock(&queue->lock);
    // sem_post(&queue->full);
    return -1;
  }
  queue->data[queue->head] = handle;
  queue->head = (queue->head + 1) % queue->size;
  // printf("unyte_queue_write: %d|%d\n",queue->head, queue->tail);
  // printf("unyte_queue_write: pthread_mutex_unlock\n");
  pthread_mutex_unlock(&queue->lock);
  // printf("unyte_queue_write: sem_post\n");
  sem_post(&queue->full);
  return 0;
}

/**
 * Check wether or not the queue is empty and return 1 for not empty 0 for empty
 */
int is_queue_empty(queue_t *queue)
{
  int val;
  int n = sem_getvalue(&queue->full, &val);
  if (n < 0)
  {
    printf("Semaphore get_value failed.\n");
    exit(EXIT_FAILURE);
  }
  if (val == 0)
  {
    return 0;
  }
  return 1;
}

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

/**
 * Init a queue with the size in argument
 */
queue_t *unyte_queue_init(size_t size);

/**
 * Read a message from a queue_t. 
 * Returns the buffer pointer void *.
 */
void *unyte_queue_read(queue_t *queue);

/**
 * Writes a buffer pointer to a queue.
 * Return 0 if message is written correctly. 
 * Return -1 if queue already full. 
 */
int unyte_queue_write(queue_t *queue, void *handle);

/**
 * Check wether or not the queue is empty.
 * Return 1 for not empty. 
 * Return 0 for empty.
 */
int is_queue_empty(queue_t *queue);

#endif

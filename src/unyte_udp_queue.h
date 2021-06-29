#ifndef H_UNYTE_UDP_QUEUE
#define H_UNYTE_UDP_QUEUE

#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct
{
  size_t head;
  size_t tail;
  size_t size;
  sem_t empty; /*= empty slots - sem_init(&empty, 0 , N);  // buffer vide */
  sem_t full;  /*= used slots  - sem_init(&full, 0 , 0);   // buffer vide */
  pthread_mutex_t lock;
  void **data;
} unyte_udp_queue_t;

/**
 * Init a queue with the size in argument
 */
unyte_udp_queue_t *unyte_udp_queue_init(size_t size);

/**
 * Read a message from a unyte_udp_queue_t. 
 * Returns the buffer pointer void *.
 */
void *unyte_udp_queue_read(unyte_udp_queue_t *queue);

/**
 * Puts the *handle to the *queue.
 * Return 0 if message is written correctly. 
 * Return -1 if queue already full dropping *handle element. 
 */
int unyte_udp_queue_write(unyte_udp_queue_t *queue, void *handle);

/**
 * Puts the element *handle to the *queue destroying the oldest element if it is full.
 * Return 0 if *handle introduced in the queue.
 * Return 1 if queue was full and oldest element from the *queue has been destroyed.
 */
int unyte_udp_queue_destructive_write(unyte_udp_queue_t *queue, void *handle);

/**
 * Check wether or not the queue is empty.
 * Return 1 for not empty. 
 * Return 0 for empty.
 */
int is_udp_queue_empty(unyte_udp_queue_t *queue);

#endif

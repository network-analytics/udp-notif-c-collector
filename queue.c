/* A simple fifo queue (or ring buffer) in c.
This implementation \should be\ "thread safe" for single producer/consumer with atomic writes of size_t.
This is because the head and tail "pointers" are only written by the producer and consumer respectively.
Demonstrated with void pointers and no memory management.
Note that empty is head==tail, thus only QUEUE_SIZE-1 entries may be used. */

#include <stdlib.h>
#include <assert.h>
#include <semaphore.h>
#include "queue.h"

void* queue_read(queue_t *queue) {
    sem_wait(&queue->full);
    pthread_mutex_lock(&queue->lock);
    if (queue->tail == queue->head) {
        return NULL;
    }
    void* handle = queue->data[queue->tail];
    queue->data[queue->tail] = NULL;
    queue->tail = (queue->tail + 1) % queue->size;
    pthread_mutex_unlock(&queue->lock);
    sem_post(&queue->empty);
    return handle;
}

int queue_write(queue_t *queue, void* handle) {
    sem_wait(&queue->empty);
    pthread_mutex_lock(&queue->lock);
    if (((queue->head + 1) % queue->size) == queue->tail) {
        return -1;
    }
    queue->data[queue->head] = handle;
    queue->head = (queue->head + 1) % queue->size;
    pthread_mutex_unlock(&queue->lock);
    sem_post(&queue->full);
    return 0;
}
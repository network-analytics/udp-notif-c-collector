#include <stdint.h>
#include <stdio.h>
#include "queue.h"
#include "unyte.h"
#include "unyte_utils.h"
#include "listening_worker.h"

/**
 * Start all the subprocesses of the collector on the given port and return the output segment queue.
 * Messages in the queues are structured in structs unyte_segment_with_metadata like defined in the 
 * unyte_utils.h file.
 */
queue_t *start_unyte_collector(uint16_t port)
{
  queue_t *output_queue = (queue_t *)malloc(sizeof(queue_t));
  if (output_queue == NULL)
  {
    printf("Malloc failed.\n");
    exit(-1);
  }

  /* Filling queue and creating thread mem protections. */
    output_queue->head = 0;
    output_queue->tail = 0;
    output_queue->size = OUTPUT_QUEUE_SIZE;
    output_queue->data = malloc(sizeof(void *) * OUTPUT_QUEUE_SIZE);
    sem_init(&output_queue->empty, 0, OUTPUT_QUEUE_SIZE);
    sem_init(&output_queue->full, 0, 0);
    pthread_mutex_init(&output_queue->lock, NULL);

  pthread_t udpListener;

  struct listener_thread_input *listener_input = (struct listener_thread_input *)malloc(sizeof(struct listener_thread_input));
  if (listener_input == NULL)
  {
    printf("Malloc failed.\n");
    exit(-1);
  }

  listener_input->port = port;
  listener_input->output_queue = output_queue;

  /*Threaded UDP listener*/
  pthread_create(&udpListener, NULL, t_listener,(void *) listener_input);

  /* Waiting for the listener to finish */
  /* pthread_join(udpListener, NULL); */

  return output_queue;
}
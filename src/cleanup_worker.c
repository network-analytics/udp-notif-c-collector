#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "cleanup_worker.h"
#include "segmentation_buffer.h"

void *t_clean_up(void *in_seg_cleanup)
{
  struct cleanup_thread_input *cleanup_in = (struct cleanup_thread_input *)in_seg_cleanup;

  struct timespec t;
  t.tv_sec = 0;
  if (cleanup_in->time > 1000)
  {
    t.tv_sec = cleanup_in->time / 1000;
  }
  t.tv_nsec = 999999 * cleanup_in->time;
  while (cleanup_in->stop_cleanup_thread == 0)
  {
    // printf("-->Setting cleanup %d %d\n",cleanup_in->time, cleanup_in->seg_buff->cleanup);
    nanosleep(&t, NULL);
    cleanup_in->seg_buff->cleanup = 1;
  }
  printf("Thread %ld: killing clean up thread\n", pthread_self());

  free(cleanup_in);
  return 0;
}

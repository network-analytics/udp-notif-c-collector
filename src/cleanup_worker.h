#ifndef H_CLEANUP_WORKER
#define H_CLEANUP_WORKER

/**
 * Threadified function launching clean up thread using cleanup_thread_input struct defined in segmentation_buffer.h.
 */
void *t_clean_up(void *in_seg_cleanup);

#endif
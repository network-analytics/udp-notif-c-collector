#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "queue.h"
#include "unyte_utils.h"
#include "parsing_worker.h"
#include "unyte_collector.h"
#include "segmentation_buffer.h"
#include "lib/hexdump.h"
#include "cleanup_worker.h"

/**
 * Assembles a message from all segments
 */
unyte_seg_met_t *create_assembled_msg(char *complete_msg, unyte_seg_met_t *src_parsed_segment, uint16_t total_payload_byte_size)
{
  // Create a new unyte_seg_met_t to push to queue
  unyte_seg_met_t *parsed_msg = (unyte_seg_met_t *)malloc(sizeof(unyte_seg_met_t));
  parsed_msg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));
  parsed_msg->metadata = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));

  copy_unyte_seg_met_headers(parsed_msg, src_parsed_segment);

  // Rewrite header length and message length
  parsed_msg->header->header_length = HEADER_BYTES;
  parsed_msg->header->message_length = total_payload_byte_size + HEADER_BYTES;

  copy_unyte_seg_met_metadata(parsed_msg, src_parsed_segment);

  parsed_msg->payload = complete_msg;
  return parsed_msg;
}

/**
 * Parser that receive unyte_minimal structs stream from the Q queue.
 * It transforms it into unyte_segment_with_metadata structs and push theses to the OUTPUT queue.
 */
int parser(struct parser_thread_input *in)
{
  struct segment_buffer *segment_buff = in->segment_buff;
  int max_malloc_errs = 3;
  while (1)
  {
    unyte_min_t *queue_data = (unyte_min_t *)unyte_queue_read(in->input);

    // Can do better
    unyte_seg_met_t *parsed_segment = parse_with_metadata(queue_data->buffer, queue_data);

    /* Unyte_minimal struct is not useful anymore */
    free(queue_data->buffer);
    free(queue_data);

    // Not fragmented message
    if (parsed_segment->header->header_length <= HEADER_BYTES)
    {
      unyte_queue_write(in->output, parsed_segment);
    }
    // Fragmented message
    else
    {
      int insert_res = insert_segment(segment_buff,
                                      parsed_segment->header->generator_id,
                                      parsed_segment->header->message_id,
                                      parsed_segment->header->f_num,
                                      parsed_segment->header->f_last,
                                      parsed_segment->header->message_length - parsed_segment->header->header_length,
                                      parsed_segment->payload);
      if (insert_res == -1)
      {
        // Content was already present for this seqnum (f_num)
        continue;
      }
      else if (insert_res == -3)
      {
        // malloc error: try 3 mallocs, if error break while
        max_malloc_errs--;
        if (max_malloc_errs <= 0)
          return -1;
      }
      // completed message
      if (insert_res == 1 || insert_res == -2)
      {
        struct message_segment_list_cell *msg_seg_list = get_segment_list(segment_buff, parsed_segment->header->generator_id, parsed_segment->header->message_id);
        char *complete_msg = reassemble_payload(msg_seg_list);

        unyte_seg_met_t *parsed_msg = create_assembled_msg(complete_msg, parsed_segment, msg_seg_list->total_payload_byte_size);

        unyte_queue_write(in->output, parsed_msg);
        clear_segment_list(segment_buff, parsed_segment->header->generator_id, parsed_segment->header->message_id);
        segment_buff->count--;
      }

      if (segment_buff->cleanup == 1 && segment_buff->count > CLEAN_COUNT_MAX)
      {
        cleanup_seg_buff(segment_buff);
      }

      fflush(stdout);

      // free unused partial parsed_segment without payload
      unyte_free_header(parsed_segment);
      unyte_free_metadata(parsed_segment);
      free(parsed_segment);
    }
  }
  return 0;
}

/**
 * Threaded parser function.
 */
void *t_parser(void *in)
{
  ((struct parser_thread_input *)in)->parser_thread_id = pthread_self();
  parser((struct parser_thread_input *)in);

  // seg_cleanup->stop_cleanup = 1;
  //FIXME: never arrives here --> maybe now it arrives here when malloc returns null
  // free(clean_up_thread);
  // free(seg_cleanup->seg_buff);
  // free(seg_cleanup);
  // free(clean_up_thread);
  printf("Stopping t_parser %ld\n", pthread_self());
  pthread_exit(NULL);
}

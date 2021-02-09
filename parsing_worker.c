#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "queue.h"
#include "unyte_utils.h"
#include "parsing_worker.h"
#include "unyte.h"
#include "segmentation_buffer.h"
#include "hexdump.h"

/**
 * Parser that receive unyte_minimal structs stream from the Q queue.
 * It transforms it into unyte_segment_with_metadata structs and push theses to the OUTPUT queue.
 */
int parser(struct parser_thread_input *in)
{
  struct segment_buffer *segment_buff = in->segment_buff;
  while (1)
  {
    /* char *segment = (char *)unyte_queue_read(q); */
    unyte_min_t *queue_data = (unyte_min_t *)unyte_queue_read(in->input);

    /* Can do better */
    unyte_seg_met_t *parsed_segment = parse_with_metadata(queue_data->buffer, queue_data);
    // printf("From %d | %d\n", parsed_segment->metadata->src_addr, parsed_segment->metadata->src_port);

    /* Unyte_minimal struct is not useful anymore */
    free(queue_data->buffer);
    free(queue_data);
    /* Check about fragmentation */

    // printf("count:%ld| %d\n",in->thread_id, segment_buff->count);
    if (parsed_segment->header->header_length <= 12)
    {
      unyte_queue_write(in->output, parsed_segment);
    }
    else
    {
      // struct segment_buffer *buf, uint32_t gid, uint32_t mid, uint32_t seqnum, int last, uint32_t payload_size, void *content
      int insert_res = insert_segment(segment_buff,
                                      parsed_segment->header->generator_id,
                                      parsed_segment->header->message_id,
                                      parsed_segment->header->f_num,
                                      parsed_segment->header->f_last,
                                      parsed_segment->header->message_length - parsed_segment->header->header_length,
                                      parsed_segment->payload);
      // printf("Result: %d\n", insert_res);
      // completed message
      if (insert_res == 1 || insert_res == -2)
      {
        struct message_segment_list_cell *msg_seg_list = get_segment_list(segment_buff, parsed_segment->header->generator_id, parsed_segment->header->message_id);
        // print_segment_list_header(msg_seg_list);
        // print_segment_list_string(msg_seg_list);
        char *complete_msg = (char *)malloc(msg_seg_list->total_payload_byte_size);
        // memset(complete_msg,0,msg_seg_list->total_payload_byte_size);
        char *msg_tmp = complete_msg;
        struct message_segment_list_cell *temp = msg_seg_list;
        // printf("equalf %d\n", '\0'==0);
        while (temp->next != NULL)
        {
          // printf("Copying %d|%d\n", msg_seg_list->total_payload_byte_size, temp->next->content_size);
          memcpy(msg_tmp, temp->next->content, temp->next->content_size);
          // hexdump(msg_tmp,temp->next->content_size - parsed_segment->header->header_length);
          msg_tmp += temp->next->content_size;
          temp = temp->next;
        }

        // printf("Hello: %s||\n", complete_msg);
        unyte_seg_met_t *parsed_msg = (unyte_seg_met_t *)malloc(sizeof(unyte_seg_met_t));
        // parsed_msg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));
        // parsed_msg->metadata = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));

        parsed_msg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));
        parsed_msg->metadata = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));

        parsed_msg->header->header_length = 12; //sans options à mettre en constant
        parsed_msg->header->message_length = msg_seg_list->total_payload_byte_size + 12;
        parsed_msg->header->generator_id = parsed_segment->header->generator_id;
        parsed_msg->header->message_id = parsed_segment->header->message_id;
        parsed_msg->header->space = parsed_segment->header->space;
        parsed_msg->header->encoding_type = parsed_segment->header->encoding_type;
        parsed_msg->header->version = parsed_segment->header->version;

        parsed_msg->metadata->collector_addr = parsed_segment->metadata->collector_addr;
        parsed_msg->metadata->src_addr = parsed_segment->metadata->src_addr;
        parsed_msg->metadata->src_port = parsed_segment->metadata->src_port;

        parsed_msg->payload = complete_msg;

        unyte_queue_write(in->output, parsed_msg);
        clear_segment_list(segment_buff, parsed_segment->header->generator_id, parsed_segment->header->message_id);

        segment_buff->count--;
      }
      //TODO: if cleanup == 1 and count > 1000 --> clean up & set cleanup=0
      //TODO: clean up = set value ++ or destroy
      if (segment_buff->cleanup == 1 && segment_buff->count > 3) {
        // printf("%d|%d\n", segment_buff->cleanup, segment_buff->count);
        cleanup_seg_buff(segment_buff);
      }
      // printf("parsing_worker: segmented packet.\n");
      fflush(stdout);
      /* TODO: Discarding the segment while fragmentation is not fully implemented.*/
      unyte_free_header(parsed_segment);
      unyte_free_metadata(parsed_segment);
      free(parsed_segment);
    }
  }
  printf("Ended");
  return 0;
}

/**
 * Threaded parser function.
 */
void *t_parser(void *in)
{
  //TODO: change architecture: listening_worker launch both thread and not one after another
  ((struct parser_thread_input *)in)->thread_id = pthread_self();

  pthread_t *clean_up_thread = (pthread_t *)malloc(sizeof(pthread_t));
  struct segment_cleanup *seg_cleanup = (struct segment_cleanup *)malloc(sizeof(struct segment_cleanup));
  seg_cleanup->seg_buff = ((struct parser_thread_input *)in)->segment_buff;
  seg_cleanup->time = 1000;
  pthread_create(clean_up_thread, NULL, t_clean_up, (void *)seg_cleanup);
  ((struct parser_thread_input *)in)->clean_up_thread = clean_up_thread;
  ((struct parser_thread_input *)in)->seg_cleanup_in = seg_cleanup;

  parser((struct parser_thread_input *)in);

  // seg_cleanup->stop_cleanup = 1;
  //FIXME: never arrives here
  // free(clean_up_thread);
  free(seg_cleanup->seg_buff);
  free(seg_cleanup);
  // free(clean_up_thread);
  printf("Stopping t_parser %ld\n", pthread_self());
  pthread_exit(NULL);
}

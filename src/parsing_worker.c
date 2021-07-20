#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "unyte_udp_utils.h"
#include "parsing_worker.h"
#include "unyte_udp_collector.h"
#include "cleanup_worker.h"

/**
 * Assembles a message from all segments.
 * Returns NULL if malloc failed or complete_msg is NULL
 */
unyte_seg_met_t *create_assembled_msg(char *complete_msg, unyte_seg_met_t *src_parsed_segment, uint16_t total_payload_byte_size)
{
  // if assembled message is null due to malloc failed
  if (complete_msg == NULL)
    return NULL;

  // Create a new unyte_seg_met_t to push to queue
  unyte_seg_met_t *parsed_msg = (unyte_seg_met_t *)malloc(sizeof(unyte_seg_met_t));
  if (parsed_msg == NULL)
    return NULL;

  parsed_msg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));
  parsed_msg->metadata = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));

  if (parsed_msg->header == NULL || parsed_msg->metadata == NULL)
  {
    if (parsed_msg->header != NULL)
      free(parsed_msg->header);
    if (parsed_msg->metadata != NULL)
      free(parsed_msg->metadata);
    free(parsed_msg);
    return NULL;
  }

  parsed_msg->metadata->src = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
  if (parsed_msg->metadata->src == NULL)
  {
    free(parsed_msg->metadata);
    free(parsed_msg->header);
    free(parsed_msg);
    return NULL;
  }

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
  unyte_seg_counters_t *counters = in->counters;
  while (1)
  {
    void *queue_bef = unyte_udp_queue_read(in->input);
    if (queue_bef == NULL)
    {
      printf("Error reading from queue\n");
      fflush(stdout);
      continue;
    }
    unyte_min_t *queue_data = (unyte_min_t *)queue_bef;
    unyte_seg_met_t *parsed_segment = parse_with_metadata(queue_data->buffer, queue_data);

    /* Unyte_minimal struct is not useful anymore */
    free(queue_data->buffer);
    free(queue_data);

    // Not segmented message
    if (parsed_segment->header->header_length <= HEADER_BYTES)
    {
      uint32_t gid = parsed_segment->header->generator_id;
      uint32_t mid = parsed_segment->header->message_id;
      int ret = unyte_udp_queue_write(in->output, parsed_segment);
      // ret == -1 queue already full, segment discarded
      if (ret < 0)
      {
        if (in->monitoring_running)
          unyte_udp_update_dropped_segment(counters, gid, mid);
        // printf("2.losing message on output queue\n");
        //TODO: syslog drop package + count
        unyte_udp_free_all(parsed_segment);
      }
      else if (in->monitoring_running)
      {
        unyte_udp_update_received_segment(counters, gid, mid);
      }
    }
    // Segmented message
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
        unyte_udp_free_all(parsed_segment);
        continue;
      }
      else if (insert_res == -3)
      {
        // malloc error: try 3 mallocs, if error break while
        max_malloc_errs--;
        if (max_malloc_errs <= 0)
        {
          unyte_udp_free_all(parsed_segment);
          return -1;
        }
      }
      // completed message
      if (insert_res == 1 || insert_res == -2)
      {
        struct message_segment_list_cell *msg_seg_list = get_segment_list(segment_buff, parsed_segment->header->generator_id, parsed_segment->header->message_id);
        char *complete_msg = reassemble_payload(msg_seg_list);

        unyte_seg_met_t *parsed_msg = create_assembled_msg(complete_msg, parsed_segment, msg_seg_list->total_payload_byte_size);
        if (parsed_msg == NULL)
        {
          printf("Malloc failed assembling message\n");
          goto clear_and_cleanup_buffer;
        }
        int ret = unyte_udp_queue_write(in->output, parsed_msg);
        // ret == -1 queue is full, we discard the message
        if (ret < 0)
        {
          if (in->monitoring_running)
            unyte_udp_update_dropped_segment(counters, parsed_segment->header->generator_id, parsed_segment->header->message_id);
          // printf("3.losing message on output queue\n");
          //TODO: syslog drop package + count
          unyte_udp_free_all(parsed_msg);
        }
        else if (in->monitoring_running)
        {
          unyte_udp_update_received_segment(counters, parsed_segment->header->generator_id, parsed_segment->header->message_id);
        }

      clear_and_cleanup_buffer:
        clear_segment_list(segment_buff, parsed_segment->header->generator_id, parsed_segment->header->message_id);
        segment_buff->count--;
      }

      if (segment_buff->cleanup == 1 && segment_buff->count > CLEAN_COUNT_MAX)
      {
        cleanup_seg_buff(segment_buff, CLEAN_UP_PASS_SIZE);
      }

      fflush(stdout);

      // free unused partial parsed_segment without payload
      unyte_udp_free_header(parsed_segment);
      unyte_udp_free_metadata(parsed_segment);
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
  pthread_t id = pthread_self();
  ((struct parser_thread_input *)in)->parser_thread_id = id;
  ((struct parser_thread_input *)in)->counters->thread_id = id;

  parser((struct parser_thread_input *)in);

  // seg_cleanup->stop_cleanup = 1;
  //FIXME: never arrives here --> maybe now it arrives here when malloc returns null
  // free(clean_up_thread);
  // free(seg_cleanup->seg_buff);
  // free(seg_cleanup);
  // free(clean_up_thread);
  printf("Stopping t_parser %ld\n", id);
  pthread_exit(NULL);
}

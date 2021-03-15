#ifndef H_SEGMENTATION_BUFFER
#define H_SEGMENTATION_BUFFER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "unyte_udp_utils.h"

// Segmentation_buffer parameters
#define SEG_BUF_SIZE 10000      // size of collision list 
#define CLEAN_UP_PASS_SIZE 500  // number of iterations to clean up
#define CLEAN_COUNT_MAX 50      // clean up segment buffer when count > CLEAN_COUNT_MAX
#define EXPIRE_MSG 3            // seconds to consider the segmented message in the buffer expired (not receiving segments anymore)

/**
 * total_size, current_size, gid, mid, only relevant for header cell
 * seqnum, content only relevant for non header cell
 */
struct message_segment_list_cell
{
  uint32_t total_size;              // total segments for a complete message, 0 if unknown
  uint32_t current_size;            // present segments in a list
  uint32_t total_payload_byte_size; // sum of segments size for this segment list
  uint32_t gid;                     // generator_id, observation_domain_id in scapy
  uint32_t mid;                     // message_id, segment_id in scapy
  uint32_t seqnum;                  // ordered by seqnum
  void *content;
  uint32_t content_size;
  struct message_segment_list_cell *next;
  int to_clean_up;                  // 0 = not passed yet; 1 = cleaner passed once;
  time_t timestamp;
};

/**
 * gid, id, seglist  only relevant for non header cell
 */
struct collision_list_cell
{
  uint32_t gid;
  uint32_t mid;
  struct message_segment_list_cell *head;
  struct collision_list_cell *next;
};

struct segment_buffer
{
  uint32_t count;
  struct collision_list_cell *hash_array[SEG_BUF_SIZE];
  uint8_t cleanup;                // 1 should clean up
  uint8_t cleanup_start_index;
};

struct cleanup_thread_input
{
  struct segment_buffer *seg_buff;
  int time;                         // clean up every <time> milliseconds
  uint8_t stop_cleanup_thread;      // flag: 1 to kill cleanup thread
};

/**
 * Create a segment buffer to store UDP-notif message segments
 */
struct segment_buffer *create_segment_buffer(uint buff_size);

/**
 * Clear a buffer of any collision list, collision list header, and clear the buffer itself
 */
int clear_buffer(struct segment_buffer *buf);

/**
 * Reassembles the payload from a segment_list
 */
char *reassemble_payload(struct message_segment_list_cell *);

/**
 * Insert a message segment inside a segment buffer 
 * inserts a content in a message segment list, maintaining order among the content based on seqnum.
 * head must be the empty header cell of a message_segment_list, cannot be NULL
 * seqnum is the sequence number of that segment
 * last is 1 if the segment is the last one of the content, 0 otherwise
 * content is the value to be added at the corresponding seqnum
 * 
 * returns 0 if the content was added and message is not complete yet
 * returns 1 if the content was added and message is complete
 * (a message was stored with last 1, and the length of the list is equal to the total number of segments)
 * returns -1 if a content was already present for this seqnum
 * returns -2 if a content was already present and message is complete
 * returns -3 if a memory allocation failed
*/
int insert_segment(struct segment_buffer *buf, uint32_t gid, uint32_t mid, uint32_t seqnum, int last, uint32_t payload_size, void *content);

// Segment buffer management

/**
 * Retrieve the header cell of a segment list for a given message 
 */
struct message_segment_list_cell *get_segment_list(struct segment_buffer *buf, uint32_t gid, uint32_t mid);
/**
 * Clear a list of segments
 */
int clear_segment_list(struct segment_buffer *buf, uint32_t gid, uint32_t mid);
/**
 * Hash function used on segmentation buffer hashmap
 * uint32_t gid: generator id / observation domain id
 * uint32_t mid: message id
 */
uint32_t hashKey(uint32_t gid, uint32_t mid);

// Print functions
void print_segment_list_header(struct message_segment_list_cell *head);
void print_segment_list_int(struct message_segment_list_cell *head);
void print_segment_buffer_int(struct segment_buffer *buf);
void print_segment_list_string(struct message_segment_list_cell *head);

/**
 * Adds a message segment in a list of segments of a given message
 */
int insert_into_msl(struct message_segment_list_cell *head, uint32_t seqnum, int last, uint32_t payload_size, void *content);
/**
 * Destroys a message
 */
void clear_msl(struct message_segment_list_cell *head);

/**
 * Initialise a message segment list 
 */
struct message_segment_list_cell *create_message_segment_list(uint32_t gid, uint32_t mid);

/**
 * Clean segmentation buffer messages.
 * Sets flag of a message to "read" to be removed next iteration cleanup.
 * Cleans up the "read" messages by <cleanup_pass_size> size.
 */
void cleanup_seg_buff(struct segment_buffer *buf, int cleanup_pass_size);

#endif
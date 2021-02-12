#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "unyte_utils.h"

// Segmentation_buffer parameters
#define SIZE_BUF 10000           // size of buffer
#define CLEAN_UP_PASS_SIZE 1000  // number of iterations to clean up
#define CLEAN_COUNT_MAX 5        // clean up segment buffer when count > CLEAN_COUNT_MAX

/**
total_size, current_size, gid, mid, only relevant for header cell
seqnum, content only relevant for non header cell
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
};

/**
gid, id, seglist  only relevant for non header cell
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
  struct collision_list_cell *hash_array[SIZE_BUF];
  uint8_t cleanup; //1 il faut clean up
  uint8_t cleanup_start_index;
};

struct cleanup_thread_input
{
  struct segment_buffer *seg_buff;
  int time;                         // clean up every <time> milliseconds
  uint8_t stop_cleanup_thread;      // flag: 1 to kill cleanup thread
};

//Create a segment buffer to store UDP-notif message segments
struct segment_buffer *create_segment_buffer();
//Clear a buffer of any collision list, collision list header, and clear the buffer itself
int clear_buffer(struct segment_buffer *buf);

// Reassembles the payload from a segment_list
char *reassemble_payload(struct message_segment_list_cell *);

/*insert a message segment inside a segment buffer */
/**
inserts a content in a message segment list, maintaining order among the content based on seqnum
head must be the empty header cell of a message_segment_list, cannot be NULL
seqnum is the sequence number of that segment
last is 1 if the segment is the last one of the content, 0 otherwise
content is the value to be added at the corresponding seqnum
returns 0 if the content was added and message is not complete yet
returns 1 if the content was added and message is complete
(a message was stored with last 1, and the length of the list is equal to the total number of segments)
returns -1 if a content was already present for this seqnum
returns -2 if a content was already present and message is complete
returns -3 if a memory allocation failed
*/
int insert_segment(struct segment_buffer *buf, uint32_t gid, uint32_t mid, uint32_t seqnum, int last, uint32_t payload_size, void *content);

/*segment buffer management*/
/*Retrieve the header cell of a segment list for a given message */
struct message_segment_list_cell *get_segment_list(struct segment_buffer *buf, uint32_t gid, uint32_t mid);
/*clear a list of segments*/
int clear_segment_list(struct segment_buffer *buf, uint32_t gid, uint32_t mid);
uint32_t hashKey(uint32_t gid, uint32_t mid);

void print_segment_list_header(struct message_segment_list_cell *head);
void print_segment_list_int(struct message_segment_list_cell *head);
void print_segment_buffer_int(struct segment_buffer *buf);
void print_segment_list_string(struct message_segment_list_cell *head);

/*adds a message segment in a list of segments of a given message*/
int insert_into_msl(struct message_segment_list_cell *head, uint32_t seqnum, int last, uint32_t payload_size, void *content);
/*destroys a message*/
void clear_msl(struct message_segment_list_cell *head);

/*initialise a message segment list */
struct message_segment_list_cell *create_message_segment_list(uint32_t gid, uint32_t mid);

void cleanup_seg_buff(struct segment_buffer *buf);

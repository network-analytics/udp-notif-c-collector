#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "unyte_utils.h"

#define SIZE_BUF 10000 // size of buffer

/**
total_size, current_size, gid, mid, only relevant for header cell
seqnum, content only relevant for non header cell
*/
struct message_segment_list_cell {
    uint32_t total_size;
    uint32_t current_size;
    uint32_t gid;
    uint32_t mid;
    uint32_t seqnum;
    void*    content;
    struct message_segment_list_cell* next;
};

/**
gid, id, seglist  only relevant for non header cell
*/
struct collision_list_cell{
    uint32_t gid;
    uint32_t mid;
    struct message_segment_list_cell* head;
    struct collision_list_cell* next; 
};

struct segment_buffer {
  uint32_t count;//TODO
  struct collision_list_cell* hash_array[SIZE_BUF];
};

//Create a segment buffer to store UDP-notif message segments
struct segment_buffer* create_segment_buffer();

/*segment buffer management*/
/*Retrieve the header cell of a segment list for a given message */
int get_segment_list(struct segment_buffer* buf, uint32_t gid, uint32_t  mid);
/*clear a list of segments*/
int clear_segment_list(struct segment_buffer* buf, uint32_t gid, uint32_t  mid);
/*insert a message segment inside a segment buffer */
int insert_segment(struct segment_buffer* buf, uint32_t gid, uint32_t mid, uint32_t seqnum, int last, void* content);

uint32_t hashKey(uint32_t gid, uint32_t mid);

void print_segment_list_header(struct message_segment_list_cell* head);
void print_segment_list_int(struct message_segment_list_cell* head);
void print_segment_buffer_int(struct segment_buffer* buf);

/*adds a message segment in a list of segments of a given message*/
int insert_into_msl(struct message_segment_list_cell* head, uint32_t seqnum, int last, void* content);
/*destroys a message*/
void clear_msl(struct message_segment_list_cell* head);

/*initialise a message segment list */
struct message_segment_list_cell* create_message_segment_list(uint32_t gid, uint32_t mid);

/*struct table_item* search(uint32_t gid, uint32_t mid, struct segment_buffer* buf); 
// adds a message based on its generator_id and message_id
uint32_t insert(uint32_t gid, uint32_t mid, struct segment_buffer* buf, struct unyte_segment_with_metadata* seg); 
struct table_item* delete(uint32_t gid, uint32_t mid, struct segment_buffer* buf); 
void *add(struct segment_buffer* main_table, struct unyte_segment_with_metadata* segment); 
*/

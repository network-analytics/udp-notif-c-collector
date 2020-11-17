#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "unyte.h"
#include "unyte_utils.h"
#include "queue.h"

#define SIZE_BUF 10000 // buffer size
#define CAPA_BUF 10 // buffer collision capacity
#define NUMB_SEG 20 // number of segments

#define SIZE_MSG 20 // message_buffer size
#define CAPA_MSG 20 // collision capacity for a given message_id
#define SIZE_SEG 20 // Maximum number of segments that can be stored in the segments array of a Message struct

struct table_item {
	int key;
	void *data; //Data can be a segment_buffer or a Message
};

struct segment_buffer {
	int count;
}

struct segment_buffer {
	int total_size;
	int current_size;
	void *segments[SEGN];
};

struct segment_buffer* main_table;
struct table_item* item;

int hashCode(int key) {
	return key % SIZE;
}

struct table_item* search(int key, struct table_item* hash_array[SIZE]) {
	//get the hash 
	int index = hashCode(key);
	
	//move in array until an empty 
	while(hash_array[index] != NULL) {
	
		if(hash_array[index]->key == key) {
			return &hash_array[index];
		}
			
		//go to next cell
		++index;
		
		//wrap around the table
		index %= SIZE;
	}		
	
	return NULL;
}

// adds a message base on its generator_id and message_id
void insert_message(struct segment_buffer* main_table, struct unyte_segment_with_metadata* segment) {

	// find the hash table for this generator id
	struct segment_buffer* generator_id_table = search(segment->header->generator_id, main_table->content)
	// if no hash table, create it
	// we have a pointer towards the possibly new hash table
	// search in this hashtable (of the current genID) the hash table for this message id
	// if nonexistent, create it
	// insert data at index f_num in the array of this message

	// TODO : accounting
	
	

	struct table_item *item = (struct table_item*) malloc(sizeof(struct table_item));
	item->data = data;
	item->key = key;

	//get the hash 
	int index = hashCode(key);

	//move in array until an empty or deleted cell
	while(hash_array[index] != NULL && hash_array[index]->key != -1) {
		
		//go to next cell
		++index;
		
		//wrap around the table
		index %= SIZE;
	}
	
	hash_array[index] = item;
}

struct table_item* delete(struct table_item* item, struct table_item* hash_array[SIZE]) {
	int key = item->key;

	//get the hash 
	int index = hashCode(key);

	//move in array until an empty
	while(hash_array[index] != NULL) {
	
		if(hash_array[index]->key == key) {
			struct table_item* temp = hash_array[index]; 
			//assign idle values at deleted position
			//hash_array[index]->key = -1;
			memset(hash_array[index]->key*, 0, SEGN);
			//hash_array[index]->data = -1;
			memset(hash_array[index]->data*, 0, SEGN);
			return temp;
		}
		//go to next cell
		++index;
		//wrap around the table
		index %= SIZE;
	}		
	
	return NULL;
}

void display(struct table_item* hash_array[SIZE]) {
	int i = 0;
	
	for(i = 0; i<SIZE; i++) {
	
		if(hash_array[i] != NULL)
			printf(" (%d,%d)",hash_array[i]->key,hash_array[i]->data);
		else
			printf(" ~~ ");
	}
	printf("\n");
}

/*struct unyte_header
{
  uint8_t version : 3;
  uint8_t space : 1;
  uint8_t encoding_type : 4;
  uint8_t header_length : 8;
  uint16_t message_length;
  uint32_t generator_id;
  uint32_t message_id;
  
  uint8_t f_type;
  uint8_t f_len;
  uint32_t f_num : 31;
  uint8_t f_last : 1;
};

struct unyte_segment
{
  struct unyte_header header;
  char *payload;
};

struct unyte_segment_with_metadata
{
  uint16_t src_port;           
  unsigned long src_addr;           
  unsigned long collector_addr;     

  struct unyte_header header;
  char *payload;
};*/

//main_table is a ref to the segment_buffer of all generator_id-specific segment_buffers
void *add(struct segment_buffer* main_table, struct unyte_segment_with_metadata* segment) {
	struct segment_buffer* generator_id_table;
	generator_id_table->content* = search(segment->header->generator_id, main_table->content);
	if (generator_id_table == NULL) {
		generator_id_table = (struct segment_buffer*) malloc(sizeof(struct segment_buffer));
		// TROUBLE
		insert_message(segment->header->generator_id, generator_id_table->content, main_table->content);
		// TROUBLE
	}
	//We know that generator_id_table is a ref to the segment_buffer of all messages of this genID
	
 //	insert(generator_id_table, seg.messageID, seg)
 	struct segment_buffer* message;
	message->segments* = search(segment->header->message_id, generator_id_table->content);
	if (message == NULL) {
		// first segment ever received for this message_id and this generator_id
		message->segments[segment->header->f_num] = segment;
		message->current_size++;
	}
	// message is the array that contains all segments for this message_id and this generator_id
	message->segments[segment->header->f_num] = &segment;
	if (message->segments[segment->header->f_num]->f_last == 1) {
		message->total_size = segment->header->f_num + 1;
	}
	if (message->current_size == message->total_size) { // as current size is incremented with the arrival of the first packet, this can only be true if total size differs from 0
		return &message;
		// free table
	} else return 0;
}

//CAREFUL ABOUT ALL USES OF SEQPKT : STRUCT UNKNOWN FOR NOW

int main() {

	main_table = (struct segment_buffer*) malloc(sizeof(struct segment_buffer));

	insert(19, 20, main_table->content);
	insert(42, 80, main_table->content);
	insert(12, 44, main_table->content);
	insert(13, 78, main_table->content);
	insert(37, 97, main_table->content);

	display(main_table->content);
	item = search(37, main_table->content);
	
	if(item != NULL) {
		printf("Element found: %d\n", item->data);
	} else {
		printf("Element not found\n");
	}

	delete(item, main_table->content);
	item = search(37, main_table->content);
	
	if(item != NULL) {
		printf("Element found: %d\n", item->data);
	} else {
		printf("Element not found\n");
	}
}

//si L bit set, total_size = franum + 1 et si current_size == total size, renvoyer un pointeur vers une struct contenant current et total size + tableau complet de segments
//libérer la place des segment_buffer s'il n'y a plus de segments pour un message id ou plus de messages pour un gen id
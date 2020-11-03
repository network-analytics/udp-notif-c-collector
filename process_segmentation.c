#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 20 // HashTable "length"
#define SEGN 100 // Maximum number of segments that can be stored in the segments array of a Message struct

struct Message {
	int totalSize;
	int currentSize;
	int segments[SEGN]; //Array of segments
};

struct TableItem {
	int key;
	int data;
	//void *data; Data can be a HashTable or a Message
};

struct HashTable {
	struct TableItem* content[SIZE];
};

struct HashTable* table;

struct TableItem* dummyItem;
struct TableItem* item;

int hashCode(int key) {
	return key % SIZE;
}

struct TableItem *search(int key, struct TableItem* hashArray[SIZE]) {
	//get the hash 
	int hashIndex = hashCode(key);
	
	//move in array until an empty 
	while(hashArray[hashIndex] != NULL) {
	
		if(hashArray[hashIndex]->key == key)
			return hashArray[hashIndex]; 
			
		//go to next cell
		++hashIndex;
		
		//wrap around the table
		hashIndex %= SIZE;
	}		
	
	return NULL;		
}

void insert(int key,int data, struct TableItem* hashArray[SIZE]) {

	struct TableItem *item = (struct TableItem*) malloc(sizeof(struct TableItem));
	item->data = data;
	item->key = key;

	//get the hash 
	int hashIndex = hashCode(key);

	//move in array until an empty or deleted cell
	while(hashArray[hashIndex] != NULL && hashArray[hashIndex]->key != -1) {
		
		//go to next cell
		++hashIndex;
		
		//wrap around the table
		hashIndex %= SIZE;
	}
	
	hashArray[hashIndex] = item;
}

struct TableItem* delete(struct TableItem* item, struct TableItem* hashArray[SIZE]) {
	int key = item->key;

	//get the hash 
	int hashIndex = hashCode(key);

	//move in array until an empty
	while(hashArray[hashIndex] != NULL) {
	
		if(hashArray[hashIndex]->key == key) {
			struct TableItem* temp = hashArray[hashIndex]; 
			
			//assign idle values at deleted position
			hashArray[hashIndex]->key = -1;
			hashArray[hashIndex]->data = -1;
			return temp;
		}
		
		//go to next cell
		++hashIndex;
		
		//wrap around the table
		hashIndex %= SIZE;
	}		
	
	return NULL;
}

void display(struct TableItem* hashArray[SIZE]) {
	int i = 0;
	
	for(i = 0; i<SIZE; i++) {
	
		if(hashArray[i] != NULL)
			printf(" (%d,%d)",hashArray[i]->key,hashArray[i]->data);
		else
			printf(" ~~ ");
	}
	printf("\n");
}

//table is a ref to the hashtable of all genIDs
void *add(HashTable* table, UDPNotifPacket segPkt){
	struct HashTable* genIDtable;
	genIDtable = search(table, segPkt.genID)
	if genIDtable == NULL {
		genIDtable = (struct HashTable*) malloc(sizeof(struct HashTable));
		insert(table, segPkt.genID, genIDtable);
	}
	//We know that genIDtable is a ref to the hashtable of all messages of this genID
	
 //	insert(genIDtable, segPkt.msgID, segPkt)
 	struct Message* message;
	message = search(genIDtable, segPkt.msgID)
	if (message == NULL) {
		//First segment ever received for that messageID for that genID
		message->segments[segPkt.seqNum] = segPkt;
		message->curentSize++;
	}
	//We know that message is the struct for this genID and this messageID
	message->segments[segPkt.seqNum] = &segPkt;
	if message->segments[segPkt.seqNum].L == 1 {
		message->totalSize = segPkt.seqNum + 1;
	}
	if message->currentSize == message->totalSize { // As current size is incremented with the arrival of the first packet, this can only be true if total size differs from 0
		return &message;
		// free table
	} else return void;
}

//CAREFUL ABOUT ALL USES OF SEQPKT : STRUCT UNKNOWN FOR NOW

int main() {

	table = (struct HashTable*) malloc(sizeof(struct HashTable));

	insert(19, 20, table->content);
	insert(42, 80, table->content);
	insert(12, 44, table->content);
	insert(13, 78, table->content);
	insert(37, 97, table->content);

	display(table->content);
	item = search(37, table->content);
	
	if(item != NULL) {
		printf("Element found: %d\n", item->data);
	} else {
		printf("Element not found\n");
	}

	delete(item, table->content);
	item = search(37, table->content);
	
	if(item != NULL) {
		printf("Element found: %d\n", item->data);
	} else {
		printf("Element not found\n");
	}
}

//si L bit set, totalsize = franum + 1 et si currentSize == total size, renvoyer un pointeur vers une struct contenant current et total size + tableau complet de segments
//libérer la place des hashtable s'il n'y a plus de segments pour un message id ou plus de messages pour un gen id
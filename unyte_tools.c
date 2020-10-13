#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>  
#include <string.h>
#include "unyte_tools.h"

#define SPACE_MASK 0b00010000
#define ET_MASK 0b00001111

uint32_t deserialize_uint32(unsigned char *buffer) {
    uint32_t res = *((uint32_t *) buffer);
    return ntohl(res);
}

uint16_t deserialize_uint16(unsigned char *buffer) {
    uint16_t res = *((uint16_t *) buffer);
    return ntohl(res);
}

struct unyte_segment *parse(char *segment)
{
  struct unyte_header *header = malloc(sizeof(struct unyte_header));

  header->version = segment[0] >> 5;
  header->space = (segment[0] & SPACE_MASK);
  header->encoding_type = (segment[0] & ET_MASK);
  header->header_length = segment[1];
  header->message_length = deserialize_uint16((unsigned char *)(segment + 2));
  header->generator_id = deserialize_uint32((unsigned char *)(segment + 4));
  header->message_id = deserialize_uint32((unsigned char *)(segment + 8));
  
  char *payload = malloc(sizeof(header->message_length - header->header_length));
  *payload = segment[header->header_length];

  struct unyte_segment *seg = malloc(sizeof(struct unyte_segment) + sizeof(payload));
  
  seg->header = *header;
  seg->payload = payload;

  return seg;
}

void printHeader(struct unyte_header *header, FILE* std) {
  fprintf(std, "\n###### Unyte header ######\n");
  fprintf(std, "Version: %u\n", header->version);
  fprintf(std, "Space: %u\n", header->space);
  fprintf(std, "Encoding: %u\n", header->encoding_type);
  fprintf(std, "Header length: %u\n", header->header_length);
  fprintf(std, "Message length: %u\n", header->message_length);
  fprintf(std, "Generator ID: %u\n", header->generator_id);
  fprintf(std, "Mesage ID: %u\n", header->message_id);

  if (std == stdout) {
      fflush(stdout);
  }
}
  
void printPayload(char *p, int len, FILE* std) {
  int i;
  for (i = 0; i < len; i++)
    {
      fprintf(std, "%c", *(p + i));
    }
    fprintf(std, "\n");

    if (std == stdout) {
      fflush(stdout);
    }
}
#include <stdint.h>

#ifndef UNYTE_UTILS_H
#define UNYTE_UTILS_H

struct unyte_header
{
  uint8_t version : 3;
  uint8_t space : 1;
  uint8_t encoding_type : 4;
  uint8_t header_length : 8;
  uint16_t message_length;
  uint32_t generator_id;
  uint32_t message_id;
  
  /* Fragmentation options */
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

struct unyte_segment *parse(char *segment);
void printHeader(struct unyte_header *header, FILE *std);
void printPayload(char *p, int len, FILE* std);

#endif
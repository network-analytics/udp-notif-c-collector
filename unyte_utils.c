#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "unyte_utils.h"

#define SPACE_MASK 0b00010000
#define ET_MASK 0b00001111
#define LAST_MASK 0b00000001

/**
 * Return 32 bits unsigned integer value out of *C+P char pointer value. 
 */
uint32_t deserialize_uint32(char *c, int p)
{
  uint32_t u = 0;
  memcpy(&u, (c + p), 4);
  return u;
}

/**
 * Return 16 bits unsigned integer value out of *C+P char pointer value. 
 */
uint16_t deserialize_uint16(char *c, int p)
{
  uint16_t u = 0;
  memcpy(&u, (c + p), 2);
  return u;
}

/**
 * Return struct unyte_minimal data out of *SEGMENT.
 */
struct unyte_minimal *minimal_parse(char *segment, struct sockaddr_in *source, struct sockaddr_in *collector)
{
  struct unyte_minimal *um = malloc(sizeof(struct unyte_minimal));

  if (um == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  um->generator_id = ntohl(deserialize_uint32((char *)segment, 4));
  um->message_id = ntohl(deserialize_uint32((char *)segment, 8));
  
  um->buffer = segment;

  /* Do I need to reverse all theses ? */
  um->src_port = ntohs(source->sin_port);
  um->src_addr = ntohl(source->sin_addr.s_addr);
  um->collector_addr = ntohl(collector->sin_addr.s_addr);

  return um;
}

/**
 * Parse udp-notif segment out of *SEGMENT char array.
 * DEPRECATED use unyte_segment_with_metadata instead 
 */
struct unyte_segment *parse(char *segment)
{
  struct unyte_header *header = malloc(sizeof(struct unyte_header));

  if (header == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  header->version = segment[0] >> 5;
  header->space = (segment[0] & SPACE_MASK);
  header->encoding_type = (segment[0] & ET_MASK);
  header->header_length = segment[1];
  header->message_length = ntohs(deserialize_uint16((char *)segment, 2));
  header->generator_id = ntohl(deserialize_uint32((char *)segment, 4));
  header->message_id = ntohl(deserialize_uint32((char *)segment, 8));

  /* Header contains options */
  /* TODO handle something else than fragmentaion */

  if (header->header_length > 12)
  {
    header->f_type = segment[12];
    header->f_len = segment[13];

    /* WARN : Modifying directly segment, is it a pb ? + Not sure it works well*/
    /* If last = TRUE */

    if ((uint8_t)(segment[17] & 0b00000001) == 1)
    {
      header->f_last = 1;
    }
    else
    {
      header->f_last = 0;
    }
    header->f_num = (ntohl(deserialize_uint32((char *)segment, 14)) >> 1);
  }

  int pSize = header->message_length - header->header_length;

  char *payload = malloc(pSize);

  if (payload == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  memcpy(payload, (segment + header->header_length), pSize);

  struct unyte_segment *seg = malloc(sizeof(struct unyte_segment) + sizeof(payload));

  if (seg == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  /* not optimal - pass pointer instead */
  seg->header = header;
  seg->payload = payload;

  free(header);

  return seg;
}

/**
 * Parse udp-notif segment out of *SEGMENT char array.
 */
struct unyte_segment_with_metadata *parse_with_metadata(char *segment, struct unyte_minimal *um)
{
  struct unyte_header *header = malloc(sizeof(struct unyte_header));

  if (header == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  header->version = segment[0] >> 5;
  header->space = (segment[0] & SPACE_MASK);
  header->encoding_type = (segment[0] & ET_MASK);
  header->header_length = segment[1];
  header->message_length = ntohs(deserialize_uint16((char *)segment, 2));
  header->generator_id = ntohl(deserialize_uint32((char *)segment, 4));
  header->message_id = ntohl(deserialize_uint32((char *)segment, 8));

  /* Header contains options */
  /* TODO handle something else than fragmentaion */

  if (header->header_length > 12)
  {
    header->f_type = segment[12];
    header->f_len = segment[13];

    /* WARN : Modifying directly segment, is it a pb ? + Not sure it works well*/
    /* If last = TRUE */

    if ((uint8_t)(segment[17] & 0b00000001) == 1)
    {
      header->f_last = 1;
    }
    else
    {
      header->f_last = 0;
    }
    header->f_num = (ntohl(deserialize_uint32((char *)segment, 14)) >> 1);
  }

  int pSize = header->message_length - header->header_length;

  char *payload = malloc(pSize);
  if (payload == NULL)
  {
    printf("Malloc failed.\n");
    exit(-1);
  }
  

  if (payload == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  memcpy(payload, (segment + header->header_length), pSize);

  /* Passing metadatas */
  struct unyte_metadata *meta = (struct unyte_metadata *)malloc(sizeof(struct unyte_metadata));
  if (meta == NULL)
  {
    printf("Malloc failed.\n");
    exit(-1);
  }
  
  /*Filling the struct */
  meta->src_addr = um->src_addr;
  meta->src_port = um->src_port;
  meta->collector_addr = um->collector_addr;

  /* The global segment container */
  struct unyte_segment_with_metadata *seg = malloc(sizeof(struct unyte_segment_with_metadata) + sizeof(payload));

  if (seg == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  seg->header = header;
  seg->payload = payload;
  seg->metadata = meta;

  return seg;
}

/**
 * Display *HEADER to *STD.
 */
void printHeader(struct unyte_header *header, FILE *std)
{
  fprintf(std, "\n###### Unyte header ######\n");
  fprintf(std, "Version: %u\n", header->version);
  fprintf(std, "Space: %u\n", header->space);
  fprintf(std, "Encoding: %u\n", header->encoding_type);
  fprintf(std, "Header length: %u\n", header->header_length);
  fprintf(std, "Message length: %u\n", header->message_length);
  fprintf(std, "Generator ID: %u\n", header->generator_id);
  fprintf(std, "Mesage ID: %u\n", header->message_id);

  /* Header contains options */

  if (header->header_length > 12)
  {
    fprintf(std, "\nOptions: YES\n");
    fprintf(std, "opt type: %u\n", header->f_type);
    fprintf(std, "frag length: %u\n", header->f_len);
    fprintf(std, "frag message number: %u\n", header->f_num);
    fprintf(std, "frag last_flag: %u\n", header->f_last);
  }

  if (std == stdout)
  {
    fflush(stdout);
  }
}

/**
 * Print LEN char from *P on *STD.
 */
void printPayload(char *p, int len, FILE *std)
{
  int i;
  for (i = 0; i < len; i++)
  {
    fprintf(std, "%c", *(p + i));
  }
  fprintf(std, "\n");

  if (std == stdout)
  {
    fflush(stdout);
  }
}
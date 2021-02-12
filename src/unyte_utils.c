#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "unyte_utils.h"

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
 * Return unyte_min_t data out of *SEGMENT.
 */
unyte_min_t *minimal_parse(char *segment, struct sockaddr_in *source, struct sockaddr_in *collector)
{
  unyte_min_t *um = malloc(sizeof(unyte_min_t));

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
unyte_segment_t *parse(char *segment)
{
  unyte_header_t *header = malloc(sizeof(unyte_header_t));

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

  if (header->header_length > HEADER_BYTES)
  {
    header->f_type = segment[12];
    header->f_len = segment[13];

    /* WARN : Modifying directly segment, is it a pb ? + Not sure it works well*/
    /* If last = TRUE */

    if ((uint8_t)(segment[15] & 0b00000001) == 1)
    {
      header->f_last = 1;
    }
    else
    {
      header->f_last = 0;
    }
    header->f_num = (ntohs(deserialize_uint16((char *)segment, 14)) >> 1);
  }

  int pSize = header->message_length - header->header_length;

  char *payload = malloc(pSize);

  if (payload == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  memcpy(payload, (segment + header->header_length), pSize);

  unyte_segment_t *seg = malloc(sizeof(unyte_segment_t) + sizeof(payload));

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
unyte_seg_met_t *parse_with_metadata(char *segment, unyte_min_t *um)
{
  unyte_header_t *header = malloc(sizeof(unyte_header_t));

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
  /* TODO: handle something else than fragmentaion ? */
  // TODO: constantes à le mettre dans headers
  if (header->header_length > 12)
  {
    header->f_type = segment[12];
    header->f_len = segment[13];

    /* WARN : Modifying directly segment, is it a pb ? + Not sure it works well*/
    /* If last = TRUE */

    if ((uint8_t)(segment[15] & 0b00000001) == 1)
    {
      header->f_last = 1;
    }
    else
    {
      header->f_last = 0;
    }
    header->f_num = (ntohs(deserialize_uint16((char *)segment, 14)) >> 1);
  }
  int pSize = header->message_length - header->header_length;
  // printf("       Psize|%d|%d|%d\n",header->message_length, header->header_length, pSize);

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
  unyte_metadata_t *meta = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));
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
  unyte_seg_met_t *seg = malloc(sizeof(unyte_seg_met_t) + sizeof(payload));

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
 * Deep copies header values without options from src to dest 
 * Returns dest
 */
unyte_seg_met_t *copy_unyte_seg_met_headers(unyte_seg_met_t *dest, unyte_seg_met_t *src)
{
  dest->header->encoding_type = src->header->encoding_type;
  dest->header->generator_id = src->header->generator_id;
  dest->header->header_length = src->header->header_length;
  dest->header->message_id = src->header->message_id;
  dest->header->message_length = src->header->message_length;
  dest->header->space = src->header->space;
  dest->header->version = src->header->version;
  return dest;
}

/**
 * Deep copies header values without options from src to dest 
 * Returns dest
 */
unyte_seg_met_t *copy_unyte_seg_met_metadata(unyte_seg_met_t *dest, unyte_seg_met_t *src)
{
  dest->metadata->collector_addr = src->metadata->collector_addr;
  dest->metadata->src_addr = src->metadata->src_addr;
  dest->metadata->src_port = src->metadata->src_port;
  return dest;
}

/**
 * Display *HEADER to *STD.
 */
void printHeader(unyte_header_t *header, FILE *std)
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

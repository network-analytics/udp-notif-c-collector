#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef UNYTE_UTILS_H
#define UNYTE_UTILS_H

#define SPACE_MASK 0b00010000
#define ET_MASK 0b00001111
#define LAST_MASK 0b00000001
#define HEADER_BYTES 12

typedef struct unyte_header
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
} unyte_header_t;

typedef struct unyte_segment
{
  unyte_header_t *header;
  char *payload;
} unyte_segment_t;

typedef struct unyte_metadata
{
  /* Metadatas */
  uint16_t src_port;       /* Source port */
  uint32_t src_addr;       /* Source interface IPv4*/
  uint32_t collector_addr; /* Collector interface IPv4*/
} unyte_metadata_t;

/**
 * The complete segment structure
 */
typedef struct unyte_segment_with_metadata
{
  unyte_metadata_t *metadata;
  unyte_header_t *header;
  char *payload;
} unyte_seg_met_t;

typedef struct unyte_minimal
{
  /* Dispatching informations */
  uint32_t generator_id;
  uint32_t message_id;

  /* Serialized datas */
  char *buffer;

  /* Metadatas */
  uint16_t src_port;       /* Source port */
  uint32_t src_addr;       /* Source interface IPv4*/
  uint32_t collector_addr; /* Collector interface IPv4*/
} unyte_min_t;

typedef struct unyte_socket
{
  struct sockaddr_in *addr; /* The socket addr */
  int *sockfd;              /* The socket file descriptor */
} unyte_sock_t;

unyte_min_t *minimal_parse(char *segment, struct sockaddr_in *source, struct sockaddr_in *collector);
unyte_segment_t *parse(char *segment);
unyte_seg_met_t *parse_with_metadata(char *segment, unyte_min_t *um);

/**
 * Deep copies header values without options from src to dest 
 * Returns dest
 */
unyte_seg_met_t *copy_unyte_seg_met_headers(unyte_seg_met_t *dest, unyte_seg_met_t *src);
unyte_seg_met_t *copy_unyte_seg_met_metadata(unyte_seg_met_t *dest, unyte_seg_met_t *src);
void printHeader(unyte_header_t *header, FILE *std);
void printPayload(char *p, int len, FILE *std);

#endif

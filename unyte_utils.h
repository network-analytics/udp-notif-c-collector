#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
  struct unyte_header *header;
  char *payload;
};

struct unyte_metadata
{
  /* Metadatas */
  uint16_t src_port;            /* Source port */
  unsigned long src_addr;       /* Source interface IPv4*/
  unsigned long collector_addr; /* Collector interface IPv4*/
};

/**
 * The complete segment structure
 */
struct unyte_segment_with_metadata
{
  struct unyte_metadata *metadata;
  struct unyte_header *header;
  char *payload;
};

struct unyte_minimal
{
  /* Dispatching informations */
  uint32_t generator_id;
  uint32_t message_id;

  /* Serialized datas */
  char *buffer;

  /* Metadatas */
  uint16_t src_port;            /* Source port */
  unsigned long src_addr;       /* Source interface IPv4*/
  unsigned long collector_addr; /* Collector interface IPv4*/
};

typedef struct unyte_socket
{
  struct sockaddr_in *addr;          /* The socket addr */
  int sockfd;                        /* The socket file descriptor */
} unytesock_t;

struct unyte_minimal *minimal_parse(char *segment, struct sockaddr_in *source, struct sockaddr_in *collector);
struct unyte_segment *parse(char *segment);
struct unyte_segment_with_metadata *parse_with_metadata(char *segment, struct unyte_minimal *um);
void printHeader(struct unyte_header *header, FILE *std);
void printPayload(char *p, int len, FILE *std);

#endif
#ifndef UNYTE_UDP_UTILS_H
#define UNYTE_UDP_UTILS_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SPACE_MASK 0b00010000
#define ET_MASK 0b00001111
#define LAST_MASK 0b00000001
#define HEADER_BYTES 12
#define OPTIONS_BYTES 4
#define F_SEGMENTATION_TYPE 1

typedef struct unyte_header
{
  uint8_t version : 3;
  uint8_t space : 1;
  uint8_t encoding_type : 4;
  uint8_t header_length : 8;
  uint16_t message_length;
  uint32_t generator_id;
  uint32_t message_id;

  /* Segmentation options */
  uint8_t f_type;
  uint8_t f_len;
  uint32_t f_num : 31;
  uint8_t f_last : 1;
} unyte_header_t;

typedef struct unyte_metadata
{
  /* Metadatas */
  // uint16_t src_port;       /* Source port */
  // uint32_t src_addr;       /* Source interface IPv4*/
  struct sockaddr_storage *src;
  uint32_t collector_addr; /* Collector interface IPv4*/
} unyte_metadata_t;

/**
 * The complete segment structure
 */
typedef struct unyte_segment_with_metadata
{
  unyte_metadata_t *metadata; // source/port
  unyte_header_t *header;     // UDP-notif headers
  char *payload;              // payload of message
} unyte_seg_met_t;

typedef struct unyte_minimal
{
  /* Dispatching informations */
  uint32_t generator_id;
  uint32_t message_id;

  /* Serialized datas */
  char *buffer;

  /* Metadatas */
  // uint16_t src_port;       /* Source port */
  // uint32_t src_addr;       /* Source interface IPv4*/
  struct sockaddr_storage *src;  // Source
  uint32_t collector_addr;      // Collector interface IPv4
} unyte_min_t;

typedef struct
{
  struct sockaddr_in *addr; /* The socket addr */
  int *sockfd;              /* The socket file descriptor */
} unyte_udp_sock_t;

struct unyte_segmented_msg
{
  // Array of segments
  unyte_seg_met_t *segments;
  uint segments_len;
};

typedef struct unyte_message
{
  uint used_mtu;
  void *buffer;
  uint buffer_len;

  // UDP-notif
  uint8_t version : 3;
  uint8_t space : 1;
  uint8_t encoding_type : 4;
  uint32_t generator_id;
  uint32_t message_id;
} unyte_message_t;

typedef enum
{
  unyte_IPV4,
  unyte_IPV6,
  unyte_UNKNOWN
} unyte_IP_type_t;

/**
 * Return unyte_min_t data out of *SEGMENT.
 */
unyte_min_t *minimal_parse(char *segment, struct sockaddr_storage *source, struct sockaddr_in *collector);

/**
 * Parse udp-notif segment out of *SEGMENT char buffer.
 */
unyte_seg_met_t *parse_with_metadata(char *segment, unyte_min_t *um);

/**
 * Deep copies header values without options from src to dest 
 * Returns dest
 */
unyte_seg_met_t *copy_unyte_seg_met_headers(unyte_seg_met_t *dest, unyte_seg_met_t *src);

/**
 * Deep copies metadata values from src to dest 
 * Returns dest
 */
unyte_seg_met_t *copy_unyte_seg_met_metadata(unyte_seg_met_t *dest, unyte_seg_met_t *src);

/**
 * Print header to std buffer
 */
void print_udp_notif_header(unyte_header_t *header, FILE *std);

/**
 * Print payload to std buffer
 */
void print_udp_notif_payload(char *p, int len, FILE *std);

struct unyte_segmented_msg *build_message(unyte_message_t *message, uint mtu);
unsigned char *serialize_message(unyte_seg_met_t *message);

/**
 * Checks whether *addr is an IPv4 or IPv6
 * Return NULL if invalid addr
 */
unyte_IP_type_t get_IP_type(char *addr);

uint8_t unyte_udp_get_version(unyte_seg_met_t *message);
uint8_t unyte_udp_get_space(unyte_seg_met_t *message);
uint8_t unyte_udp_get_encoding_type(unyte_seg_met_t *message);
uint16_t unyte_udp_get_header_length(unyte_seg_met_t *message);
uint16_t unyte_udp_get_message_length(unyte_seg_met_t *message);
uint32_t unyte_udp_get_generator_id(unyte_seg_met_t *message);
uint32_t unyte_udp_get_message_id(unyte_seg_met_t *message);
// uint16_t unyte_udp_get_src_port(unyte_seg_met_t *message);
// uint32_t unyte_udp_get_src_addr(unyte_seg_met_t *message);
struct sockaddr_storage *unyte_udp_get_src(unyte_seg_met_t *message);
uint32_t unyte_udp_get_dest_addr(unyte_seg_met_t *message);
char *unyte_udp_get_payload(unyte_seg_met_t *message);
uint16_t unyte_udp_get_payload_length(unyte_seg_met_t *message);

#endif

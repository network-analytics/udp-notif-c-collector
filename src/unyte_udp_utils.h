#ifndef UNYTE_UDP_UTILS_H
#define UNYTE_UDP_UTILS_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SPACE_MASK 0b00010000
#define ET_MASK 0b00001111
#define LAST_MASK 0b00000001
#define HEADER_BYTES 12
#define UNYTE_SEGMENTATION_OPTION_LEN 4

typedef struct unyte_option
{
  uint8_t type;
  uint8_t length;
  char *data;
  struct unyte_option *next;
} unyte_option_t;

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
  unyte_option_t *options; // Linked list of custom options
} unyte_header_t;

typedef struct unyte_metadata
{
  /* Metadatas */
  struct sockaddr_storage *src;  // Source interface IPv4 or IPv6
  struct sockaddr_storage *dest; // Destination interface IPv4 or IPv6
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
  struct sockaddr_storage *src;  // Source
  struct sockaddr_storage *dest; // Destination interface IPv4 or IPv6
} unyte_min_t;

typedef struct
{
  struct sockaddr_storage *addr; // The socket addr
  int *sockfd;                   // The socket file descriptor
} unyte_udp_sock_t;

struct unyte_segmented_msg
{
  // Array of segments
  unyte_seg_met_t *segments;
  uint segments_len;
};

typedef struct unyte_send_option
{
  uint8_t type;
  uint8_t data_length;
  char *data;
} unyte_send_option_t;

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

  // Custom options
  unyte_send_option_t *options; // array of unyte_send_option_t
  uint options_len;             // options array length
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
unyte_min_t *minimal_parse(char *segment, struct sockaddr_storage *source, struct sockaddr_storage *dest_addr);

/**
 * Parse udp-notif segment out of *SEGMENT char buffer.
 */
unyte_seg_met_t *parse_with_metadata(char *segment, unyte_min_t *um);

/**
 * Parse udp-notif segment out of *SEGMENT char buffer with legacy headers: draft-ietf-netconf-udp-pub-channel-05
 */
unyte_seg_met_t *parse_with_metadata_legacy(char *segment, unyte_min_t *um);

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

unyte_option_t *build_message_empty_options();
struct unyte_segmented_msg *build_message(unyte_message_t *message, uint mtu);
unsigned char *serialize_message(unyte_seg_met_t *message);

/**
 * Checks whether *addr is an IPv4 or IPv6
 * Return NULL if invalid addr
 */
unyte_IP_type_t get_IP_type(char *addr);

/**
 * Creates socket and binds it to an address and port.
 * char *address : string of the IPv4 or IPv6 to bind the socket to.
 * char *port : string of the port to be bind to.
 * uint64_t buffer_size : socket buffer size.
 * Returns socketfd of the created socket.
 */
int unyte_udp_create_socket(char *address, char *port, uint64_t buffer_size);

/**
 * Creates socket and binds it to an interface using SO_BINDTODEVICE option.
 * It is also boung to an IP and port.
 * char *interface : string of the interface name to bind the socket to.
 * char *address : string of the IPv4 or IPv6 to bind the socket to.
 * char *port : string of the port to be bind to.
 * uint64_t buffer_size : socket buffer size.
 * Returns socketfd of the created socket.
 */
int unyte_udp_create_interface_bound_socket(char *interface, char *address, char *port, uint64_t buffer_size);

uint options_total_bytes(unyte_option_t *options);

uint8_t unyte_udp_get_version(unyte_seg_met_t *message);
uint8_t unyte_udp_get_space(unyte_seg_met_t *message);
uint8_t unyte_udp_get_encoding_type(unyte_seg_met_t *message);
uint16_t unyte_udp_get_header_length(unyte_seg_met_t *message);
uint16_t unyte_udp_get_message_length(unyte_seg_met_t *message);
uint32_t unyte_udp_get_generator_id(unyte_seg_met_t *message);
uint32_t unyte_udp_get_message_id(unyte_seg_met_t *message);
struct sockaddr_storage *unyte_udp_get_src(unyte_seg_met_t *message);
struct sockaddr_storage *unyte_udp_get_dest_addr(unyte_seg_met_t *message);
char *unyte_udp_get_payload(unyte_seg_met_t *message);
uint16_t unyte_udp_get_payload_length(unyte_seg_met_t *message);

#endif

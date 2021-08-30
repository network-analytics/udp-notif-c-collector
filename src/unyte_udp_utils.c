#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include "unyte_udp_utils.h"

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
unyte_min_t *minimal_parse(char *segment, struct sockaddr_storage *source, struct sockaddr_storage *dest_addr)
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

  um->src = source;
  um->dest = dest_addr;

  return um;
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
  // TODO: handle something else than segmentation ? --> check if option type == 1 (define in draft)
  if (header->header_length > HEADER_BYTES)
  {
    header->f_type = segment[12];
    header->f_len = segment[13];

    // If last = TRUE
    if ((uint8_t)(segment[15] & 0b00000001) == 1)
    {
      header->f_last = 1;
    }
    else
    {
      header->f_last = 0;
    }
    // FIXME: Works only if segmentation option is the first option -> check option type == 1 (to define in draft)
    header->f_num = (ntohs(deserialize_uint16((char *)segment, HEADER_BYTES + 2)) >> 1);
  }
  int pSize = header->message_length - header->header_length;

  char *payload = malloc(pSize);

  if (payload == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }

  memcpy(payload, (segment + header->header_length), pSize);

  // Passing metadatas
  unyte_metadata_t *meta = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));
  if (meta == NULL)
  {
    printf("Malloc failed.\n");
    exit(-1);
  }

  // Filling the struct
  meta->src = um->src;
  meta->dest = um->dest;

  // The global segment container
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
  memcpy(dest->metadata->src, src->metadata->src, sizeof(struct sockaddr_storage));
  // memcpy(dest->metadata->dest, src->metadata->dest, sizeof(struct sockaddr_storage));
  return dest;
}

/**
 * Display *HEADER to *STD.
 */
void print_udp_notif_header(unyte_header_t *header, FILE *std)
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

  if (header->header_length > HEADER_BYTES)
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
void print_udp_notif_payload(char *p, int len, FILE *std)
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

struct unyte_segmented_msg *build_message(unyte_message_t *message, uint mtu)
{
  struct unyte_segmented_msg *segments_msg = (struct unyte_segmented_msg *)malloc(sizeof(struct unyte_segmented_msg));
  if (segments_msg == NULL)
  {
    printf("Malloc failed.\n");
    return NULL;
  }

  uint packets_to_send = 1;
  // Usable bytes for segmentation
  uint actual_usable_bytes = (mtu - HEADER_BYTES - OPTIONS_BYTES);
  if ((message->buffer_len + HEADER_BYTES) > mtu)
  {
    packets_to_send = (message->buffer_len / actual_usable_bytes);
    if (message->buffer_len % actual_usable_bytes != 0)
    {
      packets_to_send++;
    }
  }

  segments_msg->segments = (unyte_seg_met_t *)malloc(sizeof(unyte_seg_met_t) * packets_to_send);
  segments_msg->segments_len = packets_to_send;
  if (segments_msg->segments == NULL)
  {
    printf("Malloc failed \n");
    return NULL;
  }
  unyte_seg_met_t *current_seg = segments_msg->segments;

  if (packets_to_send == 1)
  {
    // not segmented
    current_seg->payload = (char *)malloc(message->buffer_len);
    if (current_seg->payload == NULL)
    {
      printf("Malloc failed \n");
      return NULL;
    }
    memcpy(current_seg->payload, message->buffer, message->buffer_len);
    current_seg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));

    if (current_seg->header == NULL)
    {
      printf("Malloc failed \n");
      return NULL;
    }
    current_seg->header->generator_id = message->generator_id;
    current_seg->header->message_id = message->message_id;
    current_seg->header->space = message->space;
    current_seg->header->version = message->version;
    current_seg->header->encoding_type = message->encoding_type;
    current_seg->header->header_length = HEADER_BYTES;
    current_seg->header->message_length = message->buffer_len;
  }
  else
  {
    // segmented packets
    uint copy_it = 0;
    for (uint i = 0; i < segments_msg->segments_len; i++)
    {
      // Min(bytes_to_copy, actual_usable_bytes)
      uint bytes_to_send = (message->buffer_len - copy_it) < actual_usable_bytes ? (message->buffer_len - copy_it) : actual_usable_bytes;
      current_seg->payload = (char *)malloc(bytes_to_send);
      if (current_seg->payload == NULL)
      {
        printf("Malloc failed \n");
        return NULL;
      }
      memcpy(current_seg->payload, (message->buffer + copy_it), bytes_to_send);
      current_seg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));
      if (current_seg->header == NULL)
      {
        printf("Malloc failed \n");
        return NULL;
      }
      current_seg->header->generator_id = message->generator_id;
      current_seg->header->message_id = message->message_id;
      current_seg->header->space = message->space;
      current_seg->header->version = message->version;
      current_seg->header->encoding_type = message->encoding_type;
      current_seg->header->header_length = HEADER_BYTES + OPTIONS_BYTES;
      current_seg->header->message_length = bytes_to_send;

      current_seg->header->f_num = i;
      current_seg->header->f_len = OPTIONS_BYTES;
      current_seg->header->f_type = F_SEGMENTATION_TYPE;
      if (i == segments_msg->segments_len - 1)
      {
        current_seg->header->f_last = 1;
      }
      else
      {
        current_seg->header->f_last = 0;
      }

      current_seg++;
      copy_it += bytes_to_send;
    }
  }

  return segments_msg;
}

unsigned char *serialize_message(unyte_seg_met_t *msg)
{
  uint packet_size = msg->header->message_length + HEADER_BYTES;
  if (msg->header->header_length > HEADER_BYTES)
  {
    // segmented header
    packet_size = msg->header->message_length + HEADER_BYTES + OPTIONS_BYTES;
  }

  unsigned char *parsed_bytes = (unsigned char *)malloc(packet_size);
  parsed_bytes[0] = (((msg->header->version << 5) + (msg->header->space << 4) + (msg->header->encoding_type)));
  parsed_bytes[1] = msg->header->header_length;
  // message length
  parsed_bytes[2] = ((packet_size) >> 8);
  parsed_bytes[3] = (packet_size);

  // observation id
  parsed_bytes[4] = (msg->header->generator_id >> 24);
  parsed_bytes[5] = (msg->header->generator_id >> 16);
  parsed_bytes[6] = (msg->header->generator_id >> 8);
  parsed_bytes[7] = (msg->header->generator_id);
  // message id
  parsed_bytes[8] = (msg->header->message_id >> 24);
  parsed_bytes[9] = (msg->header->message_id >> 16);
  parsed_bytes[10] = (msg->header->message_id >> 8);
  parsed_bytes[11] = (msg->header->message_id);
  uint payload_start = HEADER_BYTES;

  if (msg->header->header_length > HEADER_BYTES)
  {
    parsed_bytes[12] = (msg->header->f_type);
    parsed_bytes[13] = (msg->header->f_len);
    parsed_bytes[14] = (msg->header->f_num >> 8);
    parsed_bytes[15] = (msg->header->f_num << 1) + msg->header->f_last;
    payload_start = HEADER_BYTES + OPTIONS_BYTES;
  }
  memcpy(parsed_bytes + payload_start, msg->payload, msg->header->message_length);

  return parsed_bytes;
}

unyte_IP_type_t get_IP_type(char *addr)
{
  char buf[16];
  if (inet_pton(AF_INET, addr, buf))
    return unyte_IPV4;
  else if (inet_pton(AF_INET6, addr, buf))
    return unyte_IPV6;
  
  return unyte_UNKNOWN;
}

int unyte_udp_create_socket(char *address, char *port, uint64_t buffer_size)
{
  assert(address != NULL);
  assert(port != NULL);
  assert(buffer_size > 0);

  struct addrinfo *addr_info;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

  // Using getaddrinfo to support both IPv4 and IPv6
  int rc = getaddrinfo(address, port, &hints, &addr_info);

  if (rc != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(rc));
    exit(EXIT_FAILURE);
  }

  printf("Address type: %s | %d\n", (addr_info->ai_family == AF_INET) ? "IPv4" : "IPv6", ntohs(((struct sockaddr_in *)addr_info->ai_addr)->sin_port));

  // create socket on UDP protocol
  int sockfd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

  // handle error
  if (sockfd < 0)
  {
    perror("Cannot create socket");
    exit(EXIT_FAILURE);
  }

  // Use SO_REUSEPORT to be able to launch multiple collector on the same address
  int optval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int)) < 0)
  {
    perror("Cannot set SO_REUSEPORT option on socket");
    exit(EXIT_FAILURE);
  }

  uint64_t receive_buf_size = buffer_size;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &receive_buf_size, sizeof(receive_buf_size)) < 0)
  {
    perror("Cannot set buffer size");
    exit(EXIT_FAILURE);
  }

  if (bind(sockfd, addr_info->ai_addr, (int)addr_info->ai_addrlen) == -1)
  {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // free addr_info after usage
  freeaddrinfo(addr_info);

  return sockfd;
}

uint8_t unyte_udp_get_version(unyte_seg_met_t *message) { return message->header->version; }
uint8_t unyte_udp_get_space(unyte_seg_met_t *message) { return message->header->space; }
uint8_t unyte_udp_get_encoding_type(unyte_seg_met_t *message) { return message->header->encoding_type; }
uint16_t unyte_udp_get_header_length(unyte_seg_met_t *message) { return message->header->header_length; }
uint16_t unyte_udp_get_message_length(unyte_seg_met_t *message) { return message->header->message_length; }
uint32_t unyte_udp_get_generator_id(unyte_seg_met_t *message) { return message->header->generator_id; }
uint32_t unyte_udp_get_message_id(unyte_seg_met_t *message) { return message->header->message_id; }
struct sockaddr_storage *unyte_udp_get_src(unyte_seg_met_t *message) { return message->metadata->src; }
struct sockaddr_storage *unyte_udp_get_dest_addr(unyte_seg_met_t *message) { return message->metadata->dest; }
char *unyte_udp_get_payload(unyte_seg_met_t *message) { return message->payload; }
uint16_t unyte_udp_get_payload_length(unyte_seg_met_t *message) { return (message->header->message_length - message->header->header_length); }

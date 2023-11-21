#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <unistd.h>
#include <assert.h>
#include "unyte_udp_utils.h"
#include "hexdump.h"

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

unyte_option_t *build_message_empty_options()
{
  unyte_option_t *head = (unyte_option_t *)malloc(sizeof(unyte_option_t));
  if (head == NULL) return NULL;
  head->data = NULL;
  head->length = 0;
  head->type = 0;
  head->next = NULL;
  return head;
}

/**
 * Return unyte_min_t data out of *SEGMENT.
 */
unyte_min_t *minimal_parse(char *segment, struct sockaddr_storage *source, struct sockaddr_storage *dest_addr)
{
  unyte_min_t *um = malloc(sizeof(unyte_min_t));

  if (um == NULL)
  {
    printf("Malloc failed minimal parse\n");
    return NULL;
  }

  um->observation_domain_id = ntohl(deserialize_uint32((char *)segment, 4));
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
  unyte_option_t *options_head = build_message_empty_options();
  unyte_metadata_t *meta = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));
  if (header == NULL || options_head == NULL || meta == NULL)
  {
    printf("Malloc failed metadata\n");
    return NULL;
  }

  header->version = segment[0] >> 5;
  header->space = (segment[0] & SPACE_MASK);
  header->media_type = (segment[0] & ET_MASK);
  header->header_length = segment[1];
  header->message_length = ntohs(deserialize_uint16((char *)segment, 2));
  header->observation_domain_id = ntohl(deserialize_uint32((char *)segment, 4));
  header->message_id = ntohl(deserialize_uint32((char *)segment, 8));
  header->options = options_head;

  printf("HEADER 1 = %d\n", segment[0]);

  hexdump(segment, 21);

  // Header contains options
  unyte_option_t *last_option = options_head;
  int options_length = 0;
  printf("HEADER LENGTH = %d\n", header->header_length);
  printf("MESSAGE LENGTH = %d\n", header->message_length);
  while((options_length + HEADER_BYTES) < header->header_length)
  {
    printf("HEADER BYTES = %d\n", HEADER_BYTES);
    printf("HEADER BYTES + OPTION = %d\n", (HEADER_BYTES + options_length));
    uint8_t type = segment[HEADER_BYTES + options_length];
    uint8_t length = segment[HEADER_BYTES + options_length + 1];
    printf("LENGTH 100 = %d\n", length);
    // segmented message
    if (type == UNYTE_TYPE_SEGMENTATION) // 4
    {
      header->f_type = type;
      header->f_len = length;
      // If last = TRUE
      if ((uint8_t)(segment[HEADER_BYTES + options_length + 3] & 0b00000001) == 1)
        header->f_last = 1;
      else
        header->f_last = 0;

      header->f_num = (ntohs(deserialize_uint16((char *)segment, HEADER_BYTES + options_length + 2)) >> 1);
    }
    else // custom options 
    {
      unyte_option_t *custom_option = malloc(sizeof(unyte_option_t));
      printf("LENGTH = %d\n", (length - 2));
      char *options_data = malloc(length - 2); // 1 byte for type and 1 byte for length
      if (custom_option == NULL) printf("malloc failed custom\n");
      if (options_data == NULL) printf("malloc failed data\n");
      if (custom_option == NULL || options_data == NULL)
      {
        printf("Malloc failed option\n");
        return NULL;
      }
      custom_option->type = type;
      custom_option->length = length;
      custom_option->next = NULL;
      custom_option->data = options_data;
      memcpy(custom_option->data, segment + HEADER_BYTES + options_length + 2, length - 2);

      last_option->next = custom_option;
      last_option = custom_option;
    }
    options_length += length;
  }
  int pSize = header->message_length - header->header_length;

  char *payload = malloc(pSize);

  if (payload == NULL)
  {
    printf("Malloc failed payload\n");
    return NULL;
  }

  memcpy(payload, (segment + header->header_length), pSize);

  // Filling the struct
  meta->src = um->src;
  meta->dest = um->dest;

  // The global segment container
  unyte_seg_met_t *seg = malloc(sizeof(unyte_seg_met_t) + sizeof(payload));

  if (seg == NULL)
  {
    printf("Malloc failed segment\n");
    return NULL;
  }

  seg->header = header;
  seg->payload = payload;
  seg->metadata = meta;

  return seg;
}

int get_encoding_IANA_ET_from_legacy(uint8_t legacy_ET)
{
  // TODO: error ?
  if (legacy_ET > SUPPORTED_ET_LEN)
    return UNYTE_MEDIATYPE_RESERVED;

  return LEGACY_ET_TO_IANA[legacy_ET];
}

unyte_seg_met_t *parse_with_metadata_legacy(char *segment, unyte_min_t *um)
{
  unyte_header_t *header = malloc(sizeof(unyte_header_t));
  unyte_option_t *options_head = build_message_empty_options();
  unyte_metadata_t *meta = (unyte_metadata_t *)malloc(sizeof(unyte_metadata_t));
  if (header == NULL || options_head == NULL || meta == NULL)
  {
    printf("Malloc failed metadata legacy\n");
    return NULL;
  }

  header->version = segment[0] >> 4;
  header->space = 0;  // arbitrary
  header->media_type = get_encoding_IANA_ET_from_legacy((segment[1] & ET_MASK));
  header->header_length = HEADER_BYTES;
  header->message_length = ntohs(deserialize_uint16((char *)segment, 2));
  header->observation_domain_id = ntohl(deserialize_uint32((char *)segment, 4));
  header->message_id = ntohl(deserialize_uint32((char *)segment, 8));
  header->options = options_head;

  int pSize = header->message_length - header->header_length;

  char *payload = malloc(pSize);

  if (payload == NULL)
  {
    printf("Malloc failed payload2\n");
    return NULL;
  }

  memcpy(payload, (segment + header->header_length), pSize);

  // Filling the struct
  meta->src = um->src;
  meta->dest = um->dest;

  // The global segment container
  unyte_seg_met_t *seg = malloc(sizeof(unyte_seg_met_t) + sizeof(payload));

  if (seg == NULL)
  {
    printf("Malloc failed segment 2\n");
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
  dest->header->media_type = src->header->media_type;
  dest->header->observation_domain_id = src->header->observation_domain_id;
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
  if (src->metadata->dest != NULL)
    memcpy(dest->metadata->dest, src->metadata->dest, sizeof(struct sockaddr_storage));
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
  fprintf(std, "Media type: %u\n", header->media_type);
  fprintf(std, "Header length: %u\n", header->header_length);
  fprintf(std, "Message length: %u\n", header->message_length);
  fprintf(std, "Observation domain ID: %u\n", header->observation_domain_id);
  fprintf(std, "Mesage ID: %u\n", header->message_id);

  // Header contains options
  uint options_length = options_total_bytes(header->options);
  bool is_segmented = header->header_length > (HEADER_BYTES + options_length);
  if (is_segmented)
  {
    fprintf(std, "\nOption segmentation:\n");
    fprintf(std, "opt type: %u\n", header->f_type);
    fprintf(std, "seg length: %u\n", header->f_len);
    fprintf(std, "seg message number: %u\n", header->f_num);
    fprintf(std, "seg last_flag: %u\n", header->f_last);
  }
  if (options_length > 0)
  {
    unyte_option_t *head = header->options;
    unyte_option_t *cur = head->next;
    while(cur != NULL)
    {
      fprintf(std, "\nCustom Option:\n");
      fprintf(std, "opt type: %u\n", cur->type);
      fprintf(std, "opt length: %u\n", cur->length);
      fprintf(std, "opt description: %.*s\n", cur->length - 2, cur->data);
      cur = cur->next;
    }
  }

  if (std == stdout)
    fflush(stdout);
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

uint options_total_bytes(unyte_option_t *options)
{
  uint length = 0;
  unyte_option_t *cur = options;
  while (cur->next != NULL)
  {
    cur = cur->next;
    length += cur->length;
  }
  return length;
}

unyte_option_t *build_message_options(unyte_send_option_t *options, uint options_len)
{
  unyte_option_t *head = build_message_empty_options();
  if (head == NULL) return NULL;
  unyte_option_t *current = head;
  char *buffer;
  for (uint i = 0; i < options_len; i++)
  {
    current->next = (unyte_option_t *)malloc(sizeof(unyte_option_t));
    buffer = malloc(options[i].data_length);
    if (current->next == NULL || buffer == NULL) 
    {
      printf("Malloc failed option 2\n");
      return NULL;
    }
    current = current->next;
    current->type = options[i].type;
    current->length = options[i].data_length + 2; // length of user data + 1 byte for type and 1 byte for length of the TLV
    current->data = buffer;
    current->next = NULL;
    memcpy(current->data, options[i].data, options[i].data_length);
  }

  return head;
}

struct unyte_segmented_msg *build_message(unyte_message_t *message, uint mtu)
{
  struct unyte_segmented_msg *segments_msg = (struct unyte_segmented_msg *)malloc(sizeof(struct unyte_segmented_msg));
  if (segments_msg == NULL)
  {
    printf("Malloc failed segmented\n");
    return NULL;
  }

  // Usable bytes for segmentation
  unyte_option_t *options_header = build_message_options(message->options, message->options_len);
  uint options_header_bytes = options_total_bytes(options_header);

  uint actual_usable_bytes = (mtu - HEADER_BYTES - UNYTE_SEGMENTATION_OPTION_LEN);
  uint message_len = options_header_bytes + message->buffer_len;
  uint packets_to_send = 1;

  if ((message_len + HEADER_BYTES) > mtu)
  {
    packets_to_send = (message_len / actual_usable_bytes);
    if (message_len % actual_usable_bytes != 0)
      packets_to_send++;
  }
  printf("Packets: %d\n", packets_to_send);
  segments_msg->segments = (unyte_seg_met_t *)malloc(sizeof(unyte_seg_met_t) * packets_to_send);
  segments_msg->segments_len = packets_to_send;

  if (segments_msg->segments == NULL || options_header == NULL)
  {
    printf("Malloc failed segmented 2\n");
    return NULL;
  }
  unyte_seg_met_t *current_seg = segments_msg->segments;

  if (packets_to_send == 1)
  {
    // not segmented
    current_seg->payload = (char *)malloc(message->buffer_len);
    current_seg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));

    if (current_seg->payload == NULL || current_seg->header == NULL)
    {
      printf("Malloc failed payload 2\n");
      return NULL;
    }

    memcpy(current_seg->payload, message->buffer, message->buffer_len);

    current_seg->header->observation_domain_id = message->observation_domain_id;
    current_seg->header->message_id = message->message_id;
    current_seg->header->space = message->space;
    current_seg->header->version = message->version;
    current_seg->header->media_type = message->media_type;
    current_seg->header->header_length = HEADER_BYTES + options_header_bytes;
    current_seg->header->message_length = message->buffer_len;
    current_seg->header->options = options_header;
  }
  else
  {
    // segmented packets
    uint copy_it = 0;
    for (uint i = 0; i < segments_msg->segments_len; i++)
    {
      // Min(bytes_to_copy, actual_usable_bytes)
      uint bytes_to_send = (message->buffer_len - copy_it) < actual_usable_bytes ? (message->buffer_len - copy_it) : actual_usable_bytes;

      if (i == 0 && options_header_bytes > 0)
        bytes_to_send -= options_header_bytes;

      current_seg->payload = (char *)malloc(bytes_to_send);
      if (current_seg->payload == NULL)
      {
        printf("Malloc failed byte to send\n");
        return NULL;
      }
      memcpy(current_seg->payload, (message->buffer + copy_it), bytes_to_send);
      current_seg->header = (unyte_header_t *)malloc(sizeof(unyte_header_t));
      if (current_seg->header == NULL)
      {
        printf("Malloc failed payload 3\n");
        return NULL;
      }
      current_seg->header->observation_domain_id = message->observation_domain_id;
      current_seg->header->message_id = message->message_id;
      current_seg->header->space = message->space;
      current_seg->header->version = message->version;
      current_seg->header->media_type = message->media_type;
      current_seg->header->message_length = bytes_to_send;
      current_seg->header->header_length = HEADER_BYTES + UNYTE_SEGMENTATION_OPTION_LEN;

      // first packet have the custom options
      if (i == 0)
      {
        current_seg->header->options = options_header;
        current_seg->header->header_length += options_header_bytes;
      }
      else
        current_seg->header->options = build_message_empty_options();

      current_seg->header->f_num = i;
      current_seg->header->f_len = UNYTE_SEGMENTATION_OPTION_LEN;
      current_seg->header->f_type = UNYTE_TYPE_SEGMENTATION;

      if (i == segments_msg->segments_len - 1)
        current_seg->header->f_last = 1;
      else
        current_seg->header->f_last = 0;

      current_seg++;
      copy_it += bytes_to_send;
    }
  }

  return segments_msg;
}

unsigned char *serialize_message(unyte_seg_met_t *msg)
{
  uint custom_options = options_total_bytes(msg->header->options);
  uint packet_size = HEADER_BYTES + custom_options + msg->header->message_length;
  bool is_segmented = msg->header->header_length > (HEADER_BYTES + custom_options);
  if (is_segmented)
  {
    // segmented header
    packet_size += UNYTE_SEGMENTATION_OPTION_LEN;
  }
  unsigned char *parsed_bytes = (unsigned char *)malloc(packet_size);
  parsed_bytes[0] = (((msg->header->version << 5) + (msg->header->space << 4) + (msg->header->media_type)));
  parsed_bytes[1] = msg->header->header_length;
  // message length
  parsed_bytes[2] = ((packet_size) >> 8);
  parsed_bytes[3] = (packet_size);

  // observation id
  parsed_bytes[4] = (msg->header->observation_domain_id >> 24);
  parsed_bytes[5] = (msg->header->observation_domain_id >> 16);
  parsed_bytes[6] = (msg->header->observation_domain_id >> 8);
  parsed_bytes[7] = (msg->header->observation_domain_id);
  // message id
  parsed_bytes[8] = (msg->header->message_id >> 24);
  parsed_bytes[9] = (msg->header->message_id >> 16);
  parsed_bytes[10] = (msg->header->message_id >> 8);
  parsed_bytes[11] = (msg->header->message_id);

  uint options_it = HEADER_BYTES;
  if (is_segmented)
  {
    parsed_bytes[options_it] = (msg->header->f_type);
    parsed_bytes[options_it + 1] = (msg->header->f_len);
    parsed_bytes[options_it + 2] = (msg->header->f_num >> 8);
    parsed_bytes[options_it + 3] = (msg->header->f_num << 1) + msg->header->f_last;
    options_it += UNYTE_SEGMENTATION_OPTION_LEN;
  }

  unyte_option_t *head = msg->header->options;
  unyte_option_t *cur = head->next;
  while (cur != NULL)
  {
    parsed_bytes[options_it] = cur->type;
    parsed_bytes[options_it + 1] = cur->length;
    memcpy(parsed_bytes + options_it + 2, cur->data, cur->length - 2);
    options_it += cur->length;
    cur = cur->next;
  }
  memcpy(parsed_bytes + options_it, msg->payload, msg->header->message_length);

  // hexdump(parsed_bytes, packet_size);
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
  int on = 1;

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
  // int optval = 1;
  // if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int)) < 0)
  // {
  //   perror("Cannot set SO_REUSEPORT option on socket");
  //   exit(EXIT_FAILURE);
  // }

  // uint64_t receive_buf_size = buffer_size;
  // if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &receive_buf_size, sizeof(receive_buf_size)) < 0)
  // { 
  // }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) != 0) {
    perror("setsockopt() with SO_REUSEADDR");
    if (sockfd != INVALID_SOCKET) {
        close(sockfd);
        sockfd = INVALID_SOCKET;
    }
    return INVALID_SOCKET;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on)) != 0) {
    perror("setsockopt() with SO_REUSEPORT");
    if (sockfd != INVALID_SOCKET) {
        close(sockfd);
        sockfd = INVALID_SOCKET;
    }
    return INVALID_SOCKET;
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


int unyte_udp_create_interface_bound_socket(char *interface, char *address, char *port, bool ipv6_only, uint64_t buffer_size)
{
  assert(interface != NULL);
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
    freeaddrinfo(addr_info);
    exit(EXIT_FAILURE);
  }

  // Use SO_REUSEPORT to be able to launch multiple collector on the same address
  int optval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int)) < 0)
  {
    perror("Cannot set SO_REUSEPORT option on socket");
    freeaddrinfo(addr_info);
    exit(EXIT_FAILURE);
  }

  uint64_t receive_buf_size = buffer_size;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &receive_buf_size, sizeof(receive_buf_size)) < 0)
  {
    perror("Cannot set buffer size");
    freeaddrinfo(addr_info);
    exit(EXIT_FAILURE);
  }

  const char *interface_name = interface;
  if (interface_name != NULL && (strlen(interface_name) > 0))
  {
    printf("Binding to interface: %s\n", interface_name);
    int len = strnlen(interface_name, IFNAMSIZ);
    if (len == IFNAMSIZ)
    {
      fprintf(stderr, "Too long iface name");
      freeaddrinfo(addr_info);
      exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface_name, len) < 0)
    {
      perror("Bind socket to interface failed");
      freeaddrinfo(addr_info);
      exit(EXIT_FAILURE);
    }
  }

  if (addr_info->ai_family == AF_INET6 && ipv6_only)
  {
    int optvalyes = 1;
    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optvalyes, sizeof(optvalyes)) < 0)
    {
      perror("Cannot set IPV6_V6ONLY");
      freeaddrinfo(addr_info);
      exit(EXIT_FAILURE);
    }
  }

  if (bind(sockfd, addr_info->ai_addr, (int)addr_info->ai_addrlen) == -1)
  {
    perror("Bind failed");
    close(sockfd);
    freeaddrinfo(addr_info);
    exit(EXIT_FAILURE);
  }

  // free addr_info after usage
  freeaddrinfo(addr_info);

  return sockfd;
}

uint8_t unyte_udp_get_version(unyte_seg_met_t *message) { return message->header->version; }
uint8_t unyte_udp_get_space(unyte_seg_met_t *message) { return message->header->space; }
uint8_t unyte_udp_get_media_type(unyte_seg_met_t *message) { return message->header->media_type; }
uint16_t unyte_udp_get_header_length(unyte_seg_met_t *message) { return message->header->header_length; }
uint16_t unyte_udp_get_message_length(unyte_seg_met_t *message) { return message->header->message_length; }
uint32_t unyte_udp_get_observation_domain_id(unyte_seg_met_t *message) { return message->header->observation_domain_id; }
uint32_t unyte_udp_get_message_id(unyte_seg_met_t *message) { return message->header->message_id; }
struct sockaddr_storage *unyte_udp_get_src(unyte_seg_met_t *message) { return message->metadata->src; }
struct sockaddr_storage *unyte_udp_get_dest_addr(unyte_seg_met_t *message) { return message->metadata->dest; }
char *unyte_udp_get_payload(unyte_seg_met_t *message) { return message->payload; }
uint16_t unyte_udp_get_payload_length(unyte_seg_met_t *message) { return (message->header->message_length - message->header->header_length); }

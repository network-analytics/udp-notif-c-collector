/**
 * Example of client using a eBPF loadbalancer.
 * This allows multiple client instances listening on the same IP/port and receiving consistent data 
 * (all packets from one src IP will always go on the same collector)
 *
 * Usage: ./client_ebpf <ip> <port> <index> <balancer_max>
 *
 * Example: launching 3 instances on the same ip port. The index is the index on the map to put the socket
 * and the balancer_max is how many max instances are in use.
 *
 *    ./client_ebpf 192.168.1.17 10001 0 3
 *    ./client_ebpf 192.168.1.17 10001 1 3
 *    ./client_ebpf 192.168.1.17 10001 2 3
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/bpf.h>
#include <linux/unistd.h>

#include "../../src/hexdump.h"
#include "../../src/unyte_udp_collector.h"
#include "../../src/unyte_udp_utils.h"
#include "../../src/unyte_udp_queue.h"

#define USED_VLEN 1
#define MAX_TO_RECEIVE 20

#define BPF_KERNEL_PRG "reuseport_udp_kern.o"

#ifndef MAX_BALANCER_COUNT
// Keep in sync with _kern.c
#define MAX_BALANCER_COUNT 128
#endif

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args) {
  return level <= LIBBPF_DEBUG ? vfprintf(stderr, format, args) : 0;
}

/**
 * Creates own custom socket
 */
int create_socket(char *address, char *port)
{
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

  // Setting socket buffer to default 20 MB
  uint64_t receive_buf_size = DEFAULT_SK_BUFF_SIZE;
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

int open_socket_attach_ebpf(char *address, char *port, uint32_t key, uint32_t balancer_count)
{
  int umap_fd, size_map_fd, prog_fd;
  char filename[] = BPF_KERNEL_PRG;
  int64_t usock;
  long err = 0;

  assert(!balancer_count || key < balancer_count);
  assert(balancer_count <= MAX_BALANCER_COUNT);
  printf("from args: Using hash bucket index %u", key);
  if (balancer_count > 0) printf(" (%u buckets in total)", balancer_count);
  puts("");

  // set log
  libbpf_set_print(libbpf_print_fn);

  // Open reuseport_udp_kern.o
  struct bpf_object_open_opts opts = {.sz = sizeof(struct bpf_object_open_opts),
                                      .pin_root_path = "/sys/fs/bpf/reuseport"};
  struct bpf_object *obj = bpf_object__open_file(filename, &opts);

  err = libbpf_get_error(obj);
  if (err) {
    perror("Failed to open BPF elf file");
    return 1;
  }

  struct bpf_map *udpmap = bpf_object__find_map_by_name(obj, "udp_balancing_targets");
  assert(udpmap);

  // Load reuseport_udp_kern.o to the kernel
  if (bpf_object__load(obj) != 0) {
    perror("Error loading BPF object into kernel");
    return 1;
  }

  struct bpf_program *prog = bpf_object__find_program_by_name(obj, "_selector");
  if (!prog) {
    perror("Could not find BPF program in BPF object");
    return 1;
  }

  prog_fd = bpf_program__fd(prog);
  assert(prog_fd);

  umap_fd = bpf_map__fd(udpmap);
  assert(umap_fd);

  usock = create_socket(address, port);

  assert(usock >= 0);

  if (setsockopt(usock, SOL_SOCKET, SO_ATTACH_REUSEPORT_EBPF, &prog_fd, sizeof(prog_fd)) != 0) {
    perror("Could not attach BPF prog");
    return 1;
  }

  printf("UDP sockfd: %ld\n", usock);
  if (bpf_map_update_elem(umap_fd, &key, &usock, BPF_ANY) != 0) {
    perror("Could not update reuseport array");
    return 1;
  }

  // Determine intended number of hash buckets
  // Assumption: static during lifetime of this process
  struct bpf_map *size_map = bpf_object__find_map_by_name(obj, "size");
  assert(size_map);
  size_map_fd = bpf_map__fd(size_map);
  assert(size_map_fd);

  uint32_t index = 0;
  if (balancer_count == 0) {  // no user-supplied limit
    bpf_map_lookup_elem(size_map_fd, &index, &balancer_count);
    if (balancer_count == 0) {  // BPF program hasn't run yet to initalize this
      balancer_count = MAX_BALANCER_COUNT;
      if (bpf_map_update_elem(size_map_fd, &index, &balancer_count, BPF_ANY) != 0) {
        perror("Could not update balancer count");
        return 1;
      }
    }
  } else {  // Overwrite global count with user supplied one
    if (bpf_map_update_elem(size_map_fd, &index, &balancer_count, BPF_ANY) != 0) {
      perror("Could not update balancer count");
      return 1;
    }
  }

  return usock;
}


int main(int argc, char *argv[])
{
  if (argc != 5)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./client_ebpf <ip> <port> <index> <loadbalance_max>\n");
    exit(1);
  }

  int sockfd = open_socket_attach_ebpf(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));

  // Initialize collector options
  unyte_udp_sk_options_t options = {0};
  options.recvmmsg_vlen = USED_VLEN;
  options.socket_fd = sockfd;
  printf("Listening on socket %d\n", options.socket_fd);

  /* Initialize collector */
  unyte_udp_collector_t *collector = unyte_udp_start_collector_sk(&options);
  int recv_count = 0;
  int max = MAX_TO_RECEIVE;

  while (recv_count < max)
  {
    /* Read queue */
    void *seg_pointer = unyte_udp_queue_read(collector->queue);
    if (seg_pointer == NULL)
    {
      printf("seg_pointer null\n");
      fflush(stdout);
    }
    unyte_seg_met_t *seg = (unyte_seg_met_t *)seg_pointer;

    // printf("unyte_udp_get_version: %u\n", unyte_udp_get_version(seg));
    // printf("unyte_udp_get_space: %u\n", unyte_udp_get_space(seg));
    // printf("unyte_udp_get_encoding_type: %u\n", unyte_udp_get_encoding_type(seg));
    // printf("unyte_udp_get_header_length: %u\n", unyte_udp_get_header_length(seg));
    // printf("unyte_udp_get_message_length: %u\n", unyte_udp_get_message_length(seg));
    // printf("unyte_udp_get_generator_id: %u\n", unyte_udp_get_generator_id(seg));
    // printf("unyte_udp_get_message_id: %u\n", unyte_udp_get_message_id(seg));
    // printf("unyte_udp_get_src[family]: %u\n", unyte_udp_get_src(seg)->ss_family);
    // printf("unyte_udp_get_dest_addr[family]: %u\n", unyte_udp_get_dest_addr(seg)->ss_family);
    char ip_canonical[100];
    if (unyte_udp_get_src(seg)->ss_family == AF_INET) {
      printf("src IPv4: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_addr.s_addr, ip_canonical, sizeof ip_canonical));
      printf("src port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_port));
    } else {
      printf("src IPv6: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_addr.s6_addr, ip_canonical, sizeof ip_canonical));
      printf("src port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_port));
    }
    char ip_dest_canonical[100];
    if (unyte_udp_get_src(seg)->ss_family == AF_INET) {
      printf("dest IPv4: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_addr.s_addr, ip_dest_canonical, sizeof ip_dest_canonical));
      printf("dest port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_port));
    } else {
      printf("dest IPv6: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_addr.s6_addr, ip_dest_canonical, sizeof ip_dest_canonical));
      printf("dest port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_port));
    }
    // printf("unyte_udp_get_payload: %s\n", unyte_udp_get_payload(seg));
    // printf("unyte_udp_get_payload_length: %u\n", unyte_udp_get_payload_length(seg));

    /* Processing sample */
    recv_count++;
    print_udp_notif_header(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);

    fflush(stdout);

    /* Struct frees */
    unyte_udp_free_all(seg);
  }

  printf("Shutdown the socket\n");
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);

  // freeing collector mallocs
  unyte_udp_free_collector(collector);
  fflush(stdout);
  return 0;
}

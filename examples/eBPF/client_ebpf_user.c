/**
 * Example of client using a eBPF loadbalancer.
 * This allows multiple client instances listening on the same IP/port and receiving consistent data
 * (all packets from one src IP will always go on the same collector)
 *
 * Usage: ./client_ebpf_user <ip> <port> <index> <balancer_max>
 *
 * Example: launching 3 instances on the same ip port. The index is the index on the map to put the socket
 * and the balancer_max is how many max instances are in use.
 *
 *    ./client_ebpf_user 192.168.1.17 10001 0 3
 *    ./client_ebpf_user 192.168.1.17 10001 1 3
 *    ./client_ebpf_user 192.168.1.17 10001 2 3
 */

#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/bpf.h>
#include <linux/unistd.h>

#include "../../src/hexdump.h"
#include "../../src/unyte_udp_collector.h"
#include "../../src/unyte_udp_utils.h"
#include "../../src/unyte_udp_queue.h"
#include "../../src/unyte_udp_defaults.h"

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
 * Open socket, loads eBPF program and attaches it to the opened socket.
 * int socketfd : socket file descriptor to listen to.
 * uint32_t key : index of the socket to be filled in the eBPF hash table.
 * uint32_t balancer_count : max values to be used in eBPF reuse. Should be <= MAX_BALANCER_COUNT.
 */
int attach_ebpf_to_socket(int socketfd, uint32_t key, uint32_t balancer_count)
{
  int umap_fd, size_map_fd, prog_fd;
  char filename[] = BPF_KERNEL_PRG;
  int64_t usock = socketfd;
  long err = 0;

  assert(!balancer_count || key < balancer_count);
  assert(balancer_count <= MAX_BALANCER_COUNT);
  assert(usock >= 0);

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
    printf("Usage: ./client_ebpf_user <ip> <port> <index> <loadbalance_max>\n");
    printf("Example: ./client_ebpf_user 10.0.2.15 10001 0 5\n");
    exit(1);
  }

  printf("Listening on %s:%s\n", argv[1], argv[2]);

  // Create a udp socket with default socket buffer
  int socketfd = unyte_udp_create_socket(argv[1], argv[2], DEFAULT_SK_BUFF_SIZE);

  attach_ebpf_to_socket(socketfd, atoi(argv[3]), atoi(argv[4]));

  // Initialize collector options
  unyte_udp_options_t options = {0};
  options.recvmmsg_vlen = USED_VLEN;
  options.socket_fd = socketfd;
  printf("Listening on socket %d\n", options.socket_fd);

  /* Initialize collector */
  unyte_udp_collector_t *collector = unyte_udp_start_collector(&options);
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

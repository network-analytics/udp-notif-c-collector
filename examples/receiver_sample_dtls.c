#include <wolfssl/options.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "../src/hexdump.h"
#include "../src/unyte_udp_collector.h"

WOLFSSL_CTX*  ctx = NULL;
WOLFSSL*      ssl = NULL;
int           listenfd = INVALID_SOCKET;   /* Initialize our socket */

static void sig_handler(const int sig);
static void free_resources(void);

int main(int argc, char *argv[]){

    if (argc != 3)
    {
      printf("Error: arguments not valid\n");
      printf("Usage: ./client_sample <ip> <port>\n");
      exit(1);
    }

    int           exitVal = 1;
    struct sockaddr_in servAddr;        /* our server's address */
    struct sockaddr_in cliaddr;         /* the client's address */
    int           ret;
    int           err;
    int           recvLen = 0;    /* length of message */
    socklen_t     cliLen;
    char          buff[MAXLINE];   /* the incoming message */
    char          ack[] = "I hear you fashizzle!\n";


    if (wolfSSL_Init() != WOLFSSL_SUCCESS)
    {
        fprintf(stderr, "wolfSSL_Init error.\n");
        return exitVal;
    }

    wolfSSL_Debugging_ON();

    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_3_client_method())) == NULL)
    {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }

    if (wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, 0) != SSL_SUCCESS)
    {
        fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
        goto cleanup;
    }

    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS)
    {
        fprintf(stderr, "Error loading %s, please check the file.\n", servCertLoc);
        goto cleanup;
    }

    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS)
    {
        fprintf(stderr, "Error loading %s, please check the file.\n", servKeyLoc);
        goto cleanup;
    }

    // Create a udp socket with default socket buffer
    int socketfd = unyte_udp_create_socket(argv[1], argv[2], DEFAULT_SK_BUFF_SIZE);
    printf("Listening on %s:%s\n", argv[1], argv[2]);

    signal(SIGINT, sig_handler);

    // Initialize collector options
    unyte_udp_options_t options = {0};
    options.recvmmsg_vlen = USED_VLEN;
    options.socket_fd = socketfd; // passing socket file descriptor to listen to
    options.msg_dst_ip = false;   // destination IP not parsed from IP packet to improve performance

    /* Initialize collector */
    unyte_udp_collector_t *collector = unyte_udp_start_collector(&options);
    int recv_count = 0;
    int max = MAX_TO_RECEIVE;




    while (recv_count < max)
    {
        printf("Awaiting client connection on port %d\n", argv[2]);

        cliLen = sizeof(cliaddr);
        ret = (int)recvfrom(socketfd, (char *)&buff, sizeof(buff), MSG_PEEK, (struct sockaddr*)&cliaddr, &cliLen);

        if (ret < 0)
        {
            perror("recvfrom()");
            goto cleanup;
        }
        else if (ret == 0)
        {
            fprintf(stderr, "recvfrom zero return\n");
            goto cleanup;
        }

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL)
        {
            fprintf(stderr, "wolfSSL_new error.\n");
            goto cleanup;
        }

        if (wolfSSL_dtls_set_peer(ssl, &cliaddr, cliLen) != WOLFSSL_SUCCESS)
        {
            fprintf(stderr, "wolfSSL_dtls_set_peer error.\n");
            goto cleanup;
        }

        if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS)
        {
            fprintf(stderr, "wolfSSL_set_fd error.\n");
            break;
        }

        if (wolfSSL_accept(ssl) != SSL_SUCCESS)
        {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "SSL_accept failed.\n");
            goto cleanup;
        }
        showConnInfo(ssl);


        while (1) {
            if ((recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
                printf("heard %d bytes\n", recvLen);

                buff[recvLen] = '\0';
                printf("I heard this: \"%s\"\n", buff);
                hexdump(buff, recvLen);
            }
            else if (recvLen <= 0) {
                err = wolfSSL_get_error(ssl, 0);
                if (err == WOLFSSL_ERROR_ZERO_RETURN) /* Received shutdown */
                    break;
                fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                fprintf(stderr, "SSL_read failed.\n");
                goto cleanup;
            }
            printf("Sending reply.\n");
            if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
                err = wolfSSL_get_error(ssl, 0);
                fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                fprintf(stderr, "wolfSSL_write failed.\n");
                goto cleanup;
            }
        }
        printf("Awaiting new connection\n");









      /* Read queue */
      // void *seg_pointer = unyte_udp_queue_read(collector->queue);
      // if (seg_pointer == NULL)
      // {
      //   printf("seg_pointer null\n");
      //   fflush(stdout);
      // }
      // unyte_seg_met_t *seg = (unyte_seg_met_t *)seg_pointer;

      // printf("unyte_udp_get_version: %u\n", unyte_udp_get_version(seg));
      // printf("unyte_udp_get_space: %u\n", unyte_udp_get_space(seg));
      // printf("unyte_udp_get_media_type: %u\n", unyte_udp_get_media_type(seg));
      // printf("unyte_udp_get_header_length: %u\n", unyte_udp_get_header_length(seg));
      // printf("unyte_udp_get_message_length: %u\n", unyte_udp_get_message_length(seg));
      // printf("unyte_udp_get_observation_domain_id: %u\n", unyte_udp_get_observation_domain_id(seg));
      // printf("unyte_udp_get_message_id: %u\n", unyte_udp_get_message_id(seg));
      // printf("unyte_udp_get_src[family]: %u\n", unyte_udp_get_src(seg)->ss_family);
      //printf("unyte_udp_get_dest_addr[family]: %u\n", unyte_udp_get_dest_addr(seg) == NULL ? 0 : unyte_udp_get_dest_addr(seg)->ss_family); // NULL if options.msg_dst_ip is set to false (default)
      // char ip_canonical[100];
      // if (unyte_udp_get_src(seg)->ss_family == AF_INET) {
      //   printf("src IPv4: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_addr.s_addr, ip_canonical, sizeof ip_canonical));
      //   printf("src port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_port));
      // } else {
      //   printf("src IPv6: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_addr.s6_addr, ip_canonical, sizeof ip_canonical));
      //   printf("src port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_port));
      // }
      // Only if options.msg_dst_ip is set to true
      // if (unyte_udp_get_dest_addr(seg) != NULL) {
      //   char ip_dest_canonical[100];
      //   if (unyte_udp_get_dest_addr(seg)->ss_family == AF_INET) {
      //     printf("dest IPv4: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_addr.s_addr, ip_dest_canonical, sizeof ip_dest_canonical));
      //     printf("dest port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_port));
      //   } else {
      //     printf("dest IPv6: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_addr.s6_addr, ip_dest_canonical, sizeof ip_dest_canonical));
      //     printf("dest port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_port));
      //   }
      // }
      // printf("unyte_udp_get_payload: %.*s\n", unyte_udp_get_payload_length(seg), unyte_udp_get_payload(seg)); // payload may not be NULL-terminated
      // printf("unyte_udp_get_payload_length: %u\n", unyte_udp_get_payload_length(seg));

      /* Processing sample */
      // recv_count++;
      // print_udp_notif_header(seg->header, stdout);
      // // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
      //
      // fflush(stdout);
      //
      // /* Struct frees */
      // unyte_udp_free_all(seg);
    }

    printf("reply sent \"%s\"\n", ack);

    /* Attempt a full shutdown */
    ret = wolfSSL_shutdown(ssl);
    if (ret == WOLFSSL_SHUTDOWN_NOT_DONE)
        ret = wolfSSL_shutdown(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        err = wolfSSL_get_error(ssl, 0);
        fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
        fprintf(stderr, "wolfSSL_shutdown failed\n");
    }
    wolfSSL_free(ssl);
    ssl = NULL;





    printf("Shutdown the socket\n");
    close(*collector->sockfd);
    pthread_join(*collector->main_thread, NULL);

    // freeing collector mallocs
    unyte_udp_free_collector(collector);
    fflush(stdout);


    exitVal = 0;
cleanup:
    free_resources();
    wolfSSL_Cleanup();

    return exitVal;
}

static void sig_handler(const int sig)
{
    (void)sig;
    free_resources();
    wolfSSL_Cleanup();
}

static void free_resources(void)
{
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        ssl = NULL;
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (listenfd != INVALID_SOCKET) {
        close(listenfd);
        listenfd = INVALID_SOCKET;
    }
}

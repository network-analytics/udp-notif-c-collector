// #include <stdio.h>
// #include <stdlib.h>

// #include "../src/unyte_sender.h"

// #define MTU 1500
//
// int main(int argc, char *argv[])
// {
//   if (argc != 3)
//   {
//     printf("Error: arguments not valid\n");
//     printf("Usage: ./sender_sample <ip> <port>\n");
//     exit(1);
//   }
//
//   // Initialize collector options
//   unyte_sender_options_t options = {0};
//   options.address = argv[1];
//   options.port = argv[2];
//   options.default_mtu = MTU;
//   printf("Init sender on %s:%s | mtu: %d\n", options.address, options.port, MTU);
//
//   struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);
//
//   char *string_to_send = "Hello world1! Hello world2! Hello world3! Hello world4! Hello world5! Hello world6! Hello world7!";
//   unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
//   message->buffer = string_to_send;
//   message->buffer_len = 97;
//   // UDP-notif
//   message->version = 0;
//   message->space = 0;
//   message->media_type = 1; // json but sending string
//   message->observation_domain_id = 1000;
//   message->message_id = 2147483669;
//   message->used_mtu = 200; // use other than default configured
//   message->options = NULL;
//   message->options_len = 0; // should be initialized to 0 if no options are wanted
//
//   unyte_send(sender_sk, message);
//
//   // Freeing message and socket
//   free(message);
//   free_sender_socket(sender_sk);
//   return 0;
// }




// #include <stdio.h>
// #include <stdlib.h>

// #include "../src/unyte_sender.h"

// #define MTU 1500

// int main(int argc, char *argv[])
// {
//   if (argc != 3)
//   {
//     printf("Error: arguments not valid\n");
//     printf("Usage: ./sender_sample <ip> <port>\n");
//     exit(1);
//   }

//   // Initialize collector options
//   unyte_sender_options_t options = {0};
//   options.address = argv[1];
//   options.port = argv[2];
//   options.default_mtu = MTU;
//   printf("Init sender on %s:%s | mtu: %d\n", options.address, options.port, MTU);

//   struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);
//   int reuse = 0;

//   for(int i = 0; i < 5; i++){
//       if(i != 0){
//           reuse = 1;
//       }

//       char string_to_send[50];
//       sprintf((char *) string_to_send, "%s %d", "Message", i);
//       printf("MESSAGE 2 = %s\n", string_to_send);

//       unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));

//       message->buffer = string_to_send;
//       message->buffer_len = strlen(string_to_send);
//       printf("BUFFER LENGTH = %ld\n", strlen(string_to_send));
//       // UDP-notif
//       message->version = 0;
//       message->space = 0;
//       message->media_type = 1; // json but sending string
//       message->observation_domain_id = 1000;
//       message->message_id = 2147483669;
//       message->used_mtu = MTU; // use other than default configured
//       message->options = NULL;
//       message->options_len = 0; // should be initialized to 0 if no options are wanted

//       //int res = unyte_send_with_dtls_context(sender_sk, message, reuse);
//       int res = unyte_send(sender_sk, message);

//       printf("RETURNED VALUE = %d\n", res);
//       printf("SORTIE\n");
//       free(message);
//   }
//   printf("avant free\n");
//   free_sender_socket(sender_sk);
//   printf("apres sortie\n");
//   return 0;
// }



//---------- PARTIE DTLS INTEGREE ----------
#include <wolfssl/options.h>
#include <unistd.h>
#include <wolfssl/ssl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/unyte_sender.h"


#define MTU 1500



int main(int argc, char *argv[])
{
  printf("\n");
  if (argc != 4)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./sender_sample <ip> <port> <ca_cert_name>\n"); //on oublie le SERV_PORT dans le dtls-common
    exit(1);
  }
  char * temp_certificats = "../certs/";

  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = argv[1];
  options.port = argv[2];
  options.default_mtu = MTU;

  printf("Init sender on %s:%s | mtu: %d\n", options.address, options.port, MTU);
  
  char * caCertLoc_name = argv[3];
  char caCertLoc[100];
  strcpy(caCertLoc, temp_certificats);
  strcat(caCertLoc, caCertLoc_name);
  options.caCertLoc = caCertLoc;

  //struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);
  struct unyte_sender_socket *sender_sk = unyte_start_sender_dtls(&options);
  for(int i = 0; i < 5; i++){

      char string_to_send[50];
      sprintf((char *) string_to_send, "%s %d", "Message", i);
      printf("message to send = %s, len = %ld\n", string_to_send, strlen(string_to_send));

      unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));

      message->buffer = string_to_send;
      message->buffer_len = strlen(string_to_send);

      // UDP-notif
      message->version = 0;
      message->space = 0;
      message->media_type = 1; // json but sending string
      message->observation_domain_id = 1000;
      message->message_id = 2147483669;
      message->used_mtu = MTU; // use other than default configured
      message->options = NULL;
      message->options_len = 0; // should be initialized to 0 if no options are wanted

      //int res = unyte_send(sender_sk, message);
      unyte_send_dtls(sender_sk, message);

      free(message);
  }
  free_sender_socket(sender_sk);
  printf("\n");
  return 0;
}
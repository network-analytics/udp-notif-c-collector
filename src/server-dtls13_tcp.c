#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define MAX 1000
#define PORT 5000
#define SA struct sockaddr
   
// Function designed for chat between client and server.
void* func_thread(void* connfd_p)
{
    int connfd = (intptr_t)connfd_p;
    printf("conn fd = %d, %p\n", connfd, connfd_p);
    char buff[MAX];
    int n;
    // infinite loop for chat
    for (;;) {
        bzero(buff, MAX);
   
        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));
        // print buffer which contains the client contents
        printf("From client: %s\t To client : %s\n", buff, buff);
        // bzero(buff, MAX);
        // n = 0;
        // // copy server message in the buffer
        // while ((buff[n++] = getchar()) != '\n')
        //     ;
   
        // and send that buffer to client
        write(connfd, buff, sizeof(buff));
        printf("Buff transfert = %s\n", buff);
   
        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
    pthread_exit(NULL);
}
   
// Driver function
int main(int argc, char *argv[])
{
    if (argc != 3){
        perror("Nombre d'arguments incorrect");
        exit(1);
    }

    int sockfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;
   
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    inet_aton(argv[1], &servaddr.sin_addr);
    servaddr.sin_port = htons(atoi(argv[2]));
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);
    printf("ok\n");
   
    while(1){
        // Accept the data packet from client and verification
        printf("waiting for connection...\n");
        connfd = accept(sockfd, (SA*)&cli, &len);
        printf("ok\n");
        if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accept the client...\n");
        printf("ok\n");
        // Function for chatting between client and server
        pthread_t thread;
        int res = pthread_create(&thread, NULL, func_thread, (void*) connfd);
        if(res < 0){
            perror("pb dans le thread");
            exit(1);
        }
    }
   
    // After chatting close the socket
    close(sockfd);
    return 0;
}

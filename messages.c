#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "messages.h"

int sockfd;
char broadcastIP[16] = "255.255.255.255";
FILE* log_file;


/**
 * The function "add_to_log" writes the given text to a log file and saves the changes to disk.
 * 
 * @param text The "text" parameter is a pointer to a character array (string) that contains the text
 * to be added to the log file, for now only messages.
 */
void add_to_log(char* text){
    fprintf(log_file, "%s\n", text);
    // Save changes 
    fflush(log_file);
}

void close_client(){
    close(sockfd);
    fclose(log_file);
}

/**
 * The function `send_message` sends a UDP datagram containing a message, along with the sender's
 * nickname, IP address, and port number.
 * 
 * @param nickname The nickname is a string that represents the name or alias of the sender of the
 * message. 
 * @param ip The "ip" parameter in the "send_message" function is the IP address of the recipient to
 * whom you want to send the message.
 * @param port The `port` parameter is the port number on which the message will be sent. It is an
 * integer value that specifies the destination port for the UDP datagram.
 * @param message The `message` parameter is a string that represents the content of the message that
 * you want to send. 
 * 
 * @return an integer value of 1.
 */
int send_message(char* nickname, char* ip, int port, char* message){
    struct sockaddr_in si_broadcast;
    char send_buf[MAX_BUFFER_SIZE];
    // Формирование UDP-датаграммы
    sprintf(send_buf, "%s(%s):%s",  nickname, ip, message);

    // Отправка сообщения
    si_broadcast.sin_family = AF_INET;
    si_broadcast.sin_port = htons(port);
    si_broadcast.sin_addr.s_addr = inet_addr(broadcastIP);
    if (sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&si_broadcast, sizeof(si_broadcast)) == -1) {
        perror("sendto");
        exit(-1);
    }
    return 1;
}

/**
 * The function `accept_message` receives a message from a socket and stores it in a buffer.
 * 
 * @param buffer The buffer is a pointer to a character array where the received message will be
 * stored.
 * 
 * @return The function `accept_message` returns the number of bytes read from the socket.
 */
int accept_message(char* buffer){
    struct sockaddr_in si_other;
    socklen_t si_other_len = sizeof(si_other);
    int bytesRead = 0;

    bytesRead = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&si_other, &si_other_len);
    if (bytesRead == -1) {
        perror("recvfrom");
        exit(-1);
    }

    return bytesRead;
}

/**
 * The function `init_client` initializes a client by creating a socket, setting socket options for
 * broadcasting, configuring the server address, and binding the socket to the specified port.
 * 
 * @param port The "port" parameter is the port number on which the client socket will be bound. It is
 * the port number that the client will use to communicate with the server.
 * 
 * @return an integer value. If the function is successful, it will return 0. If there is an error, it
 * will return 1.
 */
int init_client(int port){
    // Socket creation
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up socket parametrs for broadcasting
    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(EXIT_FAILURE);
    }

    // Set up server
    struct sockaddr_in si_me;
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr*)&si_me, sizeof(si_me)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    // File to store all chat messages
    log_file = fopen(LOG_FILENAME, "a");

    return 0;
}
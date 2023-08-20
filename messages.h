#ifndef _CLIENT_H_
#define _CLIENT_H_

#define MAX_BUFFER_SIZE 1000
#define LOG_FILENAME "logs.txt"

void add_to_log(char* text);
int send_message(char* nickname, char* ip, int port, char* message);
int accept_message(char* buffer);
int init_client(int port);
void close_client();

#endif
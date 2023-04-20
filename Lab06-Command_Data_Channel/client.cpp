#include <cerrno>
#include <iostream>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>

#define OPEN_MAX 1024
#define MSG_SIZE 550
#define MY_CONNECTION 55
#define GARBAGE_SIZE 50000
 
int data[MY_CONNECTION];
int command;

void sig_term(int signo)
{
    write(command, "/report", strlen("/report"));
    char msg[1000];
    memset(msg, '\0', 1000);
    read(command, msg, 1000);
    write(STDOUT_FILENO, msg, strlen(msg));

    close(command);
    for (int i = 0; i < MY_CONNECTION; i++) close(data[i]);

    exit(1);
}

int main(int argc, char **argv)
{
    if (argc < 2) 
    {
        printf("Usage: ./a.out [port]\n");
        return -1;
    }
    
    sockaddr_in servaddr_c, servaddr_d;
    char input_addr[] = "127.0.0.1";

    servaddr_c.sin_family = AF_INET;
    servaddr_d.sin_family = AF_INET;
    servaddr_c.sin_port = htons(atoi(argv[2]));
    servaddr_d.sin_port = htons(atoi(argv[2])+1);

    signal(SIGINT, sig_term);
    signal(SIGTERM, sig_term);

    if (inet_pton(AF_INET, input_addr, &servaddr_c.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    command = socket(AF_INET, SOCK_STREAM, 0);
    connect(command, (struct sockaddr*)&servaddr_c, sizeof(servaddr_d));
    if (command < 0) {
        printf("command error\n");
        return -1;
    }

    if (inet_pton(AF_INET, input_addr, &servaddr_d.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    for (int i = 0; i < MY_CONNECTION; i++)
    {
        data[i] = socket(AF_INET, SOCK_STREAM, 0);
        connect(data[i], (struct sockaddr*)&servaddr_d, sizeof(servaddr_d));
        if (data[i] < 0) {
            printf("command error\n");
            return -1;
        }
    }

    // send reset
    write(command, "/reset", strlen("/reset"));
    char reset_msg[1000];
    memset(reset_msg, '\0', 1000);
    read(command, reset_msg, 1000);
    write(STDOUT_FILENO, reset_msg, strlen(reset_msg));

    char garbage[GARBAGE_SIZE];
    memset(garbage, 'A', GARBAGE_SIZE);

    while (true)
    {
        for (int i = 0; i < MY_CONNECTION; i++) 
            write(data[i], garbage, GARBAGE_SIZE);
    }

}

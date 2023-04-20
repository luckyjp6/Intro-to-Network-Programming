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

using namespace std;
 
int maxi, num_user;
long long int byte_counter = 0;
pollfd client[OPEN_MAX];

struct Client_info {
    int connfd;
    sockaddr_in addr;
}; 

void init() {
    maxi = 1; num_user = 0;
    for (int i = 1; i < OPEN_MAX; i++) client[i].fd = -1; /* -1: available entry */
}

int my_connect(int &listenfd, int port, sockaddr_in &servaddr) {
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));
    
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);//INADDR_ANY);
	servaddr.sin_port        = htons(port);

	if (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
    {
		printf("failed to bind\n");
		return 0;
	}

	listen(listenfd, 1024);

    return 1;
}

int time_diff(timeval *start, timeval *end) {
    return end->tv_sec - start->tv_sec + 0.0;
}

double my_time(timeval t) {
    return t.tv_sec + 1e-6*t.tv_usec;
}

void handle_new_connection(const int listenfd, Client_info new_client) {
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    
    int connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);

    // save descriptor
    int i;
    for (i = 2; i < OPEN_MAX; i++)
    {
        if (client[i].fd < 0) 
        {
            client[i].fd = connfd;
            new_client.addr = cliaddr;
            new_client.connfd = connfd;
            break;
        }
    }

    if (i == OPEN_MAX) 
    {
        printf("too many clients\n");
        return;
    }

    client[i].events = POLLRDNORM;
    if (i > maxi) maxi = i;

    num_user++;
}

void close_client(int index) {
    int connfd = client[index].fd;

    close(connfd);
    client[index].fd = -1;
    num_user--;
}

int main(int argc, char **argv) {
	if (argc < 2) {
        printf("Usage: ./a.out [port]\n");
        return -1;
    }
    
    int					listen_command, listen_data, connfd;
	pid_t				childpid;
	sockaddr_in	        servaddr;
    int	                i, nready;

    init();
    
    if (my_connect(listen_command, atoi(argv[1]), servaddr) == 0) return -1;
    client[0].fd = listen_command;
    client[0].events = POLLRDNORM;
    if (my_connect(listen_data, atoi(argv[1])+1, servaddr) == 0) return -1;    
    client[1].fd = listen_data;
    client[1].events = POLLRDNORM;

    Client_info tmp_client;
    timeval start, now;
    gettimeofday(&start, NULL); 
	for ( ; ; ) {
        nready = poll(client, maxi+1, -1);
        
        // new client
        if (client[0].revents & POLLRDNORM) {
            handle_new_connection(listen_command, tmp_client);
            nready--;
        }
        if (client[1].revents & POLLRDNORM) {
            handle_new_connection(listen_data, tmp_client);
            nready--;
        }
        
        // check all clients
		int sockfd, n;
        char buf[MSG_SIZE]; 
        
        for (i = 2; i <= maxi; i++) {
            if ((sockfd = client[i].fd) < 0) continue;
            if (client[i].revents & (POLLRDNORM | POLLERR)) {
                // read input
                memset(buf, '\0', MSG_SIZE);

                if ( (n = read(sockfd, buf, MSG_SIZE-50)) < 0) close_client(i); /* connection reset by client */
                else if (n == 0) close_client(i); /* connection closed by client */
                else { /* read command */
                    int buf_len = strlen(buf);
                    char *command = strtok(buf, " \n\r");
                    
                    if (strcmp(command, "/reset") == 0) {
                        char msg[MSG_SIZE];
                        gettimeofday(&start, NULL);
                        
                        sprintf(msg, "%f RESET %lld\n", my_time(start), byte_counter);
                        write(sockfd, msg, strlen(msg));
                        byte_counter = 0;
                    }
                    else if (strcmp(command, "/ping") == 0){
                        char msg[MSG_SIZE];
                        gettimeofday(&now, NULL);

                        sprintf(msg, "%f PONG\n", my_time(now));
                        write(sockfd, msg, strlen(msg));
                    }
                    else if (strcmp(command, "/report") == 0) {
                        char msg[MSG_SIZE];
                        gettimeofday(&now, NULL);
                        
                        int time_interval = now.tv_sec - start.tv_sec;
                        
                        double sec = 8.0 * (byte_counter/1000000.0)/time_interval;
                        sprintf(msg, "%f REPORT %lld %d %f\n", my_time(now), byte_counter, time_interval, sec);
                        write(sockfd, msg, strlen(msg));
                    }
                    else if (strcmp(command, "/clients") == 0) {
                        char msg[MSG_SIZE];
                        gettimeofday(&now, NULL);
                        
                        sprintf(msg, "%f CLIENTS %d\n", my_time(now), num_user/2);
                        write(sockfd, msg, strlen(msg));
                    }
                    else byte_counter += buf_len;
                }

                if (--nready <= 0) break; /* no more readable descs */
            }
        }
	}
    // parent closes connected socket
    close(connfd);
}
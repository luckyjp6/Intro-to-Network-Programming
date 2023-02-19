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
#include "name.h"

#define OPEN_MAX 1024
#define MSG_SIZE 300

struct Client_info {
    char name[25];
    sockaddr_in addr;
};

struct broadcast_msg{
    int except;
    char msg[MSG_SIZE];
    int msg_size;
    void set(int e) { //, char m[MSG_SIZE], int s) {
        except = e;
        msg_size = sizeof(msg);
        // memset(msg, '\0', sizeof(msg));
        // strcpy(msg, m);
    }
};

int maxi = 0, nready, i;
int num_user = 0;
Client_info client_info[OPEN_MAX];
pollfd client[OPEN_MAX];
std::vector<broadcast_msg> b_msg;

void err_sys(const char *err) {
    perror(err);
    exit(1);
}

void time_format(char* tt, int tt_size)
{  
    tm* cur;
    time_t curtime;
    time(&curtime);
    cur = localtime(&curtime);
    
    memset(tt, '\0', tt_size);
    sprintf(tt, "%-4d-%-2d-%-2d %-2d:%-2d:%-2d ",cur->tm_year+1900, cur->tm_mon+1, cur->tm_mday, cur->tm_hour, cur->tm_min, cur->tm_sec);    
}

void close_client(int index) {
    int connfd = client[index].fd;

    close(connfd);
    client[index].fd = -1;
    num_user--;

    broadcast_msg tmp;
    memset(tmp.msg, '\0', MSG_SIZE);
    sprintf(tmp.msg, "*** User <%s> has left the server\n", client_info[index].name);
    tmp.set(-1);
    b_msg.push_back(tmp);

    printf("* client %s:%d disconnected\n", inet_ntoa(client_info[index].addr.sin_addr), client_info[index].addr.sin_port);
}

// print the current time to specific connection
void write_time(int index) {
    time_t curtime;
    time(&curtime);

    char tt[30];
    time_format(tt, sizeof(tt));

    int flg = 0;
    int connfd = client[index].fd;
    if (connfd < 0) return;

    flg = send(connfd, tt, sizeof(tt)-1, MSG_NOSIGNAL);
    
    if (flg < 0) close_client(index);
}

void broadcast() {
    for (int i = 0; i < b_msg.size(); i++) { 
        for (int j = 1; j <= maxi; j++) {
            
            if (j == b_msg[i].except) continue;
            
            write_time(j);
            if (client[j].fd < 0) continue;

            if (send(client[j].fd, b_msg[i].msg, b_msg[i].msg_size, MSG_NOSIGNAL) < 0) close_client(j);
        }
    }
    b_msg.clear();
}

void error_cmd(char *cmd, int to) {
    char msg[MSG_SIZE];
    write_time(to);
    memset(msg, '\0', sizeof(msg));
    sprintf(msg, "***Unknown or incomplete command <%s>\n", cmd);
    write(client[to].fd, msg, sizeof(msg));
}

int main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	sockaddr_in	        cliaddr, servaddr;
	socklen_t			clilen = sizeof(cliaddr);
	void				sig_chld(int);

    if (argc < 2) {
        printf("Usage: ./a.out [port]\n");
        return -1;
    }

	int port_num = 0;
    sscanf(argv[1], "%d", &port_num);

    // set server sockaddr_in
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);
	servaddr.sin_port        = htons(port_num);

	if (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		printf("failed to bind\n");
		return -1;
	}

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
    listen(listenfd, 1024);

    // reset each entry (used for poll)
    for (i = 0; i < OPEN_MAX; i++)
        client[i].fd = -1; /* -1: available entry */
    
    // add event
    client[0].fd = listenfd; // the socket that listen for new connection
    client[0].events = POLLRDNORM;

	while (1) {
        // wait for any event happen
        nready = poll(client, maxi+1, -1);
        
        // new client
        if (client[0].revents & POLLRDNORM) {
            connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);
            num_user ++;

            // save descriptor
            for (i = 1; i < OPEN_MAX; i++)
                if (client[i].fd < 0) {
                    client[i].fd = connfd; 
                    client_info[i].addr = cliaddr;
                    break;
                }
            if (i == OPEN_MAX) {
                printf("too many clients\n");
                continue;
            }

            // print welcome message
            char welcome_msg[] = "*** Welcome to the simple CHAT server\n";
            write_time(i);
            write(connfd, welcome_msg, sizeof(welcome_msg));
            
            write_time(i);
            char nick_name[25];
            sprintf(nick_name, "%s %s", adjs[rand()%800], animals[rand()%800]); // random nickname
            memset(client_info[i].name, ' ', sizeof(nick_name));
            strcpy(client_info[i].name, nick_name);
       
            char msg[MSG_SIZE];
            memset(msg, '\0', sizeof(msg));
            sprintf(msg, "Total %d users online now. Your name is <%s>\n", num_user, nick_name);
            write(connfd, msg, sizeof(msg));

            // set broadcast message
            broadcast_msg announce;
            memset(announce.msg, '\0', MSG_SIZE);
            sprintf(announce.msg, "*** User <%s> has just landed on the server\n", nick_name);
            announce.set(i);
            b_msg.push_back(announce);
          
            client[i].events = POLLRDNORM;
            if (i > maxi) maxi = i;

            printf("* client connected from %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);

            if (--nready <= 0) {
                broadcast();
                continue;
            }
        }
        
        // check all clients
		int sockfd, n;
        char buf[MSG_SIZE-50];
        
        for (i = 1; i <= maxi; i++) {
            if ( (sockfd = client[i].fd) < 0) continue; /* empty entry */
            // got an event
            if (client[i].revents & (POLLRDNORM | POLLERR)) { 
                memset(buf, '\0', sizeof(buf));
                
                // read input 
                if ( (n = read(sockfd, buf, MSG_SIZE-50)) < 0) { /* got error*/
                    if (errno == ECONNRESET) close_client(i); /* connection reset by client */
                    else err_sys("read error");
                } 
                else if (n == 0) close_client(i); /* connection closed by client */
                else { /* read command */
                    if (buf[0] != '/') {
                        broadcast_msg tmp;
                        memset(tmp.msg, '\0', MSG_SIZE);
                        sprintf(tmp.msg, "*** User <%s> %s", client_info[i].name, buf);
                        tmp.set(i);
                        b_msg.push_back(tmp);
                        if (--nready <= 0) break;
                        else continue;
                    }
                        
                    const char *d = " \n";
                    char *command;
                    command = strtok(buf, d);

                    if (strcmp(command, "/name") == 0) {
                        char *nick_name;
                        nick_name = strtok(NULL, d);
                        if (nick_name == NULL) {
                            error_cmd(buf, i);
                            if (--nready <= 0) break;
                            else continue;
                        }

                        write_time(i);
                        char rename_msg[MSG_SIZE];
                        memset(rename_msg, '\0', sizeof(rename_msg));
                        sprintf(rename_msg, "*** Nickname changed to <%s>\n", nick_name);
                        write(client[i].fd, rename_msg, sizeof(rename_msg));
                        
                        broadcast_msg tmp;
                        memset(tmp.msg, '\0', MSG_SIZE);
                        sprintf(tmp.msg, "*** User <%s> renamed to <%s>\n", client_info[i].name, nick_name);
                        tmp.set(i);
                        b_msg.push_back(tmp);
                        
                        memset(client_info[i].name, ' ', sizeof(client_info[i].name));
                        strcpy(client_info[i].name, nick_name);
                    }
                    else if (strcmp(command, "/who") == 0) {
                        char line[] = "--------------------------------------------------\n";
                        write(client[i].fd, line, sizeof(line));
                        for (int u = 1; u <= maxi; u++) {
                            if (client[u].fd < 0) continue;
                            char info[MSG_SIZE], tag[2];
                            
                            if (u == i) strcpy(tag, "*");
                            else strcpy(tag, " ");

                            memset(info, '\0', sizeof(info));
                            sprintf(info, "%s %-20s %s:%d\n", tag, client_info[u].name, inet_ntoa(client_info[u].addr.sin_addr), client_info[u].addr.sin_port);
                            write(client[i].fd, info, sizeof(info));
                        }
                        write(client[i].fd, line, sizeof(line));
                    }
                    else {
                        error_cmd(buf, i);
                        if (--nready <= 0) break;
                        else continue;
                    }
                }
                
                if (--nready <= 0) break; /* no more readable descs */
            }
        }
		
        broadcast();
		// close(connfd);			/* parent closes connected socket */
	}
}
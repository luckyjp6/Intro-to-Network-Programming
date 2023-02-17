#ifndef FUNSTIONS_H
#define FUNSTIONS_H

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
#define NAME_LENGTH 10
#define SERVER_NAME "jp6jp6"

struct Client_info 
{
    char nick_name[NAME_LENGTH];
    char user_name[NAME_LENGTH];
    char real_name[NAME_LENGTH];
    sockaddr_in addr;

    void reset()
    {
        memset(nick_name, '\0', NAME_LENGTH);
        memset(user_name, '\0', NAME_LENGTH);
        memset(real_name, '\0', NAME_LENGTH);
    }
}; 

// use map to impliment the channel
struct channel_info
{
    bool is_private = false;
    char* key;
    std::string topic;
    std::vector<std::string> ban_list;
    std::vector<int> connected; // connfd
};

extern int maxi, num_user;
extern Client_info client_info[OPEN_MAX];
extern pollfd client[OPEN_MAX];
extern std::map<std::string, channel_info> channels;

void init();

void close_client(int index);

#endif
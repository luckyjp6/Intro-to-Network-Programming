#include "functions.h"

int maxi, num_user;
Client_info client_info[OPEN_MAX];
pollfd client[OPEN_MAX];
std::map<std::string, channel_info> channels;

void init() 
{
    maxi = 0; num_user = 0;
    for (auto &c:client) c.fd = -1; /* -1: available entry */
    for (auto &c:client_info) c.reset();
}

void close_client(int index) 
{
    int connfd = client[index].fd;

    close(connfd);
    client[index].fd = -1;
    num_user--;

    client_info[index].reset();
}


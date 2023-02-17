#include "functions.h"

void no_such_nick_channel(int index, std::string channel)
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 401 %s %s :No such nick/channel\n", SERVER_NAME, client_info[index].nick_name, channel.data());
    write(client[index].fd, error, strlen(error));
}

void no_such_channel(int index, std::string channel)
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 403 %s %s :No such channel\n", SERVER_NAME, client_info[index].nick_name, channel.data());
    write(client[index].fd, error, strlen(error));
}

void no_origin(int index)
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 409 %s :No origin specified\n", SERVER_NAME, client_info[index].nick_name);
    write(client[index].fd, error, strlen(error));
}

void no_recipient( int index, char *command)
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 411 %s :No recipient given (%s)\n", SERVER_NAME, client_info[index].nick_name, command);
    write(client[index].fd, error, strlen(error));
}

void no_text_send(int index)
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 412 %s :No text to send\n", SERVER_NAME, client_info[index].nick_name);
    write(client[index].fd, error, strlen(error));
}

void error_cmd(int index, char *command) 
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 421 %s %s :Unknown command\n", SERVER_NAME, client_info[index].nick_name, command);
    write(client[index].fd, error, strlen(error));
}

bool check_conflict_nick(char *nickname)
{
    for (int i = 1; i <= maxi; i++)
    {
        if (client[i].fd < 0) continue;
        if (strcmp(client_info[i].nick_name, nickname) == 0) return true;
    }
    
    return false;
}

bool check_nick_name(int index, char *nick_name, bool new_client) 
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    
    // no nickname
    if (nick_name == NULL) {
        sprintf(error, ":%s 431 : No nickname given\n", SERVER_NAME);
        write(client[index].fd, error, strlen(error));
        return true;
    }
    
    // invalid nickname
    if (strlen(nick_name) > 9)
    {
        sprintf(error, ":%s 432 %s :Erroneus nickname\n", SERVER_NAME, nick_name);
        write(client[index].fd, error, strlen(error));
        return true;
    }

    // check conflict nickname
    if (check_conflict_nick(nick_name)) 
    {
        if (new_client)
        {
            sprintf(error, ":%s 436 %s :Nickname collision KILL\n", SERVER_NAME, nick_name);
            write(client[index].fd, error, strlen(error));
        }
        else 
        {
            sprintf(error, ":%s 433 %s :Nickname is already in use\n", SERVER_NAME, nick_name);
            write(client[index].fd, error, strlen(error));
        }
        
        return true;
    }

    return false;
}

// void nickname_in_use(int index, char *nick_name)
// {
//     char error[MSG_SIZE];
//     memset(error, '\0', MSG_SIZE);
//     sprintf(error, ":%s 433 %s :Nickname is already in use\n", SERVER_NAME, nick_name);
//     write(client[index].fd, error, strlen(error));
// }

void not_on_channel(int index, std::string channel)
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 442 %s %s :You're not on that channel\n", SERVER_NAME, client_info[index].nick_name, channel.data());
    write(client[index].fd, error, strlen(error));
}

void not_registered(int index) 
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 451 :You have not registered\n", SERVER_NAME);
    write(client[index].fd, error, strlen(error));
}

void not_registered(int index, std::string nick_name) 
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 451 %s :You have not registered\n", SERVER_NAME, nick_name.data());
    write(client[index].fd, error, strlen(error));
}

void not_enough_args(int index, char *command) 
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 461 %s :Not enough parameters\n", SERVER_NAME, command);
    write(client[index].fd, error, strlen(error));
}

void reregister_error(int index)
{
    char error[MSG_SIZE];
    memset(error, '\0', MSG_SIZE);
    sprintf(error, ":%s 462 %s :You may not reregister\n", SERVER_NAME, client_info[index].nick_name);
    write(client[index].fd, error, strlen(error));
}

bool check_user_in_channel(int index, std::string channel)
{
    for (auto u: channels[channel].connected)
    {
        if (u == index) return true;
    }
    return false;
}
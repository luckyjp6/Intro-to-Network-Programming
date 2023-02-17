#include "functions.h"
#include "print_msg.h"
#include "error_func.h"

void welcome_new_client(int index) 
{
    int connfd = client[index].fd;
    char *nick_name = client_info[index].nick_name;

    char welcome_msg[MSG_SIZE];
    memset(welcome_msg, '\0', MSG_SIZE);
    sprintf(welcome_msg, ":%s 001 %s :Welcom to worm's IRC server!\n", SERVER_NAME, nick_name);
    write(connfd, welcome_msg, strlen(welcome_msg));

    memset(welcome_msg, '\0', MSG_SIZE);
    sprintf(welcome_msg, ":%s 251 %s There are %d users and 0 invisible on 1 servers\n", SERVER_NAME, nick_name, num_user);
    write(connfd, welcome_msg, strlen(welcome_msg));

    // start of motd
    memset(welcome_msg, '\0', MSG_SIZE);
    sprintf(welcome_msg, ":%s 375 %s :- %s Message of the day -\n", SERVER_NAME, nick_name, SERVER_NAME);
    write(connfd, welcome_msg, strlen(welcome_msg));

    // motd body
    char cool_msg[8][100] = {
        " __       __       __   ______    __ __    ____  ____",
        " \\ \\     /  \\     / /  /  __  \\   | '__/  / __ `' __  \\",
        "  \\ \\   / /\\ \\   / /  /  /  \\  \\  | |    |  | |  |  |  |",
        "   \\ \\_/ /  \\ \\_/ /   \\  \\__/  /  | |    |  | |  |  |  |",
        "    \\___/    \\___/     \\______/   |_|    |__| |__|  |__|",
        "{\\__/}" ,
        "( • - •)",
        "/ > $$"
    };
    for (int i = 0; i < 8; i++) {
        memset(welcome_msg, '\0', MSG_SIZE);
        sprintf(welcome_msg, ":%s 372 %s :- %s\n", SERVER_NAME, nick_name, cool_msg[i]);
        write(connfd, welcome_msg, strlen(welcome_msg));
    }
    

    // end of motd
    memset(welcome_msg, '\0', MSG_SIZE);
    sprintf(welcome_msg, ":%s 376 %s :- End of message of the day -\n", SERVER_NAME, nick_name);
    write(connfd, welcome_msg, strlen(welcome_msg));
}

void print_ping(int connfd, char *host) 
{
    char msg[MSG_SIZE];
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, "PONG\n"); // ":%s PONG :%s\n", SERVER_NAME, host);
    write(connfd, msg, strlen(msg));
}

void print_join(int index, char *c) // std::vector<std::string> join_channel)
{   
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];
    strcpy(nick_name, client_info[index].nick_name);
    
    char msg[MSG_SIZE];

    if (channels.find(c) == channels.end())
    {
        channel_info tmp;
        tmp.topic = "";
        channels[c] = tmp;
        channels[c].connected.push_back(index);
    }
    else
    {
        bool in = false;
        for (auto c:channels[c].connected)
        {
            if (c == index)
            {
                in = true;
                break;
            }
        }
        if (!in) channels[c].connected.push_back(index);
    }

    // join the channel
    
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s JOIN %s\n", nick_name, c);
    write(connfd, msg, strlen(msg));
    
    // show the topic
    memset(msg, '\0', MSG_SIZE);
    if (channels[c].topic.size() == 0) 
        sprintf(msg, ":%s 331 %s %s :No topic is set\n", SERVER_NAME, nick_name, c);
    else
        sprintf(msg, ":%s 332 %s %s :%s\n", SERVER_NAME, nick_name, c, channels[c].topic.data());
    write(connfd, msg, strlen(msg));

    print_channel_users(index,  c);
}

void print_part(int index, std::string channel_name) 
{
    int connfd = client[index].fd;

    if (channels.find(channel_name) == channels.end())
    {
        no_such_channel(index, channel_name);
        return;
    }

    bool in = false;
    for (std::vector<int>::iterator c = channels[channel_name].connected.begin(); c != channels[channel_name].connected.end(); ++c)
    {
        if (*c == index)
        {
            in = true;
            channels[channel_name].connected.erase(c);
            break;
        }
    }
    
    if (!in) 
    {
        not_on_channel(index, channel_name);
        return;
    }

    if (channels[channel_name].connected.size() == 0) channels.erase(channel_name);
    
    char msg[MSG_SIZE];
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s PART :%s\n", client_info[index].nick_name, channel_name.data());
    write(connfd, msg, strlen(msg));
}

void print_topic(int index, std::string topic, std::string channel_name) 
{
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];
    strcpy(nick_name, client_info[index].nick_name);
    char msg[MSG_SIZE];
    memset(msg, '\0', MSG_SIZE);
    
    bool in = false;
    for (std::vector<int>::iterator c = channels[channel_name].connected.begin(); c != channels[channel_name].connected.end(); ++c)
    {
        if (*c == index)
        {
            in = true;
            break;
        }
    }

    if (!in)
    {
        not_on_channel(index, channel_name);
        return; 
    }

    if (topic[topic.size()-1] == '\n') topic.erase(topic.end()-1);
    
    if (topic.size() > 0) channels[channel_name].topic = topic;

    if (channels[channel_name].topic.size() > 0)
        sprintf(msg, ":%s 332 %s %s :%s\n", SERVER_NAME, nick_name, channel_name.data(), channels[channel_name].topic.data());
    else 
        sprintf(msg, ":%s 331 %s %s :No topic is set\n", SERVER_NAME, nick_name, channel_name.data());

    write(connfd, msg, strlen(msg));
}

void print_all_users(int index)
{
    char msg[MSG_SIZE];
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];
    strcpy(nick_name, client_info[index].nick_name);

    // start
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 392 %s :%-8s %-9s %-8s\n", SERVER_NAME, nick_name, "UserID", "Terminal", "Host");
    write(connfd, msg, strlen(msg));

    // body
    for (int i = 1; i <= maxi; i++)
    {
        if (client[i].fd < 0) continue;

        memset(msg, '\0', MSG_SIZE);
        sprintf(msg, ":%s 393 %s :%-8s %-9s %-8s\n", SERVER_NAME, nick_name, client_info[i].nick_name, "-", inet_ntoa(client_info[i].addr.sin_addr));
        write(connfd, msg, strlen(msg));
    }

    // end
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 394 %s :End of users\n", SERVER_NAME, nick_name);
    write(connfd, msg, strlen(msg));
}

void print_channel_users(int index)
{
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];
    strcpy(nick_name, client_info[index].nick_name);

    if (channels.size() == 0)
    {
        char msg[MSG_SIZE];
        memset(msg, '\0', MSG_SIZE);
        sprintf(msg, ":%s 366 %s :End of Names List\n", SERVER_NAME, nick_name);
        write(connfd, msg, strlen(msg));
        return;
    }

    for (auto channel : channels)
    {
        char msg[MSG_SIZE];
        memset(msg, '\0', MSG_SIZE);
        sprintf(msg, ":%s 353 %s %s :", SERVER_NAME, nick_name, channel.first.data());
        for (auto user : channel.second.connected)
        {            
            strcat(msg, client_info[user].nick_name);
            strcat(msg, " ");
        }        
        strcat(msg, "\n");
        write(connfd, msg, strlen(msg));

        memset(msg, '\0', MSG_SIZE);
        sprintf(msg, ":%s 366 %s %s :End of Names List\n", SERVER_NAME, nick_name, channel.first.data());
        write(connfd, msg, strlen(msg));
    }
}

void print_channel_users(int index, char *c) // std::vector<std::string> wanted_channels)
{
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];
    strcpy(nick_name, client_info[index].nick_name);

    char msg[MSG_SIZE];
    if (channels.find(c) != channels.end()) 
    {
        memset(msg, '\0', MSG_SIZE);
        sprintf(msg, ":%s 353 %s %s :", SERVER_NAME, nick_name, c);

        for (auto user : channels[c].connected)
        {            
            strcat(msg, client_info[user].nick_name);
            strcat(msg, " ");
        }
        strcat(msg, "\n");
        write(connfd, msg, strlen(msg));
    }

    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 366 %s %s :End of Names List\n", SERVER_NAME, nick_name, c);
    write(connfd, msg, strlen(msg));

} 

void print_channel_info(int index)
{
    char msg[MSG_SIZE];
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];
    strcpy(nick_name, client_info[index].nick_name);

    // list start
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 321 %s Channel :Users Name\n", SERVER_NAME, nick_name);
    write(connfd, msg, strlen(msg));

    // list body
    for (auto it : channels) 
    {
        memset(msg, '\0', MSG_SIZE);
        sprintf(msg, ":%s 322 %s %s %ld :%s\n", SERVER_NAME, nick_name, it.first.data(), it.second.connected.size(), it.second.topic.data());
        write(connfd, msg, strlen(msg));
    }

    //list end
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 323 %s :End of Liset\n", SERVER_NAME, nick_name);
    write(connfd, msg, strlen(msg));
}

void print_channel_info(int index, char *c)
{   
    char msg[MSG_SIZE];
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];    
    strcpy(nick_name, client_info[index].nick_name);

    // list start
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 321 %s Channel :Users Name\n", SERVER_NAME, nick_name);
    write(connfd, msg, strlen(msg));

    // list body
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 322 %s %s %ld :%s\n", SERVER_NAME, nick_name, c, channels[c].connected.size(), channels[c].topic.data());
    write(connfd, msg, strlen(msg));


    //list end
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s 323 %s :End of Liset\n", SERVER_NAME, nick_name);
    write(connfd, msg, strlen(msg));
}

void print_msg_channel(int index, char *text, std::string channel_name)
{
    int connfd = client[index].fd;
    char nick_name[NAME_MAX];
    strcpy(nick_name, client_info[index].nick_name);
    
    char msg[MSG_SIZE];

    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, ":%s PRIVMSG %s :%s\n", nick_name, channel_name.data(), text);
    
    for (auto user: channels[channel_name].connected)
    {
        if (user == index) continue;
        write(user, msg, strlen(msg));    
    }
}
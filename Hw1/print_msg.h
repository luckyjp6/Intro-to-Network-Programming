#ifndef PRINT_MSG_H
#define PRINT_MSG_H

// void print_nick(std::string old_nick, std::string new_nick);
// void set_user(std::string nick_name, char** names);
void welcome_new_client(int index);
void print_ping(int connfd, char *host);

void print_join(int index, char *channel_name); // std::vector<std::string> join_channel);
void print_part(int index, std::string channel_name);
void print_topic(int index, std::string topic, std::string channel_name);

void print_all_users(int index);
void print_channel_users(int index);
void print_channel_users(int index, char *channel_name); // std::vector<std::string> wanted_channels);

void print_channel_info(int index);
void print_channel_info(int index, char *channel_name); // std::vector<std::string> wanted_channels);

void print_msg_channel(int index, char *text, std::string channel_name);  
#endif
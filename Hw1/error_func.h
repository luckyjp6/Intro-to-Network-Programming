#ifndef ERROR_H
#define ERROR_H

void no_such_nick_channel(int index, std::string channel); // 401
void no_such_channel(int index, std::string channel); // 403

void no_origin(int index);

void no_recipient(int index, char *command); // 411
void no_text_send(int index); // 412

void error_cmd(int index, char *command); //421

bool check_conflict_nick(char *nickname);
bool check_nick_name(int index, char *nick_name, bool new_client); // 431, 432, 433, 436
// void nickname_in_use(int index, char *nick_name); // 433

void not_on_channel(int index, std::string channel); // 442
void not_registered(int index); // 451
void not_registered(int index, std::string nick_name); // 451
void not_enough_args(int index, char *command); // 461
void reregister_error(int index); // 462

bool check_user_in_channel(int index, std::string channel);
#endif
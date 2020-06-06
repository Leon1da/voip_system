//
// Created by leonardo on 05/06/20.
//

#ifndef CLIENT_SERVER_TCP_SERVER_METHODS_H
#define CLIENT_SERVER_TCP_SERVER_METHODS_H

#endif //CLIENT_SERVER_TCP_SERVER_METHODS_H

#include "utils.h"


user * authentication(int client_desc);

void help_message(user *pUser);

void list_users(user *pUser);

user * open_chat(user *newUser);

void start_video_chat(user *pUser);

bool quit_chat(user *pUser);

bool isAlreadyLogged(user usr);

void start_comunication(user* newUser, user* destUser);


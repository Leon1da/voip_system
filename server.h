//
// Created by leonardo on 21/09/20.
//

#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <list>
#include <csignal>
#include <thread>

#include "common.h"
#include "ConnectionManager.h"
#include "peer.h"
#include "user.h"

#ifndef CHAT_UDP_SERVER_H
#define CHAT_UDP_SERVER_H

#endif //CHAT_UDP_SERVER_H

void recv_client_request_audio(Message *msg);

void recv_client_refuse_audio(Message *msg);

void recv_client_accept_audio(Message *msg);

void recv_client_handshake_audio(Message *message);

void recv_client_ringoff_audio(Message *msg);

int init_server_udp_connection(sockaddr_in socket_address);

int close_udp_connection(int socket);

void init_server();


// Comunication

void receiver();

void recv_client_chat(Message *msg);

void recv_client_authentication(sockaddr_in *client_addr, Message *msg);

void recv_client_quit(Message *msg);

void recv_client_users(Message *msg);

void send_broadcast_message(Message *msg);


// Utility

void print_logged_users();

void print_registered_users();

void print_session_chats();

string get_logged_client_list();

void print_message(char *message);


bool isLogged(const string& username);

bool isRegistered(const string& username, const string& password);

list<User> read_users_from_file(const string filename);

User * get_logged_user(const string& username);


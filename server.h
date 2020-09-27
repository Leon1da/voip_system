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
#include "config.h"

#ifndef CHAT_UDP_SERVER_H
#define CHAT_UDP_SERVER_H

#endif //CHAT_UDP_SERVER_H


void udp_init();

void close_udp_connection(int socket);

void server_init();

void signal_handler_init();

void sigintHandler(int sig_num);


// Comunication

void receiver();

void recv_client_chat(sockaddr_in client_addr, char* message);

void recv_client_authentication(sockaddr_in client_addr, char *message);

void recv_client_quit(char *message);

void recv_client_users(sockaddr_in client_addr, char *message);

void send_broadcast_message(char* message);


// Utility

void print_logged_users();

void print_registered_users();

void print_session_chats();

string get_logged_client_list();

void print_message(char *message);


bool isLogged(string username);

bool isRegistered(string username, string password);

list<User> read_users_from_file(const string filename);

User * get_logged_user(string username);


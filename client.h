//
// Created by leonardo on 21/09/20.
//

#ifndef CHAT_UDP_CLIENT_H
#define CHAT_UDP_CLIENT_H

#endif //CHAT_UDP_CLIENT_H

#include <iostream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <list>
#include <csignal>
#include <regex>

#include <semaphore.h>



void udp_init();

void udp_close();

void signal_handler_init();

// Comunication

void sender();

void receiver();

bool client_authentication();

void client_chat();

void client_users();

void client_quit();

void client_video();

void client_audio_request();


// Utility

void print_info_message();

void print_message(char *msg);

bool input_available(int fd);
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

#include "config.h"
#include "MessageManager.h"


int init_server_udp_connection(sockaddr_in socket_address);

int init_client_udp_connection(sockaddr_in socket_address);

void udp_init();

void close_udp_connection(int socket);

void signal_handler_init();

// Comunication

void sender();

void receiver();

int client_authentication();

void client_chat();

void client_users();

void client_quit();

void client_video();

void client_audio_request();


// Utility

void print_info_message();

void print_message(char *msg);

bool input_available(int fd);

void unblocked_recvfrom(int socket, char *buf, int buf_size);

// audio

void called_routine();

void caller_routine();

void client_audio_accept();

void client_audio_refuse();

void client_audio_ringoff();

void recv_audio_accept(Message *msg);

void recv_audio_request(Message *msg);

void recv_audio_ringoff();

void recv_audio_refuse();

void safe_peer_delete();
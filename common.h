//
// Created by leonardo on 12/09/20.
//

#ifndef CHAT_UDP_CONFIG_H
#define CHAT_UDP_CONFIG_H

#endif //CHAT_UDP_CONFIG_H


#include <csignal>

#define XIAOMI_ADDRESS "192.168.1.69"
#define TOSHIBA_ADDRESS "192.168.1.25"
#define LOCALHOST_ADDRESS "127.0.0.1"

#define CS_PORT 30000
#define P2P_PORT 30020

#define MAX_CONN_QUEUE 5



// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)

void sigintHandler(int sig_num);

void signal_handler_init();


#define LOG 0




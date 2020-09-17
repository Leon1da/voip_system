//
// Created by leonardo on 13/05/20.
//

#include <iostream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <thread>
#include <list>
#include <csignal>
#include <regex>

#include "config.h"

using namespace std;

void udp_init();

void udp_close();

bool client_authentication();

void receiver();

void client_chat();

void client_users();

void client_quit();

void client_video();

void signal_handler_init();

void sender();


void print_info_message();

void client_audio();

void print_message(char *msg);

int  udp_socket;
struct sockaddr_in servaddr;

string username;
list<string> logged_users;

CODE client_status;
bool running;

int main(int argc, char *argv[])
{
    cout << "Signal handler setup." << endl;
    signal_handler_init();

    cout << "Client setup." << endl;
    udp_init();

    // authentication
    while (!client_authentication());
//    if(!client_authentication()){
//        udp_close();
//        return EXIT_SUCCESS;
//    }

    client_status = SUCCESS;

    cout << "Welcome " << username << endl;

    // Handles writing to the shell
    thread send_thread(sender);

    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(udp_socket, &set); // add server descriptor on set
    // set timeout
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    running = true;
    while(running){

        int ret = select(MAX_CONN_QUEUE + 1, &set, NULL, NULL, &timeout);
        if(ret < 0) {
            if(errno == EINTR) continue;
            perror("Error during select operation: ");
            exit(EXIT_FAILURE);
        }else if(ret == 0) {
            // timeout occurred
            timeout.tv_sec = 5;
            FD_ZERO(&set); // clear set
            FD_SET(udp_socket, &set); // add server descriptor on set
            // timeout occurred
            if(!running) cout << "[Info] Timeout occurred, closing client." << endl;
        }else {
            // available data
            receiver();
        }

    }

    cout << "Closing client." << endl;
    send_thread.join();

    udp_close();

    return EXIT_SUCCESS;
}

void print_info_message() {
    cout << "Type <users> to see users online. " << endl
            << "Type <chat> to send message. " << endl
            << "Type <quit> to disconnect." << endl;
}

void print_message(char *msg) {
    cout << "[ " << msg << " ][ " << msg + MSG_H_CODE_SIZE << " ][ " << msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE << " ][ "
         << msg + MSG_HEADER_SIZE << " ]" << endl;
}


// Functionality

void client_video() {
    client_status = CODE::VIDEO;
}

void client_quit() {
    client_status = CODE::QUIT;
    running = false;

    char msg[MSG_SIZE];

    memset(msg, 0, MSG_SIZE);
    snprintf(msg, MSG_H_CODE_SIZE, "%d", CODE::QUIT);
    memcpy(msg + MSG_H_CODE_SIZE, username.c_str(), MSG_H_SRC_SIZE);

    cout << "send: ";
    print_message(msg);

    int ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }
}

void client_users() {
    client_status = CODE::USERS;

    char msg[MSG_SIZE];
    // clear buffer
    memset(msg, 0, MSG_SIZE);
    // msg code
    sprintf(msg, "%d", CODE::USERS);
    // msg src
    memcpy(msg + MSG_H_CODE_SIZE, username.c_str(), MSG_H_SRC_SIZE);

    cout << "send: ";
    print_message(msg);

    int ret;
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation.");
        exit(EXIT_FAILURE);
    }




}

void client_chat() {

    client_status = CODE::CHAT;

    char msg[MSG_SIZE];
    // clear buffer
    memset(msg, 0, MSG_SIZE);
    // msg code
    sprintf(msg, "%d", CODE::CHAT);
    // msg src
    memcpy(msg + MSG_H_CODE_SIZE, username.c_str(), MSG_H_SRC_SIZE);

    cout << "Type recipient: " << endl;
    string dst;
    cin >> dst;
    memcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, dst.c_str(), MSG_H_DST_SIZE );

    cout << "Type message: " << endl;
    read(0,msg + MSG_HEADER_SIZE, MSG_CONTENT_SIZE);

    cout << "send: ";
    print_message(msg);

    int ret;
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation.");
        exit(EXIT_FAILURE);
    }

}

bool client_authentication() {
    client_status = CODE::AUTHENTICATION;

    regex r("[a-zA-Z0-9]+ [a-zA-Z0-9]+");

    cout << "Digit <username> <password> for access to service." << endl;
    char credential[MSG_CONTENT_SIZE];
    cin.getline(credential, MSG_CONTENT_SIZE);

    if(!regex_match(credential, r)){
        cout << "No pattern mathing (<username> <password>)." << endl;
        return false;
    }

    char msg[MSG_SIZE];
    sprintf(msg, "%d", CODE::AUTHENTICATION);
    sprintf(msg + MSG_HEADER_SIZE, "%s", credential);

    print_message(msg);

    int ret;
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(ret < 0) {
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }

    // clear buffer
    memset(msg, 0, MSG_SIZE);
    // waiting for response

    ret = recvfrom(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr *) NULL, 0);
    if(ret < 0){
        perror("Error during recv operation");
        exit(EXIT_FAILURE);
    }


    print_message(msg);

    cout << "[Server] " << msg + MSG_HEADER_SIZE << endl;


    char code[MSG_H_CODE_SIZE];
    memcpy(code, msg, MSG_H_CODE_SIZE);

    CODE code_ = (CODE) atoi(code);
    if(code_ == CODE::SUCCESS){
        char user[MSG_H_DST_SIZE];
        memcpy(user, msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, MSG_H_DST_SIZE);
        username = user;
        return true;
    }else return false;

}

void receiver() {

    int ret;

    char msg[MSG_SIZE];
    ret = recvfrom(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr *) NULL, 0);
    if(ret < 0){
        perror("Error during recv operation");
        exit(EXIT_FAILURE);
    }

    cout << "recv: ";
    print_message(msg);

    char code[MSG_H_CODE_SIZE];
    memcpy(code, msg, MSG_H_CODE_SIZE);


    CODE code_ = (CODE) atoi(code);
    switch (code_) {
        case BROADCAST:
            cout << "[Server] " << msg + MSG_HEADER_SIZE << endl;
            break;
        case CHAT:
            cout << "[" << msg + MSG_H_CODE_SIZE << "] " << msg + MSG_HEADER_SIZE << endl;
            break;
        case QUIT:
            running = false;
            break;
        case USERS:
            // [TODO] update logged_users when arrive message (USERS)
            cout << "Logged users:" << endl << msg + MSG_HEADER_SIZE << endl;
            break;

        case ERROR:
            cout << "[Server] " << msg + MSG_HEADER_SIZE << endl;
        default:
            break;
    }



}

void sender(){
    while (running){
        print_info_message();

        string line;
        getline(cin,line);

        if(line.compare("chat") == 0 ) client_chat();
        else if(line.compare("users") == 0 ) client_users();
        else if(line.compare("quit") == 0 ) client_quit();
        else if(line.compare("video") == 0 ) client_video();
        else if(line.compare("audio") == 0 ) client_audio();


    }
}

void client_audio() {

}


/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    // signal(SIGINT, sigintHandler);
    cout << endl <<"Catch Ctrl+C - System status: " << client_status << endl;
    switch (client_status) {
        case CODE::AUTHENTICATION:
            cout << "Authentication stopped." << endl;
            exit(EXIT_SUCCESS);
        default:
            cout << "Service Stopped." << endl;
            client_quit();
            break;
    }

}


// Initialization

void udp_close() {

    // close the descriptor
    int ret = close(udp_socket);
    if(ret < 0){
        perror("Error during close operation");
        exit(EXIT_FAILURE);
    }

}

void udp_init() {

    // clear servaddr
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    servaddr.sin_port = htons(SERVER_CHAT_PORT);
    servaddr.sin_family = AF_INET;

    // create datagram socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // connect to server
    if(connect(udp_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        exit(0);
    }

}

void signal_handler_init() {
    /* signal handler */
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigintHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0; //SA_RESTART
    sigaction(SIGINT, &sigIntHandler, NULL);
    /* end signal handler */

}
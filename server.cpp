//
// Created by leonardo on 13/05/20.
//
#include <algorithm>
#include <fstream>

#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <list>

#include <csignal>

#include "config.h"

using namespace std;

int udp_socket;

list<Chat*> chats;

list<User> users;

list<User*> logged_users;

bool running;


void sigintHandler(int sig_num);

void udp_init();

void udp_close();

void server_init();

void signal_handler_init();


void receiver();

void recv_client_chat(sockaddr_in client_addr, char* message);

User * get_logged_user(string username);

void recv_client_authentication(sockaddr_in client_addr, char *message);

void recv_client_quit(char *msg);

void print_logged_users();

void print_registered_users();

void recv_client_users(sockaddr_in client_addr, char *message);

string get_logged_client_list();


void print_message(char *message);

void send_broadcast_message(char* message);

bool isLogged(string username);

bool isRegistered(string username, string password);

list<User> read_users_from_file(const string filename);

void print_session_chats();

int main(int argc, char *argv[])
{
    cout << "Server setup." << endl;
    server_init();
    cout << "Protocol setup." << endl;
    udp_init();
    cout << "Server ready." << endl;
    print_registered_users();

    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(udp_socket, &set); // add server descriptor on set
    // set timeout
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ret;

    running = true;
    while(running){

        ret = select(MAX_CONN_QUEUE + 1, &set, NULL, NULL, &timeout);
        if(ret < 0){
            if(errno == EINTR) continue;
            perror("Error during server select operation: ");
            exit(EXIT_FAILURE);
        }
        else if(ret == 0) {
             // timeout occurred
            timeout.tv_sec = 5;
            FD_ZERO(&set); // clear set
            FD_SET(udp_socket, &set); // add server descriptor on set
            if(!running) cout << "[Info] Timeout occurred, closing server." << endl;
        }else {
            // Available data
            receiver();
        }

    }

    cout << "Print report." << endl;
    print_session_chats();

    udp_close();
    cout << "Server shutdown." << endl;

    return EXIT_SUCCESS;
}

void print_session_chats() {
    for(Chat* c: chats){
        cout << "Users: ";
        for(User u: c->users) cout << u.username << endl;
        cout << "Messages: ";
        for(Message m: c->messages) cout << m.text << endl;
    }

}

// Functionality

void recv_client_users(sockaddr_in client_addr, char *message) {

    // [TODO] andrebbe splittata la stringa se la sua lunghezza e` maggiore di MSG_CONTENT_SIZE
    string logged_client_list = get_logged_client_list();
    strcpy(message + MSG_HEADER_SIZE, logged_client_list.c_str());

    if(LOG){
        cout << "send: ";
        print_message(message);
    }

    int len = sizeof(client_addr);
    int ret = sendto(udp_socket, message, MSG_SIZE, 0, (struct sockaddr*) &client_addr, len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }

}

void recv_client_quit(char *message) {

    char src[MSG_H_SRC_SIZE];
    memcpy(src, message + MSG_H_CODE_SIZE, MSG_H_SRC_SIZE);

    User* dest = get_logged_user(src);
    if(dest == nullptr)

    if(LOG) print_logged_users();

    logged_users.remove(dest);

    if(LOG) print_logged_users();


    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::INFO).c_str());
    string text = string("User ") + src + string(" left the chat.");
    strcpy(msg + MSG_HEADER_SIZE, text.c_str());

    if(LOG){
        cout << "send: ";
        print_message(msg);
    }

    send_broadcast_message(msg);

}

void recv_client_authentication(sockaddr_in client_addr, char *message) {

    int ret;

    char content[MSG_CONTENT_SIZE];
    memcpy(content, message + MSG_HEADER_SIZE, MSG_CONTENT_SIZE);

    string content_ = content;
    string username = content_.substr(0,content_.find(" "));
    string password = content_.substr(content_.find(" ") + 1, content_.size());

    if(LOG) cout << username << " - " << password << " request login." << endl;

    socklen_t len = sizeof(client_addr);

    char msg[MSG_SIZE];

    if(isRegistered(username, password)){
        if(!isLogged(username)){

            // Alert all client join chat
            strcpy(msg, to_string(CODE::INFO).c_str());
            string join = "User " + username + " join chat.";
            strcpy(msg + MSG_HEADER_SIZE, join.c_str());
            send_broadcast_message(msg);

            // login success for client
            memset(msg, 0, MSG_SIZE);
            sprintf(msg, "%d", CODE::SUCCESS);
            sprintf(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, "%s", username.c_str());
            sprintf(msg + MSG_HEADER_SIZE, "%s", "Login successful.");

            User* newUser = new User(0, username, password, client_addr);
            logged_users.push_back(newUser);


        } else{
            // logged
            sprintf(msg, "%d", CODE::ERROR);
            sprintf(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, "%s", username.c_str());
            sprintf(msg + MSG_HEADER_SIZE, "%s", "Login failed. You are already logged in.");

        }
    } else{
        // not registered
        sprintf(msg, "%d", CODE::ERROR);
        sprintf(msg + MSG_HEADER_SIZE, "%s", "Login failed.");
    }

    ret = sendto(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr*) &client_addr, len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }

}

void recv_client_chat(sockaddr_in client_addr, char* message){

    int ret;

    char src_buf[MSG_H_DST_SIZE];
    memcpy(src_buf, message + MSG_H_CODE_SIZE, MSG_H_SRC_SIZE);

    User* src = get_logged_user(src_buf);

    char dst_buf[MSG_H_DST_SIZE];
    memcpy(dst_buf, message + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, MSG_H_DST_SIZE);

    User* dst = get_logged_user(dst_buf);

    socklen_t addr_len;
    struct sockaddr_in addr;

    if(dst == nullptr){

        if(LOG) cout << "dst: " << dst_buf << " not found." << endl;

        sprintf(message,"%d", CODE::ERROR); // set CODE
        memcpy(message + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, message + MSG_H_CODE_SIZE, MSG_H_DST_SIZE); // set DST
        memcpy(message + MSG_HEADER_SIZE, "User not found.", MSG_CONTENT_SIZE);

        addr = client_addr;
        addr_len = sizeof(addr);

    }else{
        cout << src->username << " send message to " << dst->username << endl;
        if(LOG) cout << "dst information: " << dst->username << " - " << dst->password << " - " << dst->address.sin_addr.s_addr << " - " << dst->address.sin_port << endl;

        addr = dst ->address;
        addr_len = sizeof(dst->address);
    }

    ret = sendto(udp_socket, message, MSG_SIZE, 0, (struct sockaddr*) &addr, addr_len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }


}

bool isRegistered(string username, string password) {
    for (User &u : users ) if(username.compare(u.username) == 0 && password.compare(u.password) == 0) return true;
    return false;
}

bool isLogged(string username) {
    for(User* u : logged_users) if(username.compare(u->username) == 0) return true;
    return false;
}

void print_message(char *msg) {
    cout << "[ " << msg << " ][ " << msg + MSG_H_CODE_SIZE << " ][ " << msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE << " ][ "
         << msg + MSG_HEADER_SIZE << " ]" << endl;
}

void receiver() {

    int ret;
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);

    char msg[MSG_SIZE];
    ret = recvfrom(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr*)&client_address, &len);
    if(ret < 0) {
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }

    char code[MSG_H_CODE_SIZE];
    memcpy(code, msg, MSG_H_CODE_SIZE);


    CODE code_ = (CODE) atoi(code);
    switch (code_) {
        case AUTHENTICATION:
            if(LOG) cout << "Authentication message arrived." << endl;
            recv_client_authentication(client_address, msg);
            break;
        case CHAT:
            if(LOG) cout << "Chat message arrived." << endl;
            recv_client_chat(client_address, msg);
            break;
        case USERS:
            if(LOG) cout << "Users message arrived." << endl;
            recv_client_users(client_address, msg);
            break;
        case QUIT:
            if(LOG) cout << "Quit message arrived." << endl;
            recv_client_quit(msg);
            break;
        default:
            if(LOG) cout << "Default message arrived." << endl;
            break;
    }

}

// Initialization

void server_init() {

    if(LOG) cout << "Read Users from database." << endl;
    users = read_users_from_file("users.txt");

    if(LOG) cout << "Signal handler init." << endl;
    signal_handler_init();
}

list<User> read_users_from_file(const string filename) {

    list<User> users_read;

    std::ifstream in(filename);
    if(!in.good()){
        cout << "Error during read file: " << filename << endl;
        return users_read;
    }

    while(!in.eof()){
        User u;
        in >> u;
        users_read.push_back(u);
    }
    in.close();

    return users_read;

}


void udp_init() {

    cout << "Udp protocol setupping..." << endl;

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_CHAT_PORT);
    server_address.sin_family = AF_INET;

    // Create a UDP Socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket < 0){
        perror("Error during socket operation.");
        exit(EXIT_FAILURE);
    }

    // bind server address to socket descriptor
    int ret = bind(udp_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    if(ret < 0){
        perror("Error during bind operation.");
        exit(EXIT_FAILURE);
    }

    cout << "Udp protocol configured." << endl;

}

void udp_close() {
    cout << "Udp socket closing..." << endl;
    if(close(udp_socket) < 0){ // this cause an error on accept
        perror("Error during close server socket");
        exit(EXIT_FAILURE);
    }
    cout << "Udp socket closed." << endl;
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

void send_broadcast_message(char* message){
    int ret;
    for(User* u: logged_users){
        int len = sizeof(u->address);
        ret = sendto(udp_socket, message, MSG_SIZE, 0, (sockaddr*) &u->address, len);
        if(ret < 0){
            perror("Error during send operation");
            exit(EXIT_FAILURE);
        }
    }
}

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::QUIT).c_str());
    send_broadcast_message(msg);

    running = false;

//    if(!logged_users.empty()){
//        print_logged_users();
//        cout << "Unable to shut down server. there are connected clients." << endl;
//    }
//    else{
//        running = false;
//    }

}


// Utility

void print_registered_users() {

    cout << "Registered users: " << endl;
    for(User &u : users){
        cout << " - " << u.username << " " << u.password << endl;
    }

}

string get_logged_client_list() {
    string logged_client_list = "";
    for(User* &u: logged_users) logged_client_list.append(" - " + u->username);
    return logged_client_list;
}

void print_logged_users() {
    cout << "Logged users: " << endl;
    for(User* &u : logged_users){
        cout << " - " << u->username << " " << u->password << " "
             << u->address.sin_addr.s_addr << " " << u->address.sin_port
             << u->address.sin_family << " " << u->address.sin_zero << endl;

    }
}

User* get_logged_user(string username) {
    for(User* &u : logged_users)
        if(username.compare(u->username) == 0) return u;
    return nullptr;
}

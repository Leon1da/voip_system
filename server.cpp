//
// Created by leonardo on 13/05/20.
//

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
#include <thread>
#include <map>
#include <list>

#include "config.h"

using namespace std;

int udp_socket;

list<user> users;

list<user*> logged_users;

struct sockaddr_in servaddr;

void udp_init();

void udp_close();

void server_init();


void recv_msg();

void client_chat(sockaddr_in client_addr, char* msg);

user * getUser(string username);

void client_authentication(sockaddr_in client_addr, char *message);

void client_quit(char *msg);

void printLoggedUsers();

void printUsers();

void client_users(sockaddr_in client_addr, char *message);

string get_logged_client_list();


void print_message(char *message);

int main(int argc, char *argv[])
{
    cout << "Server setup." << endl;
    server_init();
    cout << "Protocol setup." << endl;
    udp_init();
    cout << "Server ready." << endl;
    printUsers();



    int ret;

    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(udp_socket, &set); // add server descriptor on set
    // set timeout
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    bool running = true;
    while(running){

        ret = select(MAX_CONN_QUEUE + 1, &set, NULL, NULL, &timeout);
        if(ret == -1) {
            perror("Error during server select operation: ");
            exit(EXIT_FAILURE);
        }
        else if(ret == 0) {
             cout << "Timeout occurred." << endl;
            // select timeout occurred
            timeout.tv_sec = 5;
            FD_ZERO(&set); // clear set
            FD_SET(udp_socket, &set); // add server descriptor on set
            // timeout occurred
            if(!running) cout << "[Info] Timeout occurred, closing server fd" << endl;
        }else {
            // connection incoming
            cout << "Data is available now." << endl;
            //receive message from clients
            recv_msg();
        }

    }


    udp_close();
    cout << "Server shutdown." << endl;

    return EXIT_SUCCESS;
}

// Functionality

void client_users(sockaddr_in client_addr, char *message) {

    int len = sizeof(client_addr);

    // [TODO] andrebbe splittata la stringa se la sua lunghezza e` maggiore di MSG_CONTENT_SIZE
    string logged_client_list = get_logged_client_list();
    cout << "logged_user_list: " << logged_client_list << " - size: " << logged_client_list.size() << endl;
    memcpy(message + MSG_HEADER_SIZE, logged_client_list.c_str(), MSG_CONTENT_SIZE);

    int ret = sendto(udp_socket, message, MSG_SIZE, 0, (struct sockaddr*) &client_addr, len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }



}

void client_quit(char *message) {

    char src[MSG_H_SRC_SIZE];
    memcpy(src, message + MSG_H_CODE_SIZE, MSG_H_SRC_SIZE);

    cout << "Message: " << message << " - " << message + MSG_H_CODE_SIZE <<  endl;

    user* dest = getUser(src);
    if(dest == nullptr)


    printLoggedUsers();

    list<user*> new_logged_users;
    for(user* &u: logged_users) if(dest->username.compare(u->username) != 0) new_logged_users.push_back(u);
    logged_users = new_logged_users;

    printLoggedUsers();

    cout << "User " << src << " left the chat." << endl;

}

void client_authentication(sockaddr_in client_addr, char *message) {

    int ret;

    char content[MSG_CONTENT_SIZE];
    memcpy(content, message + MSG_HEADER_SIZE, MSG_CONTENT_SIZE);

    // todo not good working
    string content_ = content;
    string username = content_.substr(0,content_.find(" "));
    string password = content_.substr(content_.find(" ") + 1, content_.size());

    cout << username << " - " << password << endl;

    user* newUser = new user(0,username, password, client_addr);


    bool registered = false;
    for (user &u : users ) if(username.compare(u.username) == 0 && password.compare(u.password) == 0) registered = true;

    char msg[MSG_SIZE];

    if(registered){
        sprintf(msg, "%d", CODE::SUCCESS);
        sprintf(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, "%s", username.c_str());
        sprintf(msg + MSG_HEADER_SIZE, "%s", "Login successful.");

        logged_users.push_back(newUser);

    }else {
        sprintf(msg, "%d", CODE::ERROR);
        sprintf(msg + MSG_HEADER_SIZE, "%s", "Login failed.");
    }

    socklen_t len = sizeof(newUser->address);
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr*) &newUser->address, len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }


}

void client_chat(sockaddr_in client_addr, char* message){

    int ret;

//    print_message(message);

//     cout << "Message: " << message << " - " << message + MSG_H_CODE_SIZE << " - " << message + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE << " - " << message + MSG_HEADER_SIZE << endl;
    char buf[MSG_H_DST_SIZE];
    memcpy(buf, message + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, MSG_H_DST_SIZE);

    user* dst = getUser(buf);

    socklen_t addr_len;
    struct sockaddr_in addr;

    if(dst == nullptr){
//        cout << "User " << message + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE << " not found." << endl;
        sprintf(message,"%d", CODE::ERROR); // set CODE
        memcpy(message + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, message + MSG_H_CODE_SIZE, MSG_H_DST_SIZE); // set DST
        memcpy(message + MSG_HEADER_SIZE, "User not found.", MSG_CONTENT_SIZE);

//        print_message(message);
        // cout << "Message: " << dst->username << " - " << dst->password << " - " << dst->address.sin_addr.s_addr << " - " << dst->address.sin_port << endl;

        addr = client_addr;
        addr_len = sizeof(addr);

    }else{
        cout << "DST: " << dst->username << " - " << dst->password << " - " << dst->address.sin_addr.s_addr << " - " << dst->address.sin_port << endl;
        addr = dst ->address;
        addr_len = sizeof(dst->address);
    }

    ret = sendto(udp_socket, message, MSG_SIZE, 0, (struct sockaddr*) &addr, addr_len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }



}

void print_message(char *message) {

    char code[MSG_H_CODE_SIZE];
    memcpy(code, message, MSG_H_CODE_SIZE);
    char src[MSG_H_SRC_SIZE];
    memcpy(src, message + MSG_H_CODE_SIZE, MSG_H_SRC_SIZE);
    char dst[MSG_H_DST_SIZE];
    memcpy(dst, message + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, MSG_H_DST_SIZE);
    char content[MSG_H_SRC_SIZE];
    memcpy(content, message + MSG_HEADER_SIZE, MSG_CONTENT_SIZE);

    cout << "[ " << code << " ][ " << src << " ][ " << dst << " ]" << endl << "[ " << content << " ]" << endl;

}

void recv_msg() {

    int ret;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    char msg[MSG_SIZE];
    ret = recvfrom(udp_socket, msg, MSG_SIZE,0, (struct sockaddr*)&cliaddr, &len);
    if(ret < 0) {
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }


    printLoggedUsers();


    char code[MSG_H_CODE_SIZE];
    memcpy(code, msg, MSG_H_CODE_SIZE);
    cout << "Message Code: " << code << endl;

    CODE code_ = (CODE) atoi(code);
    switch (code_) {
        case AUTHENTICATION:
            client_authentication(cliaddr, msg);
            break;
        case CHAT:
            client_chat(cliaddr, msg);
            break;
        case USERS:
            client_users(cliaddr, msg);
            break;
        case QUIT:
            client_quit(msg);
            break;
        default:
            break;
    }

}

// Initialization

void server_init() {

    std::ifstream in("users.txt");
    if(!in.good()) return;

    while(!in.eof()){
        user u;
        in >> u;
        users.push_back(u);
    }
    in.close();
}

void udp_init() {

    bzero(&servaddr, sizeof(servaddr));

    // Create a UDP Socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_CHAT_PORT);
    servaddr.sin_family = AF_INET;

    // bind server address to socket descriptor
    bind(udp_socket, (struct sockaddr*)&servaddr, sizeof(servaddr));

}

void udp_close() {
    cout << "Connection closed..." << endl;
    if(close(udp_socket) < 0){ // this cause an error on accept
        perror("Error during close server socket");
        exit(EXIT_FAILURE);
    }
}

// Utility

void printUsers() {

    cout << "Registered users: " << endl;
    for(user &u : users){
        cout << " - " << u.username << " " << u.password << " "
             << u.address.sin_addr.s_addr << " " << u.address.sin_port
             << u.address.sin_family << " " << u.address.sin_zero << endl;

    }

}

string get_logged_client_list() {
    string logged_client_list = "";
    for(user* &u: logged_users) logged_client_list.append(u->username + "\n");
    return logged_client_list;
}

void printLoggedUsers() {
    cout << "Logged users: " << endl;
    for(user* &u : logged_users){
        cout << " - " << u->username << " " << u->password << " "
             << u->address.sin_addr.s_addr << " " << u->address.sin_port
             << u->address.sin_family << " " << u->address.sin_zero << endl;

    }
}

user* getUser(string username) {
    for(user* &u : logged_users)
        if(username.compare(u->username) == 0) return u;
    return nullptr;
}

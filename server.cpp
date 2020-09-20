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


struct sockaddr_in servaddr;

bool running;


void sigintHandler(int sig_num);

void udp_init();

void udp_close();

void server_init();

void signal_handler_init();


void receiver();

void client_chat(sockaddr_in client_addr, char* msg);

User * get_logged_user(string username);

void client_authentication(sockaddr_in client_addr, char *message);

void client_quit(char *msg);

void print_logged_users();

void print_registered_users();

void client_users(sockaddr_in client_addr, char *message);

string get_logged_client_list();


void print_message(char *message);

void broadcast_send(string message);

bool isLogged(string username);

bool isRegistered(string username, string password);

list<User> read_users_from_file(const string filename);

list<Chat> read_chats_from_file(const string string);

Chat *find_chat(list<User> users);

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

void client_users(sockaddr_in client_addr, char *message) {

    int len = sizeof(client_addr);

    // [TODO] andrebbe splittata la stringa se la sua lunghezza e` maggiore di MSG_CONTENT_SIZE
    string logged_client_list = get_logged_client_list();
    memcpy(message + MSG_HEADER_SIZE, logged_client_list.c_str(), MSG_CONTENT_SIZE);

    if(LOG) print_message(message);

    int ret = sendto(udp_socket, message, MSG_SIZE, 0, (struct sockaddr*) &client_addr, len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }

}

void client_quit(char *message) {

    char src[MSG_H_SRC_SIZE];
    memcpy(src, message + MSG_H_CODE_SIZE, MSG_H_SRC_SIZE);

    User* dest = get_logged_user(src);
    if(dest == nullptr)

    if(LOG) print_logged_users();

    logged_users.remove(dest);

    if(LOG) print_logged_users();


    string msg = string("User ") + src + string(" left the chat.");
    cout << msg << endl;

    broadcast_send(msg);

}

void broadcast_send(string message) {
    int ret;

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    sprintf(msg, "%d", CODE::BROADCAST);
    memcpy(msg + MSG_HEADER_SIZE, message.c_str(), MSG_CONTENT_SIZE);

    for(User* u : logged_users){
        socklen_t len = sizeof(u->address);
        ret = sendto(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr*) &u->address, len);
        if(ret < 0){
            perror("Error during send operation: ");
            exit(EXIT_FAILURE);
        }

    }

}

void client_authentication(sockaddr_in client_addr, char *message) {

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

void client_chat(sockaddr_in client_addr, char* message){

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
        if(LOG) cout << src << " send message to " << dst << endl;
        if(LOG) cout << "dst information: " << dst->username << " - " << dst->password << " - " << dst->address.sin_addr.s_addr << " - " << dst->address.sin_port << endl;

        addr = dst ->address;
        addr_len = sizeof(dst->address);
    }

    ret = sendto(udp_socket, message, MSG_SIZE, 0, (struct sockaddr*) &addr, addr_len);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }



//    Message m(*src, *dst, message + MSG_HEADER_SIZE);
//    cout << "Find chat." << endl;
//    Chat* chat = find_chat(list<User>(src, dst));
//
//    if(chat == nullptr){
//        cout << "chat not found." << endl;
//        list<User> l1;
//        l1.push_back(*src);
//        l1.push_back(*dst);
//
//        list<Message> l2;
//        l2.push_back(m);
//        Chat* chat = new Chat(l1, l2);
//        chats.push_back(chat);
//
//    } else{
//
//        cout << "chat found." << endl;
//        chat->messages.push_back(m);
//    }



}

bool isRegistered(string username, string password) {
    for (User &u : users ) if(username.compare(u.username) == 0 && password.compare(u.password) == 0) return true;
    return false;
}

bool isLogged(string username) {
    for(User* u : logged_users) if(username.compare(u->username) == 0) return true;
    return false;
}

bool contains(list<User> l, User e){
    bool found = false;
    while (!l.empty() && !found)
    {
        found = (l.front() == e);
        l.pop_front();
    }
    return found;
}

list<User> intersect(list<User> l1 ,list<User> l2)
{

    User cur;
    list<User> intersection;
    while(!l1.empty())
    {
        cur = l1.front();

        if(contains(l2, cur))
            intersection.push_back(cur);
        l1.pop_front();
    }
    return intersection;
}

Chat *find_chat(list<User> users) {
    for(Chat* c : chats)
        if(intersect(c->users, users).size() == c->users.size() == users.size()) return c;
    return nullptr;
}


void print_message(char *msg) {
    cout << "[ " << msg << " ][ " << msg + MSG_H_CODE_SIZE << " ][ " << msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE << " ][ "
         << msg + MSG_HEADER_SIZE << " ]" << endl;
}

void receiver() {

    int ret;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    char msg[MSG_SIZE];
    ret = recvfrom(udp_socket, msg, MSG_SIZE,0, (struct sockaddr*)&cliaddr, &len);
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
            client_authentication(cliaddr, msg);
            break;
        case CHAT:
            if(LOG) cout << "Chat message arrived." << endl;
            client_chat(cliaddr, msg);
            break;
        case USERS:
            if(LOG) cout << "Users message arrived." << endl;
            client_users(cliaddr, msg);
            break;
        case QUIT:
            if(LOG) cout << "Quit message arrived." << endl;
            client_quit(msg);
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


    if(LOG) cout << "Read Chats from database." << endl;
    // chats = read_chats_from_file("chats.txt");

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

list<Chat> read_chats_from_file(const string filename) {

    list<Chat> chats_read;

    std::ifstream in(filename);
    if(!in.good()){
        cout << "Error during read file: " << filename << endl;
        return chats_read;
    }

    string line;
    getline(in, line, ' ');
    int num_users = atoi(line.c_str());

    cout << "num_users: " << num_users << endl;

    getline(in, line, ' ');
    int num_messages = atoi(line.c_str());

    cout << "num_messages: " << num_messages << endl;

    getline(in, line, '\n');
    cout << "line: " << line << endl;

    list<User> users;
    for (int i = 0; i < num_users; ++i) {
        User u;
        in >> u;
        users.push_back(u);
        cout << "User: " << u.id << " " << u.username << " " << u.password << endl;
    }

    getline(in, line, '\n');
    cout << "line: " << line << endl;


    list<Message> messages;
    for (int i = 0; i < num_messages; ++i) {
        User src, dst;
        in >> src;
        cout << "src: " << src.id << " " << src.username << " " << src.password << endl;
        in >> dst;
        cout << "dst: " << dst.id << " " << dst.username << " " << dst.password << endl;



        string msg = "";
        string s;
        string num_strings;
        getline(in, num_strings, ' ');
        cout << "num_strings: " <<num_strings<< endl;
        for (int j = 0; j < atoi(num_strings.c_str()); ++j) {
            in >> s;
            msg.append(s + " ");

        }

        Message m(src,dst,msg);

        messages.push_back(m);

        cout << "Message: " << m.text << endl;
    }

    Chat c(users, messages);
//    chats.push_back(c);

    in.close();

//    int user_num, messages_num;
//    user_num =  atoi(tokens.front().c_str());
//    tokens.pop_front();
//    messages_num = atoi(tokens.front().c_str());
//    tokens.pop_front();
//
//



//    FILE * fp;
//    char * line = NULL;
//    size_t len = 0;
//    ssize_t read;
//
//    fp = fopen(filename.c_str(), "r");
//    if (fp == NULL)
//        exit(EXIT_FAILURE);
//
//    int i = 0;
//    int users_num = 0;
//    int messages_num = 0;
//
//    Chat chat;
//    while ((read = getline(&line, &len, fp)) != -1) {
//        line[strlen(line) - 1] = '\0';
//        printf("Retrieved line [%s] of length %zu:\n", line, read);
//
//        // check if num_user num_messages
//
//        char** tokens = (char**) malloc(sizeof(char**));
//
//        int j = 0;
//        /* get the first token */
//        char* token = strtok(line, " ");
//        /* walk through other tokens */
//        while( token != NULL ) {
//            tokens[j] = token;
//            printf( "token[%d] = %s\n", j, tokens[j] );
//            token = strtok(NULL, " ");
//            j++;
//        }
//
//
//        if(j == 2){
//            users_num = atoi(tokens[0]);
//            messages_num = atoi(tokens[1]);
//            cout << "Users num: " << users_num << " Messages num: "<< messages_num << endl;
//        } else{
//            if(i < users_num) {
//                User u(atoi(tokens[0]), tokens[1], tokens[2]);
//                cout << " User:" << u.id << " " << u.username << " " << u.password << endl;
//                chat.users.push_back(u);
//            }
//            else {
//
//                User src(atoi(tokens[0]), tokens[1], tokens[2]);
//                User dst(atoi(tokens[3]), tokens[4], tokens[5]);
//                cout << "src: " << src.id << " " << src.username << " " << src.password << " " << endl;
//                cout << "dst: " << dst.id << " " << dst.username << " " << dst.password << " " << endl;
//                string msg = "";
//                for (int k = 6; k < j; ++k){
//                    msg.append(tokens[k]);
//                    msg.append(" ");
//                }
//                Message m(src, dst, msg);
//                chat.messages.push_back(m);
//                cout << "Text: " << m.text << endl;
//
//                chat.messages.push_back(m);
//            }
//            i++;
//        }
//
//        free(tokens);
//
//    }
//
//    fclose(fp);
//    if (line)
//        free(line);

    return chats_read;
}


void udp_init() {

    cout << "Udp protocol setupping..." << endl;

    bzero(&servaddr, sizeof(servaddr));

    // Create a UDP Socket
    
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket < 0){
        perror("Error during socket operation.");
        exit(EXIT_FAILURE);
    }
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_CHAT_PORT);
    servaddr.sin_family = AF_INET;

    // bind server address to socket descriptor
    int ret = bind(udp_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
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

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    if(!logged_users.empty()){
        print_logged_users();
        cout << "Unable to shut down server. there are connected clients." << endl;
    }
    else{
        running = false;
    }

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
    for(User* &u: logged_users) logged_client_list.append(" - " + u->username + "\n");
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

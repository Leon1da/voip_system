//
// Created by leonardo on 13/05/20.
//


#include "server.h"

using namespace std;


list<User> registered_users;
list<User*> logged_users;

MessageManager* manager;

bool running;


int main(int argc, char *argv[])
{
    cout << "Server setup." << endl;
    init_server();

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(CS_PORT);
    server_address.sin_family = AF_INET;

    int server_socket = init_server_udp_connection(server_address);
    if(server_socket < 0){
        perror("Error during init_server_udp_connection");
        exit(EXIT_FAILURE);
    }

    manager = new MessageManager(server_socket);

    cout << "Server ready." << endl;
    print_registered_users();


    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(server_socket, &set); // add server descriptor on set
    // set timeout
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ret;

    running = true;
    while(running){

        ret = select(MAX_CONN_QUEUE + 1, &set, nullptr, nullptr, &timeout);
        if(ret < 0){
            if(errno == EINTR) continue;
            perror("Error during server select operation: ");
            exit(EXIT_FAILURE);
        }
        else if(ret == 0) {
             // timeout occurred
            timeout.tv_sec = 5;
            FD_ZERO(&set); // clear set
            FD_SET(server_socket, &set); // add server descriptor on set
            if(!running) cout << "[Info] Timeout occurred, closing server." << endl;
        }else {
            // Available data
            receiver();
        }

    }

    close_udp_connection(0);
    cout << "Server shutdown." << endl;

    for(User* u: logged_users) delete u;

    return EXIT_SUCCESS;
}

// Functionality

void recv_client_users(Message *msg) {

    User* src = get_logged_user(msg->getSrc());
    if(src == nullptr){
        cout << "Fatal error: src not found." << endl;
        exit(EXIT_FAILURE);
    }

    string logged_client_list = get_logged_client_list();

    msg->setCode(USERS);
    msg->setSrc("Server");
    msg->setDst(src->username);
    msg->setContent(logged_client_list);

    if(LOG) cout << "send: " << *msg;

    int ret = manager->sendMessage(msg, &src->address);
    if(ret < 0){
        perror("Error during send users list operation");
        exit(EXIT_FAILURE);
    }

}

void recv_client_quit(Message *msg) {

    User* src = get_logged_user(msg->getSrc());
    if(src == nullptr){
        cout << "Fatal error: src not found." << endl;
        exit(EXIT_FAILURE);
    }

    Message out(INFO, "Server", "", "User " + src->username + " left the chat.");

    logged_users.remove(src);
    delete src;

    if(LOG) print_logged_users();

    send_broadcast_message(&out);


}

void recv_client_authentication(sockaddr_in *client_addr, Message *msg) {

    Message out;

    string content = msg->getContent();
    string username = content.substr(0,content.find(' '));
    string password = content.substr(content.find(' ') + 1, content.size());
    if(LOG) cout << username << " - " << password << " request login." << endl;

    if(isRegistered(username, password)){
        if(!isLogged(username)){

            Message broadcast_out(INFO, "", "", "User " + username + " join chat.");
            send_broadcast_message(&broadcast_out);

            out.setCode(SUCCESS);
            out.setSrc("Server");
            out.setDst(username);
            out.setContent("Login successful");

            User* newUser = new User(0, username, password, *client_addr);
            logged_users.push_back(newUser);

            cout << "User " << username << "has logged in." << endl;

        } else{
            // already logged
            out.setCode(ERROR);
            out.setSrc("Server");
            out.setDst(username);
            out.setContent("Login failed. You are already logged in.");

            if(LOG) cout << "Login failed." << endl;

        }
    } else{
        // not registered
        out.setCode(ERROR);
        out.setSrc("Server");
        out.setDst(username);
        out.setContent("Login failed. Wrong username or password.");

        if(LOG) cout << "Login failed." << endl;
    }

    int ret = manager->sendMessage(&out, client_addr);
    if(ret < 0){
        perror("Error during send message operation");
        exit(EXIT_FAILURE);
    }

}

void recv_client_chat(Message *msg){

    struct sockaddr_in* dst_addr;

    User *src = get_logged_user(msg->getSrc());
    if(src == nullptr) cout << "Sender not logged in." << endl;

    User *dst = get_logged_user(msg->getDst());
    if(dst == nullptr){
        cout << "User " << msg->getDst() << " is not logged in or does not exist." << endl;

        msg->setCode(ERROR);
        msg->setContent("User " + msg->getDst() + " not found.");
        msg->setDst(msg->getSrc());
        msg->setSrc("Server");

        dst_addr = &src->address;

    }else {
        cout << "User " << msg->getSrc() << " sent a message to user " << msg->getDst() << "." << endl;
        dst_addr = &dst->address;
    }

    int ret = manager->sendMessage(msg, dst_addr);
    if(ret < 0){
        perror("Error during recv_client_chat");
        exit(EXIT_FAILURE);
    }

}

void recv_client_handshake_audio(Message *msg) {

    User* src = get_logged_user(msg->getSrc());
    if(src == nullptr){
        cout << "[ERROR] Src not found." << endl;
        exit(EXIT_FAILURE);
    }

    User* dst = get_logged_user(msg->getDst());
    if(dst == nullptr){
        cout << "[ERROR] Dst not found." << endl;
        exit(EXIT_FAILURE);
    }

    int ret = manager->sendMessage(msg, &dst->address);
    if(ret < 0){
        perror("Error during recv_client_accept_audio");
        exit(EXIT_FAILURE);
    }

}

void recv_client_accept_audio(Message *msg) {

    User* src = get_logged_user(msg->getSrc());
    if(src == nullptr){
        cout << "[ERROR] Src not found." << endl;
        exit(EXIT_FAILURE);
    }

    User* dst = get_logged_user(msg->getDst());
    if(dst == nullptr){
        cout << "[ERROR] Dst not found." << endl;
        exit(EXIT_FAILURE);
    }

    int ret = manager->sendMessage(msg, &dst->address);
    if(ret < 0){
        perror("Error during recv_client_accept_audio");
        exit(EXIT_FAILURE);
    }
}

void recv_client_refuse_audio(Message *msg) {

    User* src = get_logged_user(msg->getSrc());
    if(src == nullptr){
        cout << "[ERROR] Src not found." << endl;
        exit(EXIT_FAILURE);
    }

    User* dst = get_logged_user(msg->getDst());
    if(dst == nullptr){
        cout << "[ERROR] Dst not found." << endl;
        exit(EXIT_FAILURE);
    }

    int ret = manager->sendMessage(msg, &dst->address);
    if(ret < 0){
        perror("Error during recv_client_refuse_audio");
        exit(EXIT_FAILURE);
    }

}

void recv_client_ringoff_audio(Message *msg) {

    User* src = get_logged_user(msg->getSrc());
    if(src == nullptr){
        cout << "[ERROR] Src not found." << endl;
        exit(EXIT_FAILURE);
    }

    User* dst = get_logged_user(msg->getDst());
    if(dst == nullptr){
        cout << "[ERROR] Dst not found." << endl;
        exit(EXIT_FAILURE);
    }

    int ret = manager->sendMessage(msg, &dst->address);
    if(ret < 0){
        perror("Error during recv_client_ringoff_audio");
        exit(EXIT_FAILURE);
    }

}

void recv_client_request_audio(Message *msg) {

    struct sockaddr_in* dst_addr;

    User* src = get_logged_user(msg->getSrc());
    if(src == nullptr){
        cout << "Fatal error: src not found." << endl;
        exit(EXIT_FAILURE);
    }

    User *dst = get_logged_user(msg->getDst());
    if(dst == nullptr){
        cout << "User " << msg->getDst() << " is not logged in or does not exist." << endl;

        msg->setCode(ERROR);
        msg->setContent("User " + msg->getDst() + " not found.");
        msg->setDst(msg->getSrc());
        msg->setSrc("Server");

        dst_addr = &src->address;

    }else {
        if(src == dst){
            msg->setCode(ERROR);
            msg->setContent("you can't call yourself");
            msg->setDst(msg->getSrc());
            msg->setSrc("Server");

            dst_addr = &src->address;
        }else{
            cout << "User " << msg->getSrc() << " is calling user " << msg->getDst() << "." << endl;
            dst_addr = &dst->address;
        }
    }

    int ret = manager->sendMessage(msg, dst_addr);
    if(ret < 0){
        perror("Error during recv_client_request_audio");
        exit(EXIT_FAILURE);
    }

}

void send_broadcast_message(Message *msg){
    for(User* u: logged_users) manager->sendMessage(msg, &u->address);
}

// Initialization

void init_server() {

    if(LOG) cout << "Read Users from database." << endl;
    registered_users = read_users_from_file("users.txt");

    if(LOG) cout << "Signal handler init." << endl;
    signal_handler_init();
}

int init_server_udp_connection(sockaddr_in socket_address) {

    cout << "UDP protocol setupping..." << endl;

    int ret, socket_udp;
    cout << " - socket.." << endl;
    socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0) return socket_udp;
    cout << " - socket succeeded." << endl;

    cout << " - bind.." << endl;
    ret = bind(socket_udp, (struct sockaddr *) &socket_address, sizeof(socket_address));
    if(ret < 0) return ret;
    cout << " - bind succeeded." << endl;

    cout << "Udp protocol configured." << endl;

    return socket_udp;
}

void close_udp_connection(int socket) {

    // close the descriptor
    int ret = close(socket);
    if(ret < 0){
        perror("Error during close operation");
        exit(EXIT_FAILURE);
    }

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
    Message out(QUIT, "", "", "");
    send_broadcast_message(&out);

    running = false;
    delete manager;
}


// Utility

void print_registered_users() {

    cout << "Registered users: " << endl;
    for(User &u : registered_users){
        cout << " - " << u.username << " " << u.password << endl;
    }

}

string get_logged_client_list() {
    string logged_client_list = "";
    for(User* &u: logged_users) logged_client_list.append(" - " + u->username);
    return logged_client_list;
}

User* get_logged_user(string username) {
    for(User* &u : logged_users)
        if(username.compare(u->username) == 0) return u;
    return nullptr;
}

void print_logged_users() {
    cout << "Logged users: " << endl;
    for(User* &u : logged_users){
        cout << " - " << u->username << " " << u->password << " "
             << u->address.sin_addr.s_addr << " " << u->address.sin_port
             << u->address.sin_family << " " << u->address.sin_zero << endl;

    }
}

bool isRegistered(string username, string password) {
    for (User &u : registered_users ) if(username.compare(u.username) == 0 && password.compare(u.password) == 0) return true;
    return false;
}

bool isLogged(string username) {
    for(User* u : logged_users) if(username.compare(u->username) == 0) return true;
    return false;
}

void receiver() {


    struct sockaddr_in client_address{};
    Message in;
    manager->recvMessage(&in, &client_address);
    if(LOG) cout << "recv: " << in << endl;


    switch (in.getCode()) {
        case AUTHENTICATION:
            if(LOG) cout << "Authentication message arrived." << endl;
            cout << "New user is trying to authenticate." << endl;
            recv_client_authentication(&client_address, &in);
            break;
        case CHAT:
            if(LOG) cout << "Chat message arrived." << endl;
            cout << "User " << in.getSrc() << " wants to send a message to " << in.getDst() << "." << endl;
            recv_client_chat(&in);
            break;
        case USERS:
            if(LOG) cout << "Users message arrived." << endl;
            cout << "User " << in.getSrc() << " requests users list." << endl;
            recv_client_users(&in);
            break;
        case QUIT:
            if(LOG) cout << "Quit message arrived." << endl;
            cout << "User " << in.getSrc() << " wants to exit." << endl;
            recv_client_quit(&in);
            break;
        case AUDIO:
            if(LOG) cout << "Audio message arrived." << endl;
            cout << "User " << in.getSrc() << " wants to call " << in.getDst() << "." << endl;
            recv_client_request_audio(&in);
            break;
        case ACCEPT:
            if(LOG) cout << "Accept message arrived." << endl;
            cout << "User " << in.getSrc() << " accepted call from " << in.getDst() << "." << endl;
            recv_client_accept_audio(&in);
            break;
        case REFUSE:
            if(LOG) cout << "Refuse message arrived." << endl;
            cout << "User " << in.getSrc() << " refused call." << endl;
            recv_client_refuse_audio(&in);
            break;
        case RINGOFF:
            if(LOG) cout << "Ringoff message arrived." << endl;
            cout << "User " << in.getSrc() << " ring off." << endl;
            recv_client_ringoff_audio(&in);
            break;
        case HANDSHAKE:
            if(LOG) cout << "Handshake message arrived." << endl;
            recv_client_handshake_audio(&in);
            break;
        default:
            if(LOG) cout << "Default message arrived." << endl;
            break;
    }

}

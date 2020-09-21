//
// Created by leonardo on 13/05/20.
//

#include "client.h"

#include "config.h"

using namespace std;


int  udp_socket;

string username;

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

    client_status = SUCCESS;

    cout << "Welcome " << username << endl;

    // Handles writing to the shell
    thread send_thread(sender);

    running = true;
    while (running){
        if(input_available(udp_socket)){
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
    strcpy(msg, to_string(CODE::QUIT).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str());

    if(LOG){
        cout << "send: ";
        print_message(msg);
    }
    int ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }
}

void client_users() {
    client_status = CODE::USERS;

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::USERS).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str());

    if(LOG){
        cout << "send: ";
        print_message(msg);
    }

    int ret;
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation.");
        exit(EXIT_FAILURE);
    }




}

void client_chat() {

    client_status = CODE::CHAT;

    cout << "Type recipient: " << endl;
    string dst;
    if(input_available(0))
        getline(cin,dst);
    else return;


    cout << "Type message: " << endl;
    string message;
    if(input_available(0))
        getline(cin,message);
    else return;


    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg,to_string(CODE::CHAT).c_str()); // code
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str()); // src
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, dst.c_str()); // dst
    strcpy(msg + MSG_HEADER_SIZE, message.c_str()); // content


    if(LOG){
        cout << "send: ";
        print_message(msg);
    }

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
    memset(msg, 0, MSG_SIZE);
    sprintf(msg, "%d", CODE::AUTHENTICATION);
    sprintf(msg + MSG_HEADER_SIZE, "%s", credential);

    if(LOG){
        cout << "send: ";
        print_message(msg);
    }

    int ret;
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr *) NULL, 0);
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

    if(LOG){
        cout << "recv: ";
        print_message(msg);
    }

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

    if(LOG){
        cout << "recv: ";
        print_message(msg);
    }
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
            cout << "[Server] Service interrupted." << endl;
            running = false;
            break;
        case USERS:
            cout << "Logged users:" << endl << msg + MSG_HEADER_SIZE << endl;
            break;
        case ERROR:
            cout << "[Server] " << msg + MSG_HEADER_SIZE << endl;
            break;
        case INFO:
            cout << "[Server] " << msg + MSG_HEADER_SIZE << endl;
            break;
        default:
            break;
    }



}

void sender(){
    while (running){
        print_info_message();

        string line;
        if(input_available(0))
            getline(cin,line);
        else break;

        if(line.compare("chat") == 0 ) client_chat();
        else if(line.compare("users") == 0 ) client_users();
        else if(line.compare("quit") == 0 ) client_quit();
        else if(line.compare("video") == 0 ) client_video();
        else if(line.compare("audio") == 0 ) client_audio();
        else cout << "Unknown command." << endl;


    }
}

bool input_available(int fd) {
    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(fd, &set); // add server descriptor on set
    // set timeout
    timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (running){
        int ret = select( fd + 1, &set, NULL, NULL, &timeout);
        if(ret < 0) {
            if(errno == EINTR) continue;
            perror("Error during select operation: ");
            exit(EXIT_FAILURE); // error
        }else if(ret == 0) {
            // timeout occurred
            timeout.tv_sec = 1;
            FD_ZERO(&set); // clear set
            FD_SET(fd, &set);
            // timeout occurred
            if(!running) break;
        }else return true; // input available
    }
    return false;
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

    struct sockaddr_in server_address;

    // clear servaddr
    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_address.sin_port = htons(SERVER_CHAT_PORT);
    server_address.sin_family = AF_INET;

    // create datagram socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // connect to server
    if(connect(udp_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
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
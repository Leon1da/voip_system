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

sem_t mutex;
int calling;
Peer* connected_peer;

void called_routine();
void caller_routine();

void client_audio_accept();

void recv_audio_request(char* msg);

void client_audio_refuse();

void client_audio_ringoff();

void recv_audio_accept(char *msg);

void safe_peer_delete();

void recv_audio_ringoff();

void unblocked_recvfrom(int socket, char *buf, int buf_size);

void recv_audio_refuse();

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

    int ret = sem_init(&mutex, 0, 1);
    if(ret < 0){
        perror("Error during sem_init operation");
        exit(EXIT_FAILURE);
    }

    calling = false;
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
            << "Type <audio> to call " << endl
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

/*
 * Request audio with a peer
 */
void client_audio_request() {

    client_status = CODE::AUDIO;

    // client select dst of call
    cout << "Type recipient: " << endl;
    string dst;
    if(input_available(0))
        getline(cin,dst);
    else return;

    // init peer socket
    cout << "P2P Udp protocol setupping..." << endl;
    struct sockaddr_in peer_address;
    bzero(&peer_address, sizeof(peer_address));
    peer_address.sin_addr.s_addr = INADDR_ANY; //SERVER_ADDRESS
    peer_address.sin_port = 0;
    peer_address.sin_family = AF_INET;

    int peer_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(peer_socket < 0){
        perror("Error during socket operation.");
        exit(EXIT_FAILURE);
    }

    // bind server address to socket descriptor
    int ret = bind(peer_socket, (struct sockaddr *) &peer_address, sizeof(peer_address));
    if(ret < 0){
        perror("Error during bind operation.");
        exit(EXIT_FAILURE);
    }
    cout << "P2P Udp protocol configured." << endl;

    // preparo ip e porta da mandare al client destinatario
    struct sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);
    getsockname(peer_socket, (struct sockaddr*)&peer_addr, &len);

    string addr = to_string(htonl(peer_addr.sin_addr.s_addr)).append(" ").append(to_string(ntohs(peer_addr.sin_port)));
    cout << "peer_addr.sin_addr: " << inet_ntoa(peer_addr.sin_addr) << endl;
    cout << "peer_addr.sin_addr.s_addr: " << to_string(htonl(peer_addr.sin_addr.s_addr)) << endl;
    cout << "peer_addr.sin_port: " << to_string(ntohs(peer_addr.sin_port)) << endl;

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg,to_string(CODE::AUDIO).c_str()); // code
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str()); // src
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, dst.c_str()); // dst
    strcpy(msg + MSG_HEADER_SIZE, addr.c_str());

    if(LOG){
        cout << "send: ";
        print_message(msg);
    }

    // invio al server i dati da girare al peer destinatario
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation.");
        exit(EXIT_FAILURE);
    }
    cout << "Inviata richiesta al Server." << endl;

    calling = true;
    connected_peer = new Peer();
    connected_peer->set_peer_name(dst);
    connected_peer->set_peer_socket(peer_socket);

}

/*
 * Accept audio request from peer
 */
void client_audio_accept() {
    int ret;

    // creo la socket per gestire la comunicazione
    struct sockaddr_in peer_address;
    bzero(&peer_address, sizeof(peer_address));
    peer_address.sin_addr.s_addr = connected_peer->get_peer_address().sin_addr.s_addr; // inet_addr(SERVER_ADDRESS);
    peer_address.sin_port = connected_peer->get_peer_address().sin_port; // htons(SERVER_CHAT_PORT);
    peer_address.sin_family = AF_INET;

    // create datagram socket
    int peer_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(peer_socket < 0){
        perror("Error during socket operation.");
        exit(EXIT_FAILURE);
    }

    cout << "Connecting." << endl;
    // connect to server
    ret = connect(peer_socket, (struct sockaddr *)&peer_address, sizeof(peer_address));
    if(ret < 0) {
        perror("Error during connect operation");
        exit(EXIT_FAILURE);
    }
    cout << "Connected." << endl;


    // invio al server ip e address da inoltrare al client che ha richiesto la chiamata
    struct sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);
    ret = getsockname(peer_socket, (struct sockaddr*)&peer_addr, &len);
    if(ret < 0){
        perror("Error during getsockname operation");
        exit(EXIT_FAILURE);
    }

    string addr = to_string(htonl(peer_addr.sin_addr.s_addr)).append(" ").append(to_string(ntohs(peer_addr.sin_port)));
    cout << "peer_addr.sin_addr: " << inet_ntoa(peer_addr.sin_addr) << endl;
    cout << "peer_addr.sin_addr.s_addr: " << to_string(htonl(peer_addr.sin_addr.s_addr)) << endl;
    cout << "peer_addr.sin_port: " << to_string(ntohs(peer_addr.sin_port)) << endl;

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::ACCEPT).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, connected_peer->get_peer_name().c_str());
    strcpy(msg + MSG_HEADER_SIZE, addr.c_str());

    ret = sendto(udp_socket, msg, MSG_SIZE, 0, (struct sockaddr*) NULL, 0);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }

    connected_peer->set_peer_socket(peer_socket);

    // creo un trheaad che gestisce lo scambio di dati tra i due peers
    thread called_thread(called_routine);
//    called_thread.join();
    called_thread.detach();

}

/*
 * Refused audio request from peer
 */
void client_audio_refuse() {

    // il client rifiuta la chiamata
    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::REFUSE).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, connected_peer->get_peer_name().c_str());

    int ret;
    ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation.");
        exit(EXIT_FAILURE);
    }

    safe_peer_delete();

}

/*
 * Ringoff audio call
 */
void client_audio_ringoff() {

    string peer_name = connected_peer->get_peer_name();

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::RINGOFF).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, peer_name.c_str());


    if(LOG){
        cout << "send: ";
        print_message(msg);
    }
    cout << "Sending Ringoff." << endl;
    int ret = sendto(udp_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }
    cout << "Sent Ringoff." << endl;

    safe_peer_delete();

}

void called_routine(){
    cout << "Called routine start." << endl;

    int ret, counter = 0;

    int socket = connected_peer->get_peer_socket();
    sockaddr_in address = connected_peer->get_peer_address();
    int address_len = sizeof(address);

    char msg[MSG_SIZE];
    while(calling){

        cout << "sendto.." << endl;
        if(calling){
            ret = sendto(socket, to_string(counter++).c_str(), MSG_SIZE, 0, (struct sockaddr*) &address, address_len);
            if(ret < 0) {
                perror("Error during send operation");
                exit(EXIT_FAILURE);
            }
        }
        cout << "sendto ok." << endl;

        cout << "recvfrom." << endl;

        if(calling){
            memset(msg, 0, MSG_SIZE);
            unblocked_recvfrom(socket, msg, MSG_SIZE);
        }
        cout << "recvfrom ok." << endl;

    }

    cout << "Called routine end." << endl;
}

void caller_routine(){
    cout << "Caller routine start." << endl;
    int ret, counter = 0;

    int socket = connected_peer->get_peer_socket();
    sockaddr_in address = connected_peer->get_peer_address();
    int address_len = sizeof(address);

    char msg[MSG_SIZE];
    while(calling){

        if(LOG) cout << "recvfrom." << endl;
        if(calling){
            memset(msg, 0, MSG_SIZE);
            unblocked_recvfrom(socket, msg, MSG_SIZE);
        }
        if(LOG) cout << "recvfrom ok." << endl;

        if(LOG) cout << "sendto." << endl;
        if(calling){
            ret = sendto(socket, to_string(counter++).c_str(), MSG_SIZE, 0, (struct sockaddr*) &address, address_len);
            if(ret < 0) {
                perror("Error during send operation");
                exit(EXIT_FAILURE);
            }
        }
        if(LOG) cout << "sendto ok." << endl;

    }

    cout << "Caller routine end." << endl;
}

void unblocked_recvfrom(int socket, char *buf, int buf_size) {

    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(socket, &set); // add server descriptor on set
    // set timeout
    timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int ret;
    ret = select( socket + 1, &set, NULL, NULL, &timeout);
    if(ret < 0) {
        perror("Error during select operation");
        exit(EXIT_FAILURE); // error
    }else if(ret == 0) {
        // timeout occurred
        cout << "timeout." << endl;

    }else{
        ret = recvfrom(socket, buf, buf_size, 0, NULL, NULL);
        if(ret < 0) {
            perror("Error during recv operation");
            exit(EXIT_FAILURE);
        }

    }
}

/*
 * Client receive audio request
 */
void recv_audio_request(char* msg) {


    if(LOG) print_message(msg);

    string addr_info = msg + MSG_HEADER_SIZE;
    string s_addr_string = addr_info.substr(0, addr_info.find(' '));
    string sin_port_string = addr_info.substr(addr_info.find(' ') + 1, addr_info.size());
    // cout << "peer_address.sin_addr.s_addr: " << s_addr_string << " - peer_address.sin_port: " << sin_port_string << endl;

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(s_addr_string.c_str());
    addr.sin_port = htons(atoi(sin_port_string.c_str()));
    addr.sin_family = AF_INET;

    calling = true;
    connected_peer = new Peer();
    connected_peer->set_peer_name(msg + MSG_H_CODE_SIZE);
    connected_peer->set_peer_address(addr);

}

/*
 * Client receive accept (for audio request) from other client
 */
void recv_audio_accept(char *msg) {

    if(LOG) print_message(msg);

    string addr_info = msg + MSG_HEADER_SIZE;
    string s_addr_string = addr_info.substr(0, addr_info.find(' '));
    string sin_port_string = addr_info.substr(addr_info.find(' ') + 1, addr_info.size());
    // cout << "peer_address.sin_addr.s_addr: " << s_addr_string << " - peer_address.sin_port: " << sin_port_string << endl;

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(s_addr_string.c_str());
    addr.sin_port = htons(atoi(sin_port_string.c_str()));
    addr.sin_family = AF_INET;

    string peer_name = msg + MSG_H_CODE_SIZE;


    connected_peer->set_peer_address(addr);

    thread caller_thread(caller_routine);
//    caller_thread.join();
    caller_thread.detach();
}

void safe_peer_delete() {

    int ret;

    calling = false;
    if(connected_peer != nullptr) {
        if(connected_peer->is_socket_init()){
            ret = close(connected_peer->get_peer_socket());
            if(ret < 0){
                perror("Error during close operation");
                exit(EXIT_FAILURE);
            }
        }

        delete connected_peer;
        connected_peer = nullptr;

    }

}

void recv_audio_ringoff() {
    safe_peer_delete();
}


void sender(){
    while (running){
        print_info_message();

        string line;
        if(input_available(0))
            getline(cin,line);
        else break;

        // if(calling && socket) refuse
        // else if(calling && address) refuse or accept
        // else if(calling && socket && address) ringoff
        // else if(!calling) tutto il resto

        if(!calling){
            if(line.compare("chat") == 0 ) client_chat();
            else if(line.compare("users") == 0 ) client_users();
            else if(line.compare("quit") == 0 ) client_quit();
            else if(line.compare("video") == 0 ) client_video();
            else if(line.compare("audio") == 0 ) client_audio_request(); // client vuole chiamare un client
            else cout << "Unknown command." << endl;
        }else{
            if(line.compare("accept") == 0 ) client_audio_accept(); // client accetta chiamata di un client
            else if(line.compare("refuse") == 0 ) client_audio_refuse(); // client rifiuta chiamata di un client
            else if(line.compare("ringoff") == 0 ) client_audio_ringoff(); // client attacca mentre e` in chimata con un client
            else cout << "Unknown command." << endl;

        }

//
//
//        if(line.compare("chat") == 0 ) client_chat();
//        else if(line.compare("users") == 0 ) client_users();
//        else if(line.compare("quit") == 0 ) client_quit();
//        else if(line.compare("video") == 0 ) client_video();
//        else if(line.compare("audio") == 0 ) client_audio_request(); // client vuole chiamare un client
//        else if(line.compare("accept") == 0 ) client_audio_accept(); // client accetta chiamata di un client
//        else if(line.compare("refuse") == 0 ) client_audio_refuse(); // client rifiuta chiamata di un client
//        else if(line.compare("ringoff") == 0 ) client_audio_ringoff(); // client attacca mentre e` in chimata con un client
//        else cout << "Unknown command." << endl;
    }
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
        case AUDIO:
            // il server mi comunica che un client mi sta chiamando
            cout << "Receive audio request." << endl;
            recv_audio_request(msg);
            break;
        case ACCEPT:
            // il server mi comunica che il client ha accettato la chiamata
            cout << "Receive audio accept." << endl;
            recv_audio_accept(msg);
            break;
        case REFUSE:
            // il server mi comunica che il client ha rifiutato la chiamata
            cout << "Receive audio refuse." << endl;
            recv_audio_refuse();
            break;
        case RINGOFF:
            cout << "Receive audio ringoff." << endl;
            recv_audio_ringoff();
            break;
        default:
            break;
    }



}

void recv_audio_refuse() {
    safe_peer_delete();
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
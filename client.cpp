//
// Created by leonardo on 13/05/20.
//

#include "client.h"

#include "config.h"

using namespace std;


int  client_server_socket; // for client/server connection

string username;

CODE client_status;
bool running;

int calling;
Peer* connected_peer;


int init_server_udp_connection(sockaddr_in socket_address);

int init_client_udp_connection(sockaddr_in socket_address);

int main(int argc, char *argv[])
{
    cout << "Signal handler setup." << endl;
    signal_handler_init();
    cout << "Signal handler ok." << endl;

    cout << "Client/Server connection setupping." << endl;

    struct sockaddr_in server_address{};
    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_address.sin_port = htons(SERVER_CHAT_PORT);
    server_address.sin_family = AF_INET;

    client_server_socket = init_client_udp_connection(server_address);
    if(client_server_socket < 0){
        perror("Error during udp connection init.");
        exit(EXIT_FAILURE);
    }
    cout << "Client/server connection ok." << endl;

    // authentication
    while (!client_authentication());

    client_status = SUCCESS;

    cout << "Welcome " << username << endl;
    print_info_message();

    // Handles writing to the shell
    thread send_thread(sender);

    calling = false;
    running = true;
    while (running){
        if(input_available(client_server_socket)){
            receiver();
        }
    }

    cout << "Closing client." << endl;

    send_thread.join();

    cout << "Closing client/server connection." << endl;
    close_udp_connection(client_server_socket);
    cout << "client/server connection closed." << endl;

    return EXIT_SUCCESS;
}

void print_info_message() {
    cout << "Type <users> to see users online. " << endl
            << "Type <chat> to send message. " << endl
            << "Type <audio> to call. " << endl
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
    int ret = sendto(client_server_socket, msg, MSG_SIZE, 0, nullptr, 0);
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
    ret = sendto(client_server_socket, msg, MSG_SIZE, 0, nullptr, 0);
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
    ret = sendto(client_server_socket, msg, MSG_SIZE, 0, nullptr, 0);
    if(ret < 0){
        perror("Error during send operation.");
        exit(EXIT_FAILURE);
    }

}

bool client_authentication() {
    client_status = CODE::AUTHENTICATION;

    cout << "Digit <username> <password> for access to service." << endl;
    char credential[MSG_CONTENT_SIZE];
    cin.getline(credential, MSG_CONTENT_SIZE);

    regex r("[a-zA-Z0-9]+ [a-zA-Z0-9]+");
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
    ret = sendto(client_server_socket, msg, MSG_SIZE, 0, (struct sockaddr *) nullptr, 0);
    if(ret < 0) {
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }


    memset(msg, 0, MSG_SIZE);
    ret = recvfrom(client_server_socket, msg, MSG_SIZE, 0, (struct sockaddr *) nullptr, nullptr);
    if(ret < 0){
        perror("Error during recv operation");
        exit(EXIT_FAILURE);
    }

    if(LOG){
        cout << "recv: ";
        print_message(msg);
    }

    cout << "[Server] " << msg + MSG_HEADER_SIZE << endl;

    if(strcmp(msg, to_string(CODE::SUCCESS).c_str()) == 0){
        username = msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE;
        return true;
    }
    return false;

}

/*
 * Request audio with a peer
 */
void client_audio_request() {

    client_status = CODE::AUDIO;

    if(LOG) cout << "Client audio request." << endl;

    // client select dst of call
    cout << "Type recipient: " << endl;
    string dst;
    if(input_available(0))
        getline(cin,dst);
    else return;

    struct sockaddr_in peer_address{};
    bzero(&peer_address, sizeof(peer_address));
    peer_address.sin_addr.s_addr = INADDR_ANY;
    peer_address.sin_port = 0;
    peer_address.sin_family = AF_INET;

    int socket = init_server_udp_connection(peer_address);
    if(socket < 0){
        perror("Error during init_udp_connection");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in local_peer_addr{};
    socklen_t len = sizeof(local_peer_addr);
    int ret = getsockname(socket, (struct sockaddr*)&local_peer_addr, &len);
    if(ret < 0){
        perror("Error during getsockname operation.");
        exit(EXIT_FAILURE);
    }

    string local_peer_addr_string = to_string(htonl(local_peer_addr.sin_addr.s_addr)).append(" ").append(to_string(ntohs(local_peer_addr.sin_port)));

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg,to_string(CODE::AUDIO).c_str()); // code
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str()); // src
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, dst.c_str()); // dst
    strcpy(msg + MSG_HEADER_SIZE, local_peer_addr_string.c_str());

    if(LOG){
        cout << "send: ";
        print_message(msg);
    }

    ret = sendto(client_server_socket, msg, MSG_SIZE, 0, nullptr, 0);
    if(ret < 0){
        perror("Error during send operation.");
        exit(EXIT_FAILURE);
    }

    calling = true;
    connected_peer = new Peer();
    connected_peer->set_peer_name(dst);
    connected_peer->set_peer_socket(socket);

    if(LOG) cout << "Client audio request sent." << endl;

}

/*
 * Accept audio request from peer
 */
void client_audio_accept() {

    client_status = ACCEPT;

    sockaddr_in remote_peer_addr = connected_peer->get_peer_address();
    remote_peer_addr.sin_family = AF_INET;

    int peer_socket = init_client_udp_connection(remote_peer_addr);
    if(peer_socket < 0){
        perror("Error during init_client_udp_connection operation.");
        exit(EXIT_FAILURE);
    }


    // get sock info for client
    struct sockaddr_in peer_addr{};
    socklen_t len = sizeof(peer_addr);
    int ret = getsockname(peer_socket, (struct sockaddr*)&peer_addr, &len);
    if(ret < 0){
        perror("Error during getsockname operation");
        exit(EXIT_FAILURE);
    }

    string addr = to_string(htonl(peer_addr.sin_addr.s_addr)).append(" ").append(to_string(ntohs(peer_addr.sin_port)));
    if(LOG){
        cout << "peer_addr.sin_addr: " << inet_ntoa(peer_addr.sin_addr) << endl;
        cout << "peer_addr.sin_addr.s_addr: " << to_string(htonl(peer_addr.sin_addr.s_addr)) << endl;
        cout << "peer_addr.sin_port: " << to_string(ntohs(peer_addr.sin_port)) << endl;
    }

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::ACCEPT).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, connected_peer->get_peer_name().c_str());
    strcpy(msg + MSG_HEADER_SIZE, addr.c_str());

    ret = sendto(client_server_socket, msg, MSG_SIZE, 0, (struct sockaddr*) NULL, 0);
    if(ret < 0){
        perror("Error during send operation: ");
        exit(EXIT_FAILURE);
    }

    connected_peer->set_peer_socket(peer_socket);

    thread called_thread(called_routine);
    called_thread.detach();

}

/*
 * Refused audio request from peer
 */
void client_audio_refuse() {


    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(CODE::REFUSE).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, username.c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, connected_peer->get_peer_name().c_str());

    int ret;
    ret = sendto(client_server_socket, msg, MSG_SIZE, 0, nullptr, 0);
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

    int ret = sendto(client_server_socket, msg, MSG_SIZE, 0, NULL, 0);
    if(ret < 0){
        perror("Error during send operation");
        exit(EXIT_FAILURE);
    }

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

void recv_audio_ringoff() {
    safe_peer_delete();
}

void recv_audio_refuse() {
    safe_peer_delete();
}

void safe_peer_delete() {

    calling = false;
    if(connected_peer != nullptr){
        if(connected_peer->is_socket_init())
            close_udp_connection(connected_peer->get_peer_socket());

        delete connected_peer;
        connected_peer = nullptr;

    }

}


void sender(){
    while (running){

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
            else{
                cout << "Unknown command." << endl;
                print_info_message();
            }

        }else{
            if(client_status == AUDIO){
                if(connected_peer->is_address_init()){
                    if(line.compare("accept") == 0 ) client_audio_accept(); // client accetta chiamata di un client
                    else if(line.compare("refuse") == 0 ) client_audio_refuse(); // client rifiuta chiamata di un client
                    else cout << "Incaming call.. <accept> or <refuse> ?" << endl;
                } else{
                    if(line.compare("refuse") == 0 ) client_audio_refuse(); // client rifiuta chiamata di un client
                    else cout << "Outgoing call.. <refuse> ?" << endl;
                }
            } else if(client_status == ACCEPT){
                if(line.compare("ringoff") == 0 ) client_audio_ringoff(); // client attacca mentre e` in chimata con un client
                else cout << "Calling in progress.. <ringoff> ?" << endl;
            } else cout << "Unknown status." << endl;

        }

    }
}

void receiver() {

    int ret;

    char msg[MSG_SIZE];
    ret = recvfrom(client_server_socket, msg, MSG_SIZE, 0, (struct sockaddr *) NULL, 0);
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
            client_status = ERROR;
            safe_peer_delete();
            break;
        case INFO:
            cout << "[Server] " << msg + MSG_HEADER_SIZE << endl;
            break;
        case AUDIO:
            // il server mi comunica che un client mi sta chiamando
            cout << "Receive audio request." << endl;
            client_status = AUDIO;
            recv_audio_request(msg);
            break;
        case ACCEPT:
            // il server mi comunica che il client ha accettato la chiamata
            cout << "Receive audio accept." << endl;
            client_status = ACCEPT;
            recv_audio_accept(msg);
            break;
        case REFUSE:
            // il server mi comunica che il client ha rifiutato la chiamata
            cout << "Receive audio refuse." << endl;
            client_status = REFUSE;
            recv_audio_refuse();
            break;
        case RINGOFF:
            cout << "Receive audio ringoff." << endl;
            client_status = RINGOFF;
            recv_audio_ringoff();
            break;
        default:
            break;
    }



}

bool input_available(int fd) {
    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(fd, &set); // add server descriptor on set
    // set timeout
    timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (running){
        int ret = select( fd + 1, &set, nullptr, nullptr, &timeout);
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



// Initialization

int init_server_udp_connection(sockaddr_in socket_address) {

    cout << "UDP protocol setupping..." << endl;

    int ret, socket_udp;
    cout << " - socket." << endl;
    socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0) return socket_udp;
    cout << " - socket succeeded." << endl;

    cout << " - bind." << endl;
    ret = bind(socket_udp, (struct sockaddr *) &socket_address, sizeof(socket_address));
    if(ret < 0) return ret;
    cout << " - bind succeeded." << endl;

    cout << "Udp protocol configured." << endl;

    return socket_udp;
}

int init_client_udp_connection(sockaddr_in socket_address) {

    int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0) return  socket_udp;

    int ret = connect(socket_udp, (struct sockaddr *)&socket_address, sizeof(socket_address));
    if(ret < 0) return ret;

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

void signal_handler_init() {
    /* signal handler */
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigintHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0; //SA_RESTART
    sigaction(SIGINT, &sigIntHandler, NULL);
    /* end signal handler */

}


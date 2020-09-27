//
// Created by leonardo on 13/05/20.
//

#include "client.h"


using namespace std;


MessageManager* manager;
thread* send_thread;

string username;

CODE client_status;
bool running;

int calling;
Peer* connected_peer;



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

    int client_server_socket = init_client_udp_connection(server_address);
    if(client_server_socket < 0){
        perror("Error during udp connection init.");
        exit(EXIT_FAILURE);
    }
    cout << "Client/server connection ok." << endl;

    manager = new MessageManager(client_server_socket, server_address);

    // authentication
    int ret;
    do{
        ret = client_authentication();
        if(ret < 0){
            perror("Error during client_authentication");
            close_udp_connection(client_server_socket);
            delete manager;
            exit(EXIT_FAILURE);
        }
    } while (ret != SUCCESS);

    client_status = SUCCESS;

    cout << "Welcome " << username << endl;
    print_info_message();

    // Handles writing to the shell
    send_thread = new thread(sender);


    calling = false;
    running = true;
    while (running){
        if(input_available(client_server_socket)){
            receiver();
        }
    }

    cout << "Closing client." << endl;

    send_thread->join();
    delete send_thread;

    delete manager;

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

// Functionality

void client_video() {
    client_status = CODE::VIDEO;
}

void client_quit() {

    client_status = CODE::QUIT;
    running = false;

    Message out(QUIT, username, "", "");
    cout << "send: " << out << endl;
    manager->sendMessage(&out);

}

void client_users() {

    client_status = CODE::USERS;

    Message out(USERS, username, "", "");
    cout << "send: " << out << endl;
    manager->sendMessage(&out);

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

    Message out(CHAT, username, dst, message);
    cout << "send: " << out << endl;
    manager->sendMessage(&out);

}

int client_authentication() {

    client_status = CODE::AUTHENTICATION;

    cout << "Digit <username> <password> for access to service." << endl;
    char credential[MSG_CONTENT_SIZE];
    cin.getline(credential, MSG_CONTENT_SIZE);

    regex r("[a-zA-Z0-9]+ [a-zA-Z0-9]+");
    if(!regex_match(credential, r)){
        cout << "No pattern mathing (<username> <password>)." << endl;
        return false;
    }

    int ret;
    Message out(AUTHENTICATION, "", "", credential);
    cout << "send: " << out << endl;
    ret = manager->sendMessage(&out);
    if(ret < 0) return ret;

    Message in;
    ret = manager->recvMessage(&in);
    if(ret < 0) return ret;
    cout << "recv: " << in << endl;

    cout << "[Server] " << in.getContent() << endl;

    if(in.getCode() == SUCCESS){
        username = in.getDst();
        return SUCCESS;
    }
    return ERROR;

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

    Message out(AUDIO, username, dst, local_peer_addr_string);
    cout << "send: " << out << endl;
    manager->sendMessage(&out);

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

    string address_string = to_string(htonl(peer_addr.sin_addr.s_addr)).append(" ").append(to_string(ntohs(peer_addr.sin_port)));
    if(LOG){
        cout << "peer_addr.sin_addr: " << inet_ntoa(peer_addr.sin_addr) << endl;
        cout << "peer_addr.sin_addr.s_addr: " << to_string(htonl(peer_addr.sin_addr.s_addr)) << endl;
        cout << "peer_addr.sin_port: " << to_string(ntohs(peer_addr.sin_port)) << endl;
    }

    Message out(ACCEPT, username, connected_peer->get_peer_name(), address_string);
    cout << "send: " << out << endl;
    manager->sendMessage(&out);


    connected_peer->set_peer_socket(peer_socket);

    thread called_thread(called_routine);
    called_thread.detach();

}

/*
 * Refused audio request from peer
 */
void client_audio_refuse() {
    if(connected_peer != nullptr && connected_peer->is_name_init()){
        Message out(REFUSE, username, connected_peer->get_peer_name(), "");
        cout << "send: " << out << endl;
        manager->sendMessage(&out);

    }

    safe_peer_delete();

}

/*
 * Ringoff audio call
 */
void client_audio_ringoff() {
    if(connected_peer != nullptr && connected_peer->is_name_init()) {
        Message out(RINGOFF, username, connected_peer->get_peer_name(), "");
        cout << "send: " << out << endl;
        manager->sendMessage(&out);
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
            memset(msg, 0, MSG_SIZE);
            ret = sendto(socket, msg, MSG_SIZE, 0, (struct sockaddr*) &address, address_len);
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
            memset(msg, 0, MSG_SIZE);
            ret = sendto(socket, msg, MSG_SIZE, 0, (struct sockaddr*) &address, address_len);
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
void recv_audio_request(Message *msg) {

    client_status = AUDIO;

    string content = msg->getContent();
    string s_addr_string = content.substr(0, content.find(' '));
    string sin_port_string = content.substr(content.find(' ') + 1, content.size());

    struct sockaddr_in remote_peer_address{};
    bzero(&remote_peer_address, sizeof(remote_peer_address));
    remote_peer_address.sin_addr.s_addr = inet_addr(s_addr_string.c_str());
    remote_peer_address.sin_port = htons(atoi(sin_port_string.c_str()));
    remote_peer_address.sin_family = AF_INET;

    calling = true;
    connected_peer = new Peer();
    connected_peer->set_peer_name(msg->getSrc());
    connected_peer->set_peer_address(remote_peer_address);


}

/*
 * Client receive accept (for audio request) from other client
 */
void recv_audio_accept(Message *msg) {

    client_status = ACCEPT;

    string content = msg->getContent();
    string s_addr_string = content.substr(0, content.find(' '));
    string sin_port_string = content.substr(content.find(' ') + 1, content.size());

    struct sockaddr_in addr{};
    bzero(&addr, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(s_addr_string.c_str());
    addr.sin_port = htons(atoi(sin_port_string.c_str()));
    addr.sin_family = AF_INET;

    string peer_name = msg->getSrc();

    connected_peer->set_peer_address(addr);

    thread caller_thread(caller_routine);
    caller_thread.detach();

}

void recv_audio_ringoff() {
    client_status = RINGOFF;
    safe_peer_delete();
}

void recv_audio_refuse() {
    client_status = REFUSE;
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

    Message in;
    manager->recvMessage(&in);
    cout << "recv: " << in << endl;

    switch (in.getCode()) {
        case CHAT:
            cout << "[" << in.getSrc() << "] " << in.getContent() << endl;
            break;
        case QUIT:
            cout << "[Server] Service interrupted." << endl;
            running = false;
            break;
        case USERS:
            cout << "Logged users:" << endl << in.getContent() << endl;
            break;
        case ERROR:
            cout << "[Server] " << in.getContent() << endl;
            client_status = ERROR;
            safe_peer_delete();
            break;
        case INFO:
            cout << "[Server] " << in.getContent() << endl;
            break;
        case AUDIO:
            // il server mi comunica che un client mi sta chiamando
            cout << "Receive audio request." << endl;
            recv_audio_request(&in);
            break;
        case ACCEPT:
            // il server mi comunica che il client ha accettato la chiamata
            cout << "Receive audio accept." << endl;
            recv_audio_accept(&in);
            break;
        case REFUSE:
            // il server mi comunica che il client ha rifiutato la chiamata
            cout << "Receive audio refuse." << endl;
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

int init_client_udp_connection(sockaddr_in socket_address) {

    cout << "UDP protocol setupping..." << endl;

    cout << " -  socket.." << endl;
    int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0) return  socket_udp;
    cout << " - socket succeeded." << endl;

    cout << " - connect.." << endl;
    int ret = connect(socket_udp, (struct sockaddr *)&socket_address, sizeof(socket_address));
    if(ret < 0) return ret;
    cout << " - connect succeeded.." << endl;

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

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    // signal(SIGINT, sigintHandler);
    cout << endl <<"Catch Ctrl+C - System status: " << client_status << endl;
    switch (client_status) {
        case CODE::AUTHENTICATION:
            cout << "Authentication stopped." << endl;
            break;
        case CODE::AUDIO:
            cout << "Audio request stopped." << endl;
            client_audio_refuse();
            break;
        case CODE::ACCEPT:
            cout << "Audio accept stopped." << endl;
            client_audio_ringoff();
            break;
        case CODE::RINGOFF:
            cout << "Audio Ringoff stopped." << endl;
            client_audio_ringoff();
            break;
        default:
            cout << "Service Stopped." << endl;
            break;
    }

    if(client_status != AUTHENTICATION){
        client_quit();
        send_thread->join();
        delete send_thread;
    }
    close_udp_connection(manager->getSocket());
    delete manager;
    exit(EXIT_SUCCESS);

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


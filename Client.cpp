//
// Created by leonardo on 13/05/20.
//


#include "client.h"


using namespace std;

char public_ip[256];
in_addr private_ip;


AudioManager audioManagerIn;
AudioManager audioManagerOut;

ConnectionManager* manager;
thread* send_thread;

string username;

CODE client_status;
bool running;

int calling;
Peer* connected_peer;



int main(int argc, char *argv[])
{
    string server_ip_address;
    if(argc < 2) server_ip_address = "127.0.0.1";
    else server_ip_address = argv[1];

    cout << "Client start. " << endl;
    cout << "Server address: " << server_ip_address << endl;
    cout << "Run as: ./client <server_ip> to set a different ip address." << endl;

    if(LOG) cout << "Signal handler setup.." << endl;
    signal_handler_init();
    if(LOG) cout << "Signal handler ok." << endl;


    cout << "Client/Server connection setupping." << endl;

    struct sockaddr_in server_address{};
    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = inet_addr(server_ip_address.c_str());
    server_address.sin_port = htons(CS_PORT);
    server_address.sin_family = AF_INET;

    int client_server_socket = init_client_udp_connection(server_address);
    if(client_server_socket < 0) handle_error("Error during udp connection init.");

    cout << "Client/server connection ok." << endl;

    manager = new ConnectionManager(client_server_socket, server_address);

    if (ipify(public_ip, sizeof(public_ip))) cout << "[ERROR] No IP address retrived." << endl;
    else cout << "public ip address: " << public_ip << endl;

    // authentication
    int ret;
    do{
        ret = client_authentication();
        if(ret < 0){
            close_udp_connection(client_server_socket);
            delete manager;
            handle_error("Error during client_authentication");
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
        ret = available(client_server_socket, 1, 0);
        if(ret < 0){
            if(errno == EINTR) continue;
        } else if(ret == 0) {
            // timeout occurred
            if(!running) break;
        } else{
            // input available
            receiver();
        }
    }

    send_thread->join();
    delete send_thread;

    delete manager;

    cout << "Closing client/server connection." << endl;
    close_udp_connection(client_server_socket);
    cout << "client/server connection closed." << endl;

    cout << "Client closed." << endl;

    return EXIT_SUCCESS;
}

// Functionality

/*
 * Function signatures starting with "client"
 * are handlers of actions performed by the client
 * interacting with the shell.
 * The following word indicates the action
 * performed by the client
 */

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
    if(LOG) cout << "send: " << out << endl;
    ret = manager->sendMessage(&out);
    if(ret < 0) return ret;

    Message in;
    ret = manager->recvMessage(&in);
    if(ret < 0) return ret;
    if(LOG) cout << "recv: " << in << endl;

    cout << "[Server] " << in.getContent() << endl;

    if(in.getCode() == SUCCESS){
        username = in.getDst();
        return SUCCESS;
    }
    return ERROR;

}

void client_video() {
    client_status = CODE::VIDEO;
}

void client_quit() {

    client_status = CODE::QUIT;
    running = false;

    Message out(QUIT, username, "", "");
    if(LOG) cout << "send: " << out << endl;
    int ret = manager->sendMessage(&out);
    if(ret < 0) perror("client_quit");

}

void client_users() {

    client_status = CODE::USERS;

    Message out(USERS, username, "", "");
    if(LOG) cout << "send: " << out << endl;
    int ret = manager->sendMessage(&out);
    if(ret < 0) perror("client_users");

}

void client_chat() {

    client_status = CODE::CHAT;

    cout << "Type recipient: " << endl;
    string dst;
    while (running){
        int ret = available(0, 1, 0);
        if(ret < 0) {
            if(errno == EINTR) continue;
            else perror("client_chat");
            //error
        } else if(ret == 0 ) {
            //timeout
            if(!running) return;
        } else{
            getline(cin,dst);
            break;
        }
    }

    cout << "Type message: " << endl;
    string message;

    while (running){
        int ret = available(0, 1, 0);
        if(ret < 0) {
            if(errno == EINTR) continue;
            else perror("client_chat");

            //error
        } else if(ret == 0 ) {
            //timeout
            if(!running) return;
        } else{
            getline(cin,message);
            break;
        }
    }

    Message out(CHAT, username, dst, message);
    if(LOG) cout << "send: " << out << endl;

    int ret = manager->sendMessage(&out);
    if(ret < 0) perror("client_chat");


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
    while (running){
        int ret = available(0, 1, 0);
        if(ret < 0) {
            if(errno == EINTR) continue;
            else perror("client_audio_request");
            //error
        } else if(ret == 0 ) {
            //timeout
            if(!running) return;
        } else{
            getline(cin,dst);
            break;
        }
    }

    struct sockaddr_in peer_address{};
    bzero(&peer_address, sizeof(peer_address));
    peer_address.sin_addr.s_addr = htonl(INADDR_ANY);
    peer_address.sin_port = htons(P2P_PORT);
    peer_address.sin_family = AF_INET;

    int socket = init_server_udp_connection(peer_address);
    if(socket < 0){
        perror("client_audio_request");
        return;
    }


    // public ip and local ip
    string address_info = string(public_ip) + " " + string(inet_ntoa(private_ip));

    Message out(AUDIO, username, dst, address_info);
    if(LOG) cout << "send: " << out << endl;
    int ret = manager->sendMessage(&out);
    if(ret < 0) perror("client_audio_request");


    calling = true;
    connected_peer = new Peer();
    connected_peer->set_peer_name(dst);
    connected_peer->set_peer_socket(socket);

    if(LOG) cout << "Client audio request sent." << endl;

    cout << " <refuse> " << endl;

}

/*
 * Accept audio request from peer
 */
void client_audio_accept() {

    client_status = ACCEPT;

    sockaddr_in remote_peer_addr = connected_peer->get_peer_address();

    int peer_socket = init_client_udp_connection(remote_peer_addr);
    if(peer_socket < 0) perror("client_audio_accept");

    connected_peer->set_peer_socket(peer_socket);

    string address_info = string(public_ip) + " " + string(inet_ntoa(private_ip));

    Message out(ACCEPT, username, connected_peer->get_peer_name(), address_info);
    if(LOG) cout << "send: " << out << endl;
    int ret = manager->sendMessage(&out);
    if(ret < 0) perror("client_audio_accept");

    cout << " <ringoff> " << endl;

}

/*
 * Refused audio request from peer
 */
void client_audio_refuse() {

    client_status = REFUSE;

    if(connected_peer != nullptr && connected_peer->is_name_init()){
        Message out(REFUSE, username, connected_peer->get_peer_name(), "");
        if(LOG) cout << "send: " << out << endl;
        int ret = manager->sendMessage(&out);
        if(ret < 0) perror("client_audio_refuse");

    }

    safe_peer_delete();

}

/*
 * Ringoff audio call
 */
void client_audio_ringoff() {

    client_status = RINGOFF;

    if(connected_peer != nullptr && connected_peer->is_name_init()) {
        Message out(RINGOFF, username, connected_peer->get_peer_name(), "");
        if(LOG) cout << "send: " << out << endl;
        int ret = manager->sendMessage(&out);
        if(ret < 0) perror("client_audio_ringoff");

    }

    safe_peer_delete();

}

/*
 * Function signatures starting with "recv"
 * are handlers of incoming messages
 * from Server (Client/Server arch) or
 * Client (Peer to peer arch).
 */

/*
 * Client receive audio request
 */
void recv_audio_request(Message *msg) {

    if(calling){
        // occupato in un altra chiamata
        Message out(REFUSE, username, msg->getSrc(), "Client busy.");
        if(LOG) cout << "send: " << out << endl;
        int ret = manager->sendMessage(&out);
        if(ret < 0) perror("recv_audio_request");
        return;
    }

    client_status = AUDIO;

    const string& content = msg->getContent();
    string public_ip_string = content.substr(0, content.find(' '));
    string local_ip_string = content.substr(content.find(' ') + 1, content.size());

    if(LOG) cout << public_ip_string << " - " << local_ip_string << endl;

    struct sockaddr_in remote_peer_address{};
    bzero(&remote_peer_address, sizeof(remote_peer_address));
    if(public_ip_string == public_ip) remote_peer_address.sin_addr.s_addr = inet_addr(local_ip_string.c_str());
    else remote_peer_address.sin_addr.s_addr = inet_addr(public_ip_string.c_str());
    remote_peer_address.sin_port = htons(P2P_PORT);
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

    int ret;
    // Alert that i'm ready for handshake
    Message out(HANDSHAKE, msg->getDst(), msg->getSrc(), "Start Handshake");
    if(LOG) cout << "send: " << out << endl;
    ret = manager->sendMessage(&out);
    if(ret < 0) perror("recv_audio_accept");


    cout << "P2P Handshake." << endl;

    int socket = connected_peer->get_peer_socket();
    struct sockaddr_in address{};
    socklen_t address_len = sizeof(struct sockaddr_in);

    char buf[MSG_SIZE];
    while (calling){
        ret = available(socket, 1, 0);
        if(ret < 0){
            perror("recv_audio_accept - select");
            return;
        } else if(ret == 0){
            // timeout occurred
            if(LOG) cout << "recv_audio_handshake Timeout occurred." << endl;
            if(!calling) return;
        } else{

            int ret = recvfrom(connected_peer->get_peer_socket(), buf, MSG_SIZE, 0, (struct sockaddr*) &address, (socklen_t*) &address_len);
            if(ret < 0) perror("recv_audio_accept - recvfrom");
            if(LOG) cout << buf << endl;
            break;
        }
    }
    if(calling){
        ret = sendto(connected_peer->get_peer_socket(), "HANDSHAKE CALLER", MSG_SIZE, 0,  (struct sockaddr*) &address, (socklen_t) sizeof(address));
        if(ret < 0) perror("recv_audio_accept - sendto");
    }

    cout << "P2P Handshake end." << endl;

    connected_peer->set_peer_address(address);

    thread caller_thread(call_routine);
    caller_thread.detach();


}

void recv_audio_handshake() {

    cout << "P2P Handshake." << endl;

    int ret, socket = connected_peer->get_peer_socket();

    if(calling){
        ret = sendto(socket, "HANDSHAKE CALLED", MSG_SIZE, 0, nullptr, 0);
        if(ret < 0) perror("recv_audio_handshake - sendto");
    }

    char buf[MSG_SIZE];
    while (calling){
        ret = available(socket, 1, 0);
        if(ret < 0){
            perror("recv_audio_handshake - select");
            return;
        } else if(ret == 0){
            // timeout occurred
            if(LOG) cout << "recv_audio_handshake Timeout occurred." << endl;
            if(!calling) return;
        } else{

            ret = recvfrom(connected_peer->get_peer_socket(), buf, MSG_SIZE, 0, nullptr, nullptr);
            if(ret < 0) perror("recv_audio_handshake - recvfrom");
            if(LOG) cout << buf << endl;
            break;

        }

    }

    cout << "P2P Handshake end." << endl;

    thread called_thread(call_routine);
    called_thread.detach();

}

void recv_audio_ringoff() {
    client_status = RINGOFF;
    safe_peer_delete();
}

void recv_audio_refuse() {
    client_status = REFUSE;
    safe_peer_delete();
}


/*
 * Thread handler that captures audio from the device and sends it to the receiver
 */
void sender_audio_routine(){

    if(LOG) cout << "sender_audio_routine" << endl;

    int socket = connected_peer->get_peer_socket();
    sockaddr_in address = connected_peer->get_peer_address();
    int address_len = sizeof(address);

    while (calling) {

        audioManagerOut.read();

        if(calling){
            int ret = sendto(socket, audioManagerOut.getBuffer(), audioManagerOut.getSize(), 0,  (struct sockaddr*) &address, (socklen_t) address_len);
            if(ret < 0){
                if(errno == ECONNREFUSED) break;
                else perror("sender_audio_routine - sendto");
            } else if(ret != audioManagerOut.getSize()) if(LOG) cout << "short write: wrote %d bytes" << endl;
        }
    }

    if(LOG) cout << "sender_audio_routine end" << endl;
}

/*
 * Thread handler that receive audio from sender and playback it to device
 */
void receiver_audio_routine(){

    if(LOG) cout << "receiver_audio_routine" << endl;

    int socket = connected_peer->get_peer_socket();
    sockaddr_in address = connected_peer->get_peer_address();
    int address_len = sizeof(address);

    while (calling) {

        char *buffer = audioManagerIn.getBuffer();
        int size = audioManagerIn.getSize();
        if(calling){
            int ret = available(socket, 1, 0);
            if(ret < 0) perror("receiver_audio_routine - select");
            else if(ret == 0) {
                // timeout occurred
                if(LOG) cout << "Timeout receiver_audio_routine." << endl;
                if(!calling) break;
            } else{
                // available
                ret = recvfrom(socket, buffer, size, 0, (struct sockaddr*) &address, (socklen_t*) &address_len);
                if(ret < 0){
                    if(errno == ECONNREFUSED) break;
                    else perror("receiver_audio_routine - recvfrom");
                }
            }

        }

        audioManagerIn.write();

    }

    if(LOG) cout << "receiver_audio_routine end." << endl;


}

/*
 * Thread handler that init ALSA library. (Audio Manager object)
 * It starts sender_audio_routine and receiver_audio_routine
 * and closes audio interaction when comunication is interrupted (Audio Manager destroy).
 */
void call_routine(){

    if(LOG) cout << "Call routine start." << endl;

    if(LOG) cout << "Audio Manager init.." << endl;
    audioManagerIn.init_playback();
    audioManagerOut.init_capture();
    if(LOG) cout << "Audio Manager ok." << endl;

    if(LOG) cout << "sender thread start." << endl;
    thread sender_audio(sender_audio_routine);
    if(LOG) cout << "receiver thread start." << endl;
    thread receiver_audio(receiver_audio_routine);


    if(LOG) cout << "receiver thread join." << endl;
    receiver_audio.join();
    if(LOG) cout << "sender thread join." << endl;
    sender_audio.join();

    if(LOG) cout << "Audio Manager destroy.." << endl;
    audioManagerIn.destroy_playback();
    audioManagerOut.destroy_capture();
    if(LOG) cout << "Audio Manager destroyed." << endl;

    if(LOG) cout << "Call routine end." << endl;

}

/*
 * Handles reading from the shell.
 * Starts "client_ .." functions
 */
void sender(){
    while (running){

        string line;
        int ret = available(0, 1, 0);
        if(ret < 0) {
            if(errno == EINTR) continue;
            else perror("sender - select");
        } else if(ret == 0 ) {
            //timeout
            if(!running) return;
            else continue;
        } else{
            getline(cin,line);
        }


//        if(line == "chat" ) client_chat();
//        else if(line == "users" ) client_users();
//        else if(line == "quit" ) client_quit();
//        else if(line == "video" ) client_video();
//        else if(line == "audio" ) client_audio_request();
//        else if(line == "accept" ) client_audio_accept(); // client accetta chiamata di un client
//        else if(line == "refuse" ) client_audio_refuse(); // client rifiuta chiamata di un client
//        else if(line == "ringoff" ) client_audio_ringoff(); // client attacca mentre e` in chimata con un client
//        else cout << "Unknown status." << endl;



        if(!calling){
            if(line == "chat" ) client_chat();
            else if(line == "users" ) client_users();
            else if(line == "quit" ) client_quit();
            else if(line == "video" ) client_video();
            else if(line == "audio" ) client_audio_request(); // client vuole chiamare un client
            else{
                cout << "Unknown command." << endl;
                print_info_message();
            }

        }else{
            if(client_status == AUDIO){
                if(connected_peer->is_address_init()){
                    if(line == "accept" ) client_audio_accept(); // client accetta chiamata di un client
                    else if(line == "refuse" ) client_audio_refuse(); // client rifiuta chiamata di un client
                    else cout << "Incoming call.. <accept> or <refuse> ?" << endl;
                } else{
                    if(line == "refuse" ) client_audio_refuse(); // client rifiuta chiamata di un client
                    else cout << "Outgoing call.. <refuse> ?" << endl;
                }
            } else if(client_status == ACCEPT){
                if(line == "ringoff" ) client_audio_ringoff(); // client attacca mentre e` in chimata con un client
                else cout << "Calling in progress.. <ringoff> ?" << endl;
            } else cout << "Unknown status." << endl;

        }

    }
}

/*
 * Manages messages arriving from the server
 */
void receiver() {

    Message in;
    int ret = manager->recvMessage(&in);
    if(ret < 0) perror("receiver - recvMessage");
    if(LOG) cout << "recv: " << in << endl;

    switch (in.getCode()) {
        case CHAT:
            cout << "[" << in.getSrc() << "] " << in.getContent() << endl;
            break;
        case QUIT:
            cout << "[Server] Service interrupted." << endl;
            safe_peer_delete();
            running = false;
            break;
        case USERS:
            cout << "[Server] Logged users:" << endl << in.getContent() << endl;
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
            if(LOG) cout << "Receive audio request." << endl;
            cout << "Incoming call from " << in.getSrc() << ".\n <accept> <refuse> " << endl;
            recv_audio_request(&in);
            break;
        case ACCEPT:
            // il server mi comunica che il client ha accettato la chiamata
            if(LOG) cout << "Receive audio accept." << endl;
            cout << in.getSrc() << " accepted call." << endl;
            recv_audio_accept(&in);
            break;
        case REFUSE:
            // il server mi comunica che il client ha rifiutato la chiamata
            if(LOG) cout << "Receive audio refuse." << endl;
            cout << in.getSrc() << " refused call." << endl;
            recv_audio_refuse();
            break;
        case RINGOFF:
            if(LOG) cout << "Receive audio ringoff." << endl;
            cout << in.getSrc() << " closed call." << endl;
            recv_audio_ringoff();
            break;
        case HANDSHAKE:
            if(LOG) cout << "Receive handshake message." << endl;
            recv_audio_handshake();
            break;
        default:
            break;
    }

}

// Initialization

int init_server_udp_connection(sockaddr_in socket_address) {

    if(LOG) cout << "UDP protocol setupping..." << endl;

    int ret, socket_udp;
    if(LOG) cout << " - socket.." << endl;
    socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0){
        perror("init_server_udp_connection - socket");
        return socket_udp;
    }
    if(LOG) cout << " - socket succeeded." << endl;

    if(LOG) cout << " - bind.." << endl;
    ret = bind(socket_udp, (struct sockaddr *) &socket_address, sizeof(socket_address));
    if(ret < 0){
        perror("init_server_udp_connection - bind");
        return ret;
    }
    if(LOG) cout << " - bind succeeded." << endl;

    if(LOG) cout << "Udp protocol configured." << endl;

    return socket_udp;
}

int init_client_udp_connection(sockaddr_in socket_address) {

    if(LOG) cout << "UDP protocol setupping..." << endl;

    if(LOG) cout << " -  socket.." << endl;
    int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0) {
        perror("init_client_udp_connection - socket");
        return  socket_udp;
    }
    if(LOG) cout << " - socket succeeded." << endl;

    if(LOG) cout << " - connect.." << endl;
    int ret = connect(socket_udp, (struct sockaddr *)&socket_address, sizeof(socket_address));
    if(ret < 0) {
        perror("init_client_udp_connection - connect");
        return ret;
    }
    if(LOG) cout << " - connect succeeded.." << endl;

    if(LOG) cout << " - retrive address info.." << endl;
    struct sockaddr_in client_address{};
    socklen_t len_client = sizeof(client_address);
    ret = getsockname(socket_udp, (struct sockaddr*)&client_address, &len_client);
    if(ret < 0) {
        perror("init_client_udp_connection - getsockname");
        return ret;
    }

    private_ip = client_address.sin_addr;
    if(LOG) cout << " - local ip address: " << inet_ntoa(private_ip) << endl;
    if(LOG) cout << " - address info ok." << endl;

    if(LOG) cout << "Udp protocol configured." << endl;
    return socket_udp;
}

int close_udp_connection(int socket) {

    // close the descriptor
    int ret = close(socket);
    if(ret < 0) handle_error("close_udp_connection - close");
    return ret;

}

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    // signal(SIGINT, sigintHandler);
    if(LOG) cout << endl <<"Catch Ctrl+C - System status: " << client_status << endl;
    switch (client_status) {
        case CODE::AUTHENTICATION:
            if(LOG) cout << "Authentication stopped." << endl;
            break;
        case CODE::AUDIO:
            if(LOG) cout << "Audio request stopped." << endl;
            client_audio_refuse();
            break;
        case CODE::ACCEPT:
            if(LOG) cout << "Audio accept stopped." << endl;
            client_audio_ringoff();
            cout << "Call interrupted, now you can stop service." << endl;
            return;
        case CODE::RINGOFF:
            if(LOG) cout << "Audio Ringoff stopped." << endl;
//            client_audio_ringoff();
            break;
        default:
            if(LOG) cout << "Service Stopped." << endl;
            break;
    }

    if(client_status != AUTHENTICATION){
        client_quit();
        send_thread->join();
        delete send_thread;
    }
    int ret = close_udp_connection(manager->getSocket());
    if(ret < 0) perror("sigintHandler - close_udp_connection");
    delete manager;
    exit(EXIT_SUCCESS);

}

void signal_handler_init() {
    /* signal handler */
    struct sigaction sigIntHandler{};
    sigIntHandler.sa_handler = sigintHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0; //SA_RESTART
    sigaction(SIGINT, &sigIntHandler, nullptr);
    /* end signal handler */

}



/*
 * Wrapper for select operation
 * allow not blocking operation
 * ( as recvfrom, getline, etc..)
 */
int available(int fd, int sec, int usec){
    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(fd, &set); // add server descriptor on set
    // set timeout
    timeval timeout{};
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    int ret = select( fd + 1, &set, nullptr, nullptr, &timeout);
    if(ret < 0) perror("available - select");
    return ret;
}

void safe_peer_delete() {

    calling = false;
    if(connected_peer != nullptr){
        if(connected_peer->is_socket_init())
            close_udp_connection(connected_peer->get_peer_socket());
        delete connected_peer;
        connected_peer = nullptr;
    } else if(LOG) cout << "safe_peer_delete - connected_peer == nullptr" << endl;
}

void print_info_message() {
    cout << "Type <users> to see users online. " << endl
         << "Type <chat> to send message. " << endl
         << "Type <audio> to call. " << endl
         << "Type <quit> to disconnect." << endl;
}

void print_socket_address(sockaddr_in *pIn) {

    cout << "address info: " << inet_ntoa(pIn->sin_addr)
         << " - " << to_string(htonl(pIn->sin_addr.s_addr))
         << " - " << to_string(ntohs(pIn->sin_port)) << endl;

}

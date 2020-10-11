//
// Created by leonardo on 13/05/20.
//


#include "Client.h"


using namespace std;

char public_ip[256];
in_addr private_ip;


AudioManager audioManagerIn;
AudioManager audioManagerOut;

ConnectionManager connectionManager;

//ConnectionManager* manager;
thread* send_thread;

string username;

CODE client_status;
bool running;

//int calling;

PeerConnectionManager peerConnectionManager;
//Peer* connected_peer;



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

    connectionManager.setAddress(server_address);
    int ret = connectionManager.initClientConnection();
    if(ret < 0) handle_error("initClientConnection");


    // int client_server_socket = init_client_udp_connection(server_address);
    // if(client_server_socket < 0) handle_error("Error during udp connection init.");

    cout << "Client/server connection ok." << endl;

    // manager = new ConnectionManager(client_server_socket, server_address);

    if (ipify(public_ip, sizeof(public_ip))) cout << "[ERROR] No IP address retrived." << endl;
    else cout << "public ip address: " << public_ip << endl;

    // authentication
    // int ret;
    do{
        ret = client_authentication();
        if(ret < 0){
            connectionManager.closeConnection();
//            close_udp_connection(client_server_socket);
//            delete manager;
            handle_error("Error during client_authentication");
        }
    } while (ret != SUCCESS);

    client_status = SUCCESS;

    cout << "Welcome " << username << endl;
    print_info_message();

    // Handles writing to the shell
    send_thread = new thread(sender);


    // calling = false;
    running = true;
    while (running){
        ret = connectionManager.available(1,0);
        // ret = available(client_server_socket, 1, 0);
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

//    delete manager;

    cout << "Closing client/server connection." << endl;
    connectionManager.closeConnection();
//    close_udp_connection(client_server_socket);
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
    // ret = manager->sendMessage(&out);
    ret = connectionManager.sendMessage(&out);
    if(ret < 0) return ret;

    Message in;
    // ret = manager->recvMessage(&in);
    ret = connectionManager.recvMessage(&in);
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
    // int ret = manager->sendMessage(&out);
    int ret = connectionManager.sendMessage(&out);
    if(ret < 0) perror("client_quit");

}

void client_users() {

    client_status = CODE::USERS;

    Message out(USERS, username, "", "");
    if(LOG) cout << "send: " << out << endl;
    // int ret = manager->sendMessage(&out);
    int ret = connectionManager.sendMessage(&out);
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

    // int ret = manager->sendMessage(&out);
    int ret = connectionManager.sendMessage(&out);
    if(ret < 0) perror("client_chat");


}

/*
 * Request audio with a peer
 */
void client_audio_request() {

    client_status = CODE::AUDIO;

    int ret;

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

    ret = peerConnectionManager.initServerConnection(peer_address);
    if(ret < 0) {
        perror("client_audio_request");
        return;
    }

    peerConnectionManager.setCalling(true);
    peerConnectionManager.setPeerName(dst);
//    peerConnectionManager.setSocket(socket);

//    int socket = init_server_udp_connection(peer_address);
//    if(socket < 0){
//        perror("client_audio_request");
//        return;
//    }


    // public ip and local ip
    string address_info = string(public_ip) + " " + string(inet_ntoa(private_ip));

    Message out(AUDIO, username, dst, address_info);
    if(LOG) cout << "send: " << out << endl;
    // int ret = manager->sendMessage(&out);
    ret = connectionManager.sendMessage(&out);
    if(ret < 0) perror("client_audio_request");


//    calling = true;
//    connected_peer = new Peer();
//    connected_peer->set_peer_name(dst);
//    connected_peer->set_peer_socket(socket);




    if(LOG) cout << "Client audio request sent." << endl;

    cout << " <refuse> " << endl;

}

/*
 * Accept audio request from peer
 */
void client_audio_accept() {

    client_status = ACCEPT;

    int ret;

    // sockaddr_in remote_peer_addr = connected_peer->get_peer_address();
    // sockaddr_in remote_peer_addr = peerConnectionManager.getAddress();

    ret = peerConnectionManager.initClientConnection();
    if(ret < 0) perror("client_audio_accept");

    // int peer_socket = init_client_udp_connection(remote_peer_addr);
    // if(peer_socket < 0) perror("client_audio_accept");

    // connected_peer->set_peer_socket(peer_socket);
    // peerConnectionManager.setSocket(peer_socket);

    string address_info = string(public_ip) + " " + string(inet_ntoa(private_ip));

    string dst = peerConnectionManager.getPeerName();
    Message out(ACCEPT, username, dst, address_info);
    // Message out(ACCEPT, username, connected_peer->get_peer_name(), address_info);
    if(LOG) cout << "send: " << out << endl;
    // int ret = manager->sendMessage(&out);
    ret = connectionManager.sendMessage(&out);
    if(ret < 0) perror("client_audio_accept");

    cout << " <ringoff> " << endl;

}

/*
 * Refused audio request from peer
 */
void client_audio_refuse() {

    client_status = REFUSE;
    int ret;

    if(peerConnectionManager.isInitPeerName()){
        string peer_name = peerConnectionManager.getPeerName();
        Message out(REFUSE, username, peer_name, "");
        if(LOG) cout << "send: " << out << endl;
        // int ret = manager->sendMessage(&out);
        ret = connectionManager.sendMessage(&out);
        if(ret < 0) perror("client_audio_refuse");

    }
//    if(connected_peer != nullptr && connected_peer->is_name_init()){
//        Message out(REFUSE, username, connected_peer->get_peer_name(), "");
//        if(LOG) cout << "send: " << out << endl;
//        int ret = manager->sendMessage(&out);
//        if(ret < 0) perror("client_audio_refuse");
//
//    }

    safe_peer_delete();

}

/*
 * Ringoff audio call
 */
void client_audio_ringoff() {

    client_status = RINGOFF;

    int ret;

    if(peerConnectionManager.isInitPeerName()) {
        Message out(RINGOFF, username, peerConnectionManager.getPeerName(), "Ringoff");
        if(LOG) cout << "send: " << out << endl;
        // int ret = manager->sendMessage(&out);
        ret = connectionManager.sendMessage(&out);
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

    int ret;
    if(peerConnectionManager.isCalling()){
        // occupato in un altra chiamata
        Message out(REFUSE, username, msg->getSrc(), "Client busy.");
        if(LOG) cout << "send: " << out << endl;
        ret = connectionManager.sendMessage(&out);
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

    string src = msg->getSrc();
    peerConnectionManager.setCalling(true);
    peerConnectionManager.setPeerName(src);
    peerConnectionManager.setAddress(remote_peer_address);


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
    // ret = manager->sendMessage(&out);
    ret = connectionManager.sendMessage(&out);
    if(ret < 0) perror("recv_audio_accept");


    cout << "P2P Handshake." << endl;

    ret = peerConnectionManager.server_handshake();
    if(ret < 0){
        perror("server_peer_handshake");
        return;
    }

    cout << "P2P Handshake end." << endl;

    thread caller_thread(call_routine);
    caller_thread.detach();


}

void recv_audio_handshake() {

    cout << "P2P Handshake." << endl;

    int ret;
    ret = peerConnectionManager.client_handshake();
    if(ret < 0){
        perror("client_handshake");
        return;
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
    
    while (peerConnectionManager.isCalling()) {

        audioManagerOut.read();

        char *buffer = audioManagerOut.getBuffer();
        int size = audioManagerOut.getSize();

        if(peerConnectionManager.isCalling()){
            int ret = peerConnectionManager.sendMessage(buffer, size);
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

    int ret;

    while (peerConnectionManager.isCalling()) {

        char *buffer = audioManagerIn.getBuffer();
        int size = audioManagerIn.getSize();
        if(peerConnectionManager.isCalling()){

            ret = available(peerConnectionManager.getSocket(), 1, 0);
            if(ret < 0) perror("receiver_audio_routine - select");
            else if(ret == 0) {
                // timeout occurred
                if(LOG) cout << "Timeout receiver_audio_routine." << endl;
                if(!peerConnectionManager.isCalling()) break;
            } else{
                // available
                ret = peerConnectionManager.recvMessage(buffer, size);
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



        if(!peerConnectionManager.isCalling()){
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
                if(peerConnectionManager.isInitAddress()){
                // if(connected_peer->is_address_init()){
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

    int ret;
    Message in;
    // int ret = manager->recvMessage(&in);
    connectionManager.recvMessage(&in);
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


/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    int ret;
    // signal(SIGINT, sigintHandler);
    if(LOG) cout << endl <<"Catch Ctrl+C - System status: " << client_status << endl;
    switch (client_status) {
        case CODE::AUTHENTICATION:
            if(LOG) cout << "Authentication stopped." << endl;
            break;
        case CODE::AUDIO:
            if(LOG) cout << "Audio request stopped." << endl;
            client_audio_refuse();
//            break;
            return;
        case CODE::ACCEPT:
            if(LOG) cout << "Audio accept stopped." << endl;
            client_audio_ringoff();
//            cout << "Call interrupted, now you can stop service." << endl;
//            return;
            return;
        case CODE::RINGOFF:
            if(LOG) cout << "Audio Ringoff stopped." << endl;
//            client_audio_ringoff();
//            break;
            return;
        default:

            if(LOG) cout << "Service Stopped." << endl;
            client_quit();
            send_thread->join();
            delete send_thread;

            break;
    }

//    if(client_status != AUTHENTICATION){
//        client_quit();
//        send_thread->join();
//        delete send_thread;
//    }
    ret = connectionManager.closeConnection();
    // int ret = close_udp_connection(manager->getSocket());
    if(ret < 0) perror("sigintHandler - closeConnection");
    // delete manager;
    exit(EXIT_SUCCESS);

}


void safe_peer_delete() {
    int ret;
    peerConnectionManager.setCalling(false);
    if(peerConnectionManager.isInitSocket()){
        cout << "socket init." << endl;
        ret = connectionManager.closeConnection();
//        int ret = close_udp_connection(peerConnectionManager.getSocket());
        if(ret < 0) perror("close_udp_connection");
    }
    peerConnectionManager = PeerConnectionManager();

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

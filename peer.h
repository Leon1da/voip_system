//
// Created by leonardo on 07/10/20.
//

#ifndef CHAT_UDP_PEER_H
#define CHAT_UDP_PEER_H

#endif //CHAT_UDP_PEER_H


#include <iostream>
#include <netinet/in.h>

using namespace std;

class Peer{
private:
    string peer_name;
    int peer_socket;
    sockaddr_in peer_address;

    bool socket_init;
    bool address_init;
    bool name_init;

public:
    Peer(){
        socket_init = false;
        address_init = false;
        name_init = false;
    }


    bool is_socket_init() const {
        return socket_init;
    }

    bool is_address_init() const {
        return address_init;
    }

    bool is_name_init() const {
        return name_init;
    }

    const sockaddr_in &get_peer_address() const {
        return peer_address;
    }

    const string &get_peer_name() const {
        return peer_name;
    }

    void set_peer_name(const string &peerName) {
        peer_name = peerName;
        name_init = true;
    }

    int get_peer_socket() const {
        return peer_socket;
    }

    void set_peer_socket(int peerSocket) {
        peer_socket = peerSocket;
        socket_init = true;
    }

    void set_peer_address(const sockaddr_in &peerAddress) {
        peer_address = peerAddress;
        address_init = true;
    }


    string address;
    string port;
    string public_ip;
    string local_ip;
};


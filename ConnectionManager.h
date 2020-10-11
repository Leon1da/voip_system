//
// Created by leonardo on 27/09/20.
//

#ifndef CHAT_UDP_CONNECTIONMANAGER_H
#define CHAT_UDP_CONNECTIONMANAGER_H

#include "Common.h"
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <ostream>
#include <cstdio>
#include <iostream>

#include <zconf.h>

// MSG

#define MSG_H_CODE_SIZE 8
#define MSG_H_DST_SIZE 24
#define MSG_H_SRC_SIZE 24

#define MSG_HEADER_SIZE MSG_H_CODE_SIZE + MSG_H_DST_SIZE + MSG_H_SRC_SIZE

#define MSG_CONTENT_SIZE 512

#define MSG_SIZE MSG_HEADER_SIZE + MSG_CONTENT_SIZE

/*
 *
 * [              MSG              ]
 * [       HEADER       ][ CONTENT ]
 * [ CODE ][ SRC ][ DST ][ CONTENT ]
 *
*/

using namespace std;


enum CODE{
    ERROR = 1001,
    INFO = 1003 ,
    SUCCESS = 1004,
    AUTHENTICATION = 1005,
    CHAT = 1006,
    USERS = 1007,
    QUIT = 1008,
    VIDEO = 1009,
    AUDIO = 1011,
    ACCEPT = 1012,
    REFUSE = 1013,
    RINGOFF = 1015,
    HANDSHAKE = 1016
};



class Message{

private:
    CODE code;
    string src;
    string dst;
    string content;

public:

    Message();

    Message(CODE code, const string &src, const string &dst, const string &content);

    friend ostream &operator<<(ostream &os, const Message &message);

    CODE getCode() const;

    void setCode(CODE code);

    const string &getSrc() const;

    void setSrc(const string &src);

    const string &getDst() const;

    void setDst(const string &dst);

    const string &getContent() const;

    void setContent(const string &content);
};


class ConnectionManager {

private:

    bool init_socket = false;
    bool init_address = false;

protected:
    int m_socket;
    sockaddr_in m_address;

public:
    ConnectionManager(int socket, const sockaddr_in &address);

    ConnectionManager(int socket);

    ConnectionManager();

    int getSocket() const;

    void setSocket(int socket);

    const sockaddr_in &getAddress() const;

    void setAddress(const sockaddr_in &address);

    bool isInitSocket() const;

    bool isInitAddress() const;

    int sendMessage(Message* message);

    int sendMessage(Message* p_message, sockaddr_in* p_adddress);

    int recvMessage(Message* message);

    int recvMessage(Message* message, sockaddr_in* src_address);

    int initServerConnection(sockaddr_in in);

    int initClientConnection();

    int closeConnection();

    int getAddressInfo(sockaddr_in *client_address);

    int available(int sec, int usec);
};

class PeerConnectionManager : public ConnectionManager{

private:

    bool m_calling = false;
    string m_peer_name;

    bool init_peer_name = false;

public:

    bool isCalling() const;

    void setCalling(bool calling);

    bool isInitPeerName() const;

    PeerConnectionManager();

    const string &getPeerName() const;

    void setPeerName(const string &peerName);

    int sendMessage(char* buf, int size);

    int recvMessage(char* buf, int size);

    int recvMessage(char *buf, int size, sockaddr_in *address, socklen_t* address_len);

    int client_handshake();

    int server_handshake();
};



#endif //CHAT_UDP_CONNECTIONMANAGER_H

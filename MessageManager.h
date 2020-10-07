//
// Created by leonardo on 27/09/20.
//

#ifndef CHAT_UDP_MESSAGEMANAGER_H
#define CHAT_UDP_MESSAGEMANAGER_H


#include <netinet/in.h>
#include <cstring>
#include <string>
#include <ostream>

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
    RINGOFF = 1015
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

class MessageManager {

private:
    int socket;
    sockaddr_in address;

public:
    MessageManager(int socket, const sockaddr_in &address);

    MessageManager(int socket);

    int getSocket() const;

    void setSocket(int socket);

    const sockaddr_in &getAddress() const;

    void setAddress(const sockaddr_in &address);

    int sendMessage(Message* message);

    int sendMessage(Message* p_message, sockaddr_in* p_adddress);

    int recvMessage(Message* message);

    int recvMessage(Message* message, sockaddr_in* src_address);
};




#endif //CHAT_UDP_MESSAGEMANAGER_H

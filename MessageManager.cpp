//
// Created by leonardo on 27/09/20.
//

#include "config.h"
#include "MessageManager.h"


Message::Message(CODE code, const string &src, const string &dst, const string &content) : code(code), src(src),
                                                                                           dst(dst), content(content) {}

CODE Message::getCode() const {
    return code;
}

void Message::setCode(CODE code) {
    Message::code = code;
}

const string &Message::getSrc() const {
    return src;
}

void Message::setSrc(const string &src) {
    Message::src = src;
}

const string &Message::getDst() const {
    return dst;
}

void Message::setDst(const string &dst) {
    Message::dst = dst;
}

const string &Message::getContent() const {
    return content;
}

void Message::setContent(const string &content) {
    Message::content = content;
}

ostream &operator<<(ostream &os, const Message &message) {
    os << "[ " << message.code << " ][ " << message.src << " ][ " << message.dst << " ][ " << message.content <<  " ]";
    return os;
}

Message::Message() {}

MessageManager::MessageManager(int socket) : socket(socket) {}

MessageManager::MessageManager(int socket, const sockaddr_in &address) : socket(socket), address(address) {}

int MessageManager::getSocket() const {
    return socket;
}

void MessageManager::setSocket(int socket) {
    MessageManager::socket = socket;
}

const sockaddr_in &MessageManager::getAddress() const {
    return address;
}

void MessageManager::setAddress(const sockaddr_in &address) {
    MessageManager::address = address;
}

int MessageManager::sendMessage(Message *message) {

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(message->getCode()).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, message->getSrc().c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, message->getDst().c_str());
    strcpy( msg + MSG_HEADER_SIZE, message->getContent().c_str());
    
    int ret = sendto(socket, msg, MSG_SIZE, 0, (struct sockaddr*) &address, sizeof(address));
    return ret;

}

int MessageManager::sendMessage(Message *p_message, sockaddr_in* p_adddress) {

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(p_message->getCode()).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, p_message->getSrc().c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, p_message->getDst().c_str());
    strcpy( msg + MSG_HEADER_SIZE, p_message->getContent().c_str());

    int ret = sendto(socket, msg, MSG_SIZE, 0, (struct sockaddr*) p_adddress, sizeof(*p_adddress));
    return ret;
}



int MessageManager::recvMessage(Message *message) {
    
    char msg[MSG_SIZE];

    struct sockaddr_in address{};
    socklen_t len = sizeof(address);

    int ret = recvfrom(socket, msg, MSG_SIZE, 0, (struct sockaddr *) &address, &len);
    if(ret < 0) return ret;

    CODE code = (CODE) atoi(msg);
    string src_ = msg + MSG_H_CODE_SIZE;
    string dst_ = msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE;
    string content_ = msg + MSG_HEADER_SIZE;


    message->setCode(code);
    message->setSrc(src_);
    message->setDst(dst_);
    message->setContent(content_);

    return ret;
}

int MessageManager::recvMessage(Message *message, sockaddr_in *src_address) {

    char msg[MSG_SIZE];
    socklen_t len = sizeof(*src_address);

    int ret = recvfrom(socket, msg, MSG_SIZE, 0, (struct sockaddr *) src_address, &len);
    if(ret < 0) return ret;

    CODE code = (CODE) atoi(msg);
    string src_ = msg + MSG_H_CODE_SIZE;
    string dst_ = msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE;
    string content_ = msg + MSG_HEADER_SIZE;


    message->setCode(code);
    message->setSrc(src_);
    message->setDst(dst_);
    message->setContent(content_);

    return ret;

}


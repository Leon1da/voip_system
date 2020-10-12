//
// Created by leonardo on 27/09/20.
//

#include "ConnectionManager.h"


using namespace std;


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

ConnectionManager::ConnectionManager() {}

int ConnectionManager::getSocket() const {
    return m_socket;
}

void ConnectionManager::setSocket(int socket) {
    ConnectionManager::m_socket = socket;
    ConnectionManager::init_socket = true;
}

const sockaddr_in &ConnectionManager::getAddress() const {
    return m_address;
}

void ConnectionManager::setAddress(const sockaddr_in &address) {
    ConnectionManager::m_address = address;
    ConnectionManager::init_address = true;
}

int ConnectionManager::sendMessage(Message *message) {

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(message->getCode()).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, message->getSrc().c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, message->getDst().c_str());
    strcpy( msg + MSG_HEADER_SIZE, message->getContent().c_str());
    
    int ret = sendto(m_socket, msg, MSG_SIZE, 0, (struct sockaddr*) &m_address, sizeof(m_address));
    return ret;

}

int ConnectionManager::sendMessage(Message *p_message, sockaddr_in* p_adddress) {

    char msg[MSG_SIZE];
    memset(msg, 0, MSG_SIZE);
    strcpy(msg, to_string(p_message->getCode()).c_str());
    strcpy(msg + MSG_H_CODE_SIZE, p_message->getSrc().c_str());
    strcpy(msg + MSG_H_CODE_SIZE + MSG_H_SRC_SIZE, p_message->getDst().c_str());
    strcpy( msg + MSG_HEADER_SIZE, p_message->getContent().c_str());

    int ret = sendto(m_socket, msg, MSG_SIZE, 0, (struct sockaddr*) p_adddress, sizeof(*p_adddress));
    return ret;
}

int ConnectionManager::recvMessage(Message *message) {
    
    char msg[MSG_SIZE];

    struct sockaddr_in address{};
    socklen_t len = sizeof(address);

    int ret = recvfrom(m_socket, msg, MSG_SIZE, 0, (struct sockaddr *) &address, &len);
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

int ConnectionManager::recvMessage(Message *message, sockaddr_in *src_address) {

    char msg[MSG_SIZE];
    socklen_t len = sizeof(*src_address);

    int ret = recvfrom(m_socket, msg, MSG_SIZE, 0, (struct sockaddr *) src_address, &len);
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

bool ConnectionManager::isInitSocket() const {
    return init_socket;
}

bool ConnectionManager::isInitAddress() const {
    return init_address;
}

int ConnectionManager::initServerConnection(sockaddr_in in) {

    if(LOG) cout << "UDP protocol setupping..." << endl;

    int ret;
    if(LOG) cout << " - socket.." << endl;
    int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0){
        perror("socket");
        return socket_udp;
    }

    // setSocket(socket_udp); // m_socket = socket_udp

    if(LOG) cout << " - socket succeeded." << endl;

    if(LOG) cout << " - bind.." << endl;
    ret = bind(socket_udp, (struct sockaddr *) &in, sizeof(in));
    if(ret < 0){
        perror("bind");
        return ret;
    }
    if(LOG) cout << " - bind succeeded." << endl;

    if(LOG) cout << "Udp protocol configured." << endl;

    return socket_udp;
}

int ConnectionManager::initClientConnection() {

    if(LOG) cout << "UDP protocol setupping..." << endl;

    if(LOG) cout << " -  socket.." << endl;
    int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0) {
        perror("socket");
        return  socket_udp;
    }
    if(LOG) cout << " - socket succeeded." << endl;

    if(LOG) cout << " - connect.." << endl;
    int ret = connect(socket_udp, (struct sockaddr *)&m_address, sizeof(m_address));
    if(ret < 0) {
        perror("connect");
        return ret;
    }
    if(LOG) cout << " - connect succeeded.." << endl;

    if(LOG) cout << "Udp protocol configured." << endl;

    return socket_udp;
}

int ConnectionManager::getSockName(sockaddr_in* address) {

    if(LOG) cout << " - retrive address info.." << endl;

    socklen_t len_client = sizeof(*address);
    int ret = getsockname(m_socket, (struct sockaddr*)address, &len_client);
    if(ret < 0) {
        perror("getsockname");
        return ret;
    }

    if(LOG) cout << " - retrive address info ok." << endl;

    return EXIT_SUCCESS;

}

int ConnectionManager::getPeerName(sockaddr_in *address) {
    if(LOG) cout << " - retrive address info.." << endl;

    socklen_t len_client = sizeof(*address);
    int ret = getpeername(m_socket, (struct sockaddr*)address, &len_client);
    if(ret < 0) {
        perror("getsockname");
        return ret;
    }

    if(LOG) cout << " - retrive address info ok." << endl;

    return EXIT_SUCCESS;
}

int ConnectionManager::closeConnection() {

    // close the descriptor
    int ret = close(m_socket);
    if(ret < 0) perror("close");
    return ret;
}

int ConnectionManager::available(int sec, int usec) {
    return ::available(m_socket, sec, usec);
}

const string &PeerConnectionManager::getPeerName() const {
    return m_peer_name;
}

void PeerConnectionManager::setPeerName(const string &peerName) {
    m_peer_name = peerName;
    init_peer_name = true;
}

PeerConnectionManager::PeerConnectionManager() {
    m_calling = false;
}

bool PeerConnectionManager::isInitPeerName() const {
    return init_peer_name;
}

int PeerConnectionManager::sendMessage(char *buf, int size) {
    int ret;
    ret = sendto(m_socket, buf, size, 0, (struct sockaddr*) &m_address, (socklen_t) sizeof(m_address));
    return ret;
}

int PeerConnectionManager::recvMessage(char *buf, int size, sockaddr_in* address, socklen_t* address_len) {
    int ret;
    ret = recvfrom(m_socket, buf, size, 0, (struct sockaddr*) address, address_len);
    return ret;
}

int PeerConnectionManager::recvMessage(char *buf, int size) {
    return recvMessage(buf, size, nullptr, nullptr);
}

bool PeerConnectionManager::isCalling() const {
    return m_calling;
}

void PeerConnectionManager::setCalling(bool calling) {
    PeerConnectionManager::m_calling = calling;
}

int PeerConnectionManager::client_handshake() {
    int ret;

    if(m_calling){

        ret = sendto(m_socket, "HANDSHAKE CALLED", MSG_SIZE, 0, nullptr, 0);
        if(ret < 0) perror("sendto");

    }

    char buf[MSG_SIZE];
    memset(buf, 0, MSG_SIZE);
    while (m_calling){
        ret = ::available(m_socket, 1, 0);
        if(ret < 0){
            perror("select");
            return ret;
        } else if(ret == 0){
            // timeout occurred
            if(LOG) cout << "client_handshake Timeout occurred." << endl;
            if(!m_calling) return ret;
        } else{

            ret = recvfrom(m_socket, buf, MSG_SIZE, 0, nullptr, nullptr);
            if(ret < 0){
                perror("recvfrom");
                return ret;
            }
            if(LOG) cout << buf << endl;
            break;

        }

    }
    return 0;
}

int PeerConnectionManager::server_handshake() {

    int ret;

    char buf[MSG_SIZE];
    memset(buf, 0, MSG_SIZE);
    while (m_calling){
        ret = ::available(m_socket, 1, 0);
        if(ret < 0){
            perror("select");
            return ret;
        } else if(ret == 0){
            // timeout occurred
            if(LOG) cout << "server_peer_handshake Timeout occurred." << endl;
            if(!m_calling) return 0;
        } else{

            struct sockaddr_in addr{};
            socklen_t addr_len = sizeof(struct sockaddr_in);
            int ret = recvfrom(m_socket, buf, MSG_SIZE, 0, (struct sockaddr*) &addr, (socklen_t*) &addr_len);
            if(ret < 0){
                perror("recvfrom");
                return ret;
            }
            if(LOG) cout << buf << endl;
            m_address = addr;
            break;
        }
    }

    if(m_calling){

        ret = sendto(m_socket, "HANDSHAKE CALLER", MSG_SIZE, 0, (struct sockaddr*) &m_address, (socklen_t) sizeof(m_address));
        if(ret < 0) perror("sendto");
    }


    return EXIT_SUCCESS;
}


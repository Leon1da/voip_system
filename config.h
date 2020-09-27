//
// Created by leonardo on 12/09/20.
//

#ifndef CHAT_UDP_CONFIG_H
#define CHAT_UDP_CONFIG_H

#endif //CHAT_UDP_CONFIG_H

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_AUTH_PORT 5000
#define SERVER_CHAT_PORT 6000
#define P2P_AUDIO_PORT 4000

#define MAX_CONN_QUEUE 5

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

#define LOG 1

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

class User{

public:

    User(){};

    User(int id, string username, string password, sockaddr_in address) {
        this->id = id;
        this->username = username;
        this->password = password;
        this->address = address;
    }

    User(int id, string username, string password) {
        this->id = id;
        this->username = username;
        this->password = password;
    }

    int id;
    string username;
    string password;
    sockaddr_in address;

    bool operator==(const User &rhs) const {
        return id == rhs.id;
    }

    bool operator!=(const User &rhs) const {
        return !(rhs == *this);
    }

    /*
     * Write the member variables to stream objects
     */
    friend std::ostream & operator << (std::ostream &out, const User & obj)
    {
        out << obj.id << " " << obj.username << " " << obj.password;
        return out;
    }

    /*
    * Read data from stream object and fill it in member variables
    */
    friend std::istream & operator >> (std::istream &in, User &obj)
    {
        in >> obj.id;
        in >> obj.username;
        in >> obj.password;
        return in;
    }

};

class Message{
public:
    User src;
    User dst;
    string text;

    Message(){}

    Message(User src, User dst, string text){
        this->src = src;
        this->dst = dst;
        this->text = text;
    }

/*
    * Write the member variables to stream objects
    */
    friend std::ostream & operator << (std::ostream &out, const Message & obj)
    {
        out << obj.src << " " << obj.dst << " " << obj.text;
        return out;
    }

    /*
    * Read data from stream object and fill it in member variables
    */
    friend std::istream & operator >> (std::istream &in, Message &obj)
    {
        in >> obj.src;
        in >> obj.dst;
        in >> obj.text;

        return in;
    }
};

class Peer{
private:
    string peer_name;
    int peer_socket;
    sockaddr_in peer_address;

    bool socket_init;
    bool address_init;

public:
    Peer(){
        socket_init = false;
        address_init = false;
    }


    bool is_socket_init() const {
        return socket_init;
    }

    bool is_address_init() const {
        return address_init;
    }

    const sockaddr_in &get_peer_address() const {
        return peer_address;
    }

    const string &get_peer_name() const {
        return peer_name;
    }

    void set_peer_name(const string &peerName) {
        peer_name = peerName;
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


};

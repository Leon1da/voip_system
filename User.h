//
// Created by leonardo on 07/10/20.
//

#ifndef CHAT_UDP_USER_H
#define CHAT_UDP_USER_H

#endif //CHAT_UDP_USER_H

#include <iostream>
#include <netinet/in.h>


using namespace std;

class User{

public:

    User()= default;;

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

    int id{};
    string username;
    string password;
    sockaddr_in address{};

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

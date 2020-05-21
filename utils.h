//
// Created by leonardo on 17/05/20.
//

#ifndef CLIENT_SERVER_TCP_UTILS_H
#define CLIENT_SERVER_TCP_UTILS_H

#include <string>
#include <list>
#include <string.h>


using namespace std;

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 3000

#define MSG_BUFSIZE 512
#define SRC 8
#define DEST 8

#define USERNAME_MAXLEN 30
#define PASSWORD_MAXLEN 30


class user;
class chat;
class msg;


class user {
private: int id;
private: string username;
private: string password;
private: int fd;

public:
    user(int id, string username,  string password, int fd) {
        user::id = id;
        user::username = username;
        user::password = password;
        user::fd = fd;
    }
    user(int id, string username,  string password) {
        user::id = id;
        user::username = username;
        user::password = password;
    }

    user(){}

    void setId(int id) {
        user::id = id;
    }

    void setUsername( string username) {
        user::username = username;
    }

    void setPassword(string password) {
        user::password = password;
    }

    void setFD(int fd){
        user::fd = fd;
    }

    int getId() {
        return id;
    }

    string getUsername() {
        return username;
    }

    string getPassword() {
        return password;
    }

    int getFD(){
        return fd;
    }
};


class msg{
private: user mittente;       //fd
private: user destinatario;   //fd
private: string content;
public:
    msg(user mittente, user destinatario, string content){
        msg::mittente = mittente;
        msg::destinatario = destinatario;
        msg::content = content;
    }

    msg(){}
    user getMittente()  {
        return mittente;
    }

    user getDestinatario()  {
        return destinatario;
    }

    const string getContent()  {
        return content;
    }

    void setMittente(user mittente) {
        msg::mittente = mittente;
    }

    void setDestinatario(user destinatario) {
        msg::destinatario = destinatario;
    }

    void setContent(const string &content) {
        msg::content = content;
    }

};

class chat {
private: user user_1, user_2;
private: list<msg> messages;


public:
    chat(user user_1, user user_2, list <msg> messages){
        chat::user_1 = user_1;
        chat::user_2 = user_2;
        chat::messages = messages;
    }
    chat(){}

    user getUser1() {
        return user_1;
    }
    user getUser2() {
        return user_2;
    }

    list <msg> getMessages() {
        return messages;
    }

    void setUser1(user user_1){
        chat::user_1 = user_1;
    }

    void setUser2(user user_2){
        chat::user_2 = user_2;
    }

    void setMessages(list <msg> messages) {
        chat::messages = messages;
    }

};



#endif //CLIENT_SERVER_TCP_UTILS_H

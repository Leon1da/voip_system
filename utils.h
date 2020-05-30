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

#define MSG_SIZE 512
#define CMD_SIZE 16

#define USERNAME_MAXLEN 30
#define PASSWORD_MAXLEN 30

#define MAX_CONN_QUEUE 5

enum TYPE{
    BROADCAST = 0,
    UNICAST = 1,
    AUTH = 2,
    INFO = 3,
    CHAT = 4

};

class user;
class chat;
class msg;


class user {
private: int id;
private: string username;
private: string password;
private: int fd;
private: bool logged;
public:
    bool operator==(const user &rhs) const {
        return id == rhs.id;
    }

    bool operator!=(const user &rhs) const {
        return !(rhs == *this);
    }

public:
    user(int id, string username,  string password, int fd, bool logged) {
        user::id = id;
        user::username = username;
        user::password = password;
        user::fd = fd;
        user::logged = logged;
    }
    user(int id, string username,  string password) {
        user::id = id;
        user::username = username;
        user::password = password;
    }

    user(){}

    void setLogged(bool log){
        user::logged = log;
    }

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

    bool isLogged(){
        return logged;
    }
};


class msg{
private: TYPE type;
private: user mittente;       //fd
private: user destinatario;   //fd
private: string content;
public:
    msg(TYPE type, user mittente, user destinatario, string content){
        msg::type = type;
        msg::mittente = mittente;
        msg::destinatario = destinatario;
        msg::content = content;
    }

    msg(){}
    TYPE getType(){
        return type;
    }
    user getMittente()  {
        return mittente;
    }

    user getDestinatario()  {
        return destinatario;
    }

    const string getContent()  {
        return content;
    }
    void setType(TYPE type){
        msg::type = type;
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

private: list<user> users;
private: list<string> messages;


public:
    chat(list<user> users, list <string> messages){
        chat::users = users;
        chat::messages = messages;
    }

    chat(){}

    list<user> getUsers() {
        return users;
    }

    list <string> getMessages() {
        return messages;
    }

    void setUsers(list<user> users){
        chat::users = users;
    }

    void setMessages(list <string> messages) {
        chat::messages = messages;
    }
    void addMessage(string msg){
        chat::messages.push_back(msg);
    }

};



#endif //CLIENT_SERVER_TCP_UTILS_H

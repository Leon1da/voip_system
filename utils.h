//
// Created by leonardo on 17/05/20.
//

#ifndef CLIENT_SERVER_TCP_UTILS_H
#define CLIENT_SERVER_TCP_UTILS_H

#include <string>
#include <list>
#include <cstring>
#include <iostream>


#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 3000

#define MSG_SIZE 1024
#define CMD_SIZE 16

#define USERNAME_MAXLEN 30
#define PASSWORD_MAXLEN 30

#define MAX_CONN_QUEUE 5

#define LOG 0
#define USERS_DB "users.txt"
#define CHATS_DB "chats.txt"



using namespace std;

// prototipi dei metodi definiti in send_recv.c
void    send(int socket, const char *msg);
size_t  recv(int socket, char *buf, size_t buf_len);



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

    /*
     * Write the member variables to stream objects
     */
    friend std::ostream & operator << (std::ostream &out, const user & obj)
    {
        out << obj.id << " " << obj.username << " " << obj.password << endl;
        return out;
    }

    /*
    * Read data from stream object and fill it in member variables
    */
    friend std::istream & operator >> (std::istream &in,  user &obj)
    {
        in >> obj.id;
        in >> obj.username;
        in >> obj.password;
        return in;
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
private: user mittente;       //fd
private: user destinatario;   //fd
private: string content;
public:
    msg( user mittente, user destinatario, string content){
        msg::mittente = mittente;
        msg::destinatario = destinatario;
        msg::content = content;
    }


    msg() {

    }

    /*
     * Write the member variables to stream objects
     */
    friend std::ostream & operator << (std::ostream &out, const msg & obj)
    {
        out << obj.content << endl;
        return out;
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
private: list<msg> messages;

    /*
     * Write the member variables to stream objects
     */
    friend std::ostream & operator << (std::ostream &out, const chat & obj)
    {
        out << "users:" << endl;
        for(user u : obj.users) out << u.getUsername();
        out << "chats:" << endl;
        for(msg m : obj.messages) out << m;
        return out;
    }

public:
    chat(list<user> users, list <msg> messages){
        chat::users = users;
        chat::messages = messages;
    }

    chat(){}

    list<user> getUsers() {
        return users;
    }

    list <msg> getMessages() {
        return messages;
    }

    void setUsers(list<user> users){
        chat::users = users;
    }

    void setMessages(list <msg> messages) {
        chat::messages = messages;
    }
    void addMessage(msg msg){
        chat::messages.push_back(msg);
    }

};



#endif //CLIENT_SERVER_TCP_UTILS_H

//
// Created by leonardo on 13/05/20.
//
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include "utils.h"
#include "methods.h"

#include <map>

using namespace std;

#define MAX_CONN 10

//coda di messaggi
list<msg>* msg_queue = new list<msg>();

list<user*>* users = new list<user*>();
list<chat*>* chats = new list<chat*>();


list<user*>* active_users = new list<user*>();
list<chat*>* active_chats = new list<chat*>();



int serverSd;

chat* start_chat_session(user* user_1);

user* authentication(int client_desc);

void connection_handler(int client_desc){

    // autenticazione client
    user* usr = authentication(client_desc);
    if(usr) send_msg(client_desc,"Login effettuato con successo");
    else {
        send_msg(client_desc,"Login fallito");
        cerr << "autenticazione fallita!"<<endl;
    }

    cout << usr->getUsername() << " si e` loggato." << endl;

    while (1){

    }

    chat* chat = start_chat_session(usr);
    if(chat) send_msg(client_desc,"Utente disponibile a chattare");
    else{
        send_msg(client_desc,"Utente non disponibile");
        cerr << "scelta destinatario fallita"<< endl;
    }

    cout << "inizio scambio messaggi"<<endl;
    //buffer to send and receive messages with
    char message[MSG_BUFSIZE];

    int running = true;
    while (running) {

        int bytesRead = recv_msg(usr->getFD(),message,sizeof(message));
        if(bytesRead>0){
            char msg_buf[MSG_BUFSIZE];
            memcpy(msg_buf, message, MSG_BUFSIZE);
            msg* newMsg = new msg(chat->getUser1(),chat->getUser2(),msg_buf);
            msg_queue->push_back(*newMsg);
        }

    }

    close(client_desc);

}

// scelta chat
chat* start_chat_session(user* user_1) {

    chat* newChat = {0};
    do{
        string message="";
        // invio messaggio benvenuto
        if(active_users->size() < 2){
            message = "Nessun utente online\n";
        } else{
            message = "Utenti Online: \n";
            for (user *user : *active_users) {
                if(user->getId() != user_1->getId()){
                    message.append(to_string(user->getId()));
                    message.append(" - ");
                    message.append(user->getUsername() + "\n");
                }
            }
            message.append("Seleziona l'id dell'utente con cui desideri parlare\n");
        }

        send_msg(user_1->getFD(),message.c_str());

        char msg[DEST];
        recv_msg(user_1->getFD(),msg,DEST);

        int dest = atoi(msg); //destinatario chat

        // controllo se e` gia attiva una chat
        for(chat* &chat: *active_chats){
            if(chat->getUser1().getId() == dest || chat->getUser1().getId() == user_1->getId()){
                if(chat->getUser2().getId() == dest || chat->getUser2().getId() == user_1->getId()){
                    newChat = chat;
                }
            }

        }

        // altrimenti ne creo una nuova
        if(!newChat){
            for (user* &active : *active_users) {
                if(active->getId() == dest){
                    newChat = new chat();
                    newChat->setUser1(*user_1);
                    newChat->setUser2(*active);
                    active_chats->push_back(newChat);
                    break;
                }
            }
        }

        if(!newChat) send_msg(user_1->getFD(),"utente non disponibile");

    }while (!newChat);

    return  newChat;

}

// autenticazione client
user* authentication(int client_desc) {

    // invio messaggio benvenuto
    string msg = "Benvenuto, effettua il login per iniziare a chattare.\n";
    send_msg(client_desc,msg.c_str());

    // verifico se l'utente e` registrato
    user* utente = {0};

    do{
        send_msg(client_desc,"username: ");
        char username[USERNAME_MAXLEN];
        if(recv_msg(client_desc,username,sizeof(username))<0){
            //errore
            cout << "errore username";
        }

        send_msg(client_desc,"password: ");
        char password[PASSWORD_MAXLEN];
        if(recv_msg(client_desc,password,sizeof(password))<0){
            //errore
            cout << "errore password";
        }

        for(user* &u : *users){

            if(strcmp(u->getUsername().c_str(),username) == 0 &&
               strcmp(u->getPassword().c_str(),password) == 0){
                // utente attivo
                utente = new user();
                utente->setId(u->getId());
                utente->setUsername(u->getUsername());
                utente->setPassword(u->getPassword());
                utente->setFD(client_desc);

                // lo metto nella lista di utenti attivi
                active_users->push_back(utente);
                break;
            }
        }
        if(!utente) send_msg(client_desc,"credenziali errate");

    }while (!utente);

    return utente;
}

void queue_routine(){
    while (1){
        msg msg;
        if(!msg_queue->empty()){
            // prendo il primo messagio in coda
            msg = msg_queue->front();
            msg_queue->pop_front();
        }
        else break;

        cout<<"unqueue"<<endl;

        user mittente = msg.getMittente();
        user destinatario = msg.getDestinatario();
        string text_msg = mittente.getUsername().append(": ").append(msg.getContent());
        cout << text_msg <<endl;
        //invio il messaggio
        send_msg(msg.getDestinatario().getFD(),text_msg.c_str());
    }

}

//Server side
int main(int argc, char *argv[])
{
    user* u1 = new user(10,"leonardo","leonardo");
    user* u2 = new user(11,"francesco","francesco");
    user* u3 = new user(12,"matteo","matteo");
    user* u4 = new user(13,"federico","federico");
    users->push_back(u1);
    users->push_back(u2);
    users->push_back(u3);
    users->push_back(u4);


    //setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERVER_PORT);

    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSd < 0){
        perror("Error establishing the server socket: ");
        exit(0);
    }

    int reuse = 1;
    int err = setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (err < 0){
        perror("Error establishing the server socket: ");
        exit(0);
    }

    //bind the socket to its local address
    int bindStatus = bind(serverSd, (struct sockaddr*) &servAddr,
                          sizeof(servAddr));
    if(bindStatus < 0){
        perror("Error binding socket to local address: ");
        exit(0);
    }

    cout << "Waiting for a client to connect..." << endl;
    //listen for up to 5 requests at a time
    if(listen(serverSd,5) < 0){
        perror("Error listen on server socket: ");
        exit(0);
    }

    // init message queue routine

    thread* routine = new thread(queue_routine);

    //receive a request from client using accept
    //we need a new address to connect with the client
    sockaddr_in* newSockAddr = (sockaddr_in*) calloc(1,sizeof(sockaddr_in));
    socklen_t newSockAddrSize = sizeof(newSockAddr);

    // one thread for each incoming connection
    list<thread*> connection;
    //accept, create a new socket descriptor to
    //handle the new connection with client

    int running = true;
    while (running){
        int client_fd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if(client_fd < 0){
            perror("Error accepting request from client: ");
            exit(1);
        }

        cout << "New connection incoming" << endl;

        // client connection helndler
        thread* t1 = new thread(connection_handler,client_fd);
        connection.push_back(t1);

        newSockAddr = (sockaddr_in*) calloc(1,sizeof(sockaddr_in));

    }

    close(serverSd);
    cout << "Connection closed..." << endl;

    return 0;
}

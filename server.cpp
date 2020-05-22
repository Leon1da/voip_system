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

list<user>* active_users = new list<user>();



int serverSd;


user * authentication(int client_desc);

void connection_handler(int client_desc){

    // autenticazione client
    user* newUser = authentication(client_desc);
    if(newUser!= nullptr){
        msg msg;
        msg.setType(BROADCAST);
        msg.setContent(newUser->getUsername() + " e` Online.");
        msg_queue->push_back(msg);
    } else return;

    // start chat session

    cout << "start chat session" <<endl;
    while (1) {

        send_msg(newUser->getFD(),"[SERVER] Per inviare un messaggio selezione un utente scrivendo il suo username: <username>");
        send_msg(newUser->getFD(),"[SERVER] Utenti Online:");
        if(active_users->size()>1){
            for (user &user : *active_users) {
                if(user != *newUser) send_msg(newUser->getFD(), user.getUsername().c_str());

            }
        } else{
            send_msg(newUser->getFD(), "[SERVER] Non ci sono altri utenti online");
        }

        char dest[USERNAME_MAXLEN];
        int bytesRead = recv_msg(newUser->getFD(),dest,USERNAME_MAXLEN);
        if(bytesRead>0){
            user* destUser = nullptr;
            for(user &u : *active_users){
                if(u.getUsername() == dest){
                    destUser = &u;
                    break;
                }
            }

            if(destUser == nullptr) send_msg(newUser->getFD(),"[SERVER] L'utente che hai selezionato non e` attivo");
            else{
                send_msg(newUser->getFD(),"[SERVER] L'utente e` online, inzia a chattare");

                char message[MSG_BUFSIZE];
                bytesRead = recv_msg(newUser->getFD(), message, sizeof(message));
                if (bytesRead > 0) {
                    string msg = "[";
                    msg.append(newUser->getUsername());
                    msg.append("] ");
                    msg.append(message);
                    send_msg(destUser->getFD(),msg.c_str());
                }
            }
        }
    }

    cout << "end_chat_session"<<endl;

    close(client_desc);

}

// autenticazione client
user* authentication(int client_desc) {

    user* utente;
    // invio messaggio benvenuto
    send_msg(client_desc, "Benvenuto, effettua il login per iniziare a chattare.");

    // verifico se l'utente e` registrato
    bool login = false;
    do{

        send_msg(client_desc,"Inserisci username: ");
        char username[USERNAME_MAXLEN];
        if(recv_msg(client_desc,username,sizeof(username)) < 0){
            cout << "errore username";
            return nullptr;
        }

        send_msg(client_desc,"Inserisci password: ");
        char password[PASSWORD_MAXLEN];
        if(recv_msg(client_desc,password,sizeof(password))<0){
            //errore
            cout << "errore password";
            return nullptr;
        }

        for(user* &u : *users){
            if(strcmp(u->getUsername().c_str(),username) == 0 &&
               strcmp(u->getPassword().c_str(),password) == 0){

                // utente registrato
                utente = new user(
                        u->getId(),
                        u->getUsername(),
                        u->getPassword(),
                        client_desc
                        );
                // lo metto nella lista di utenti attivi
                active_users->push_back(*utente);
                login = true;
                break;
            }
        }
        if(!login) send_msg(client_desc,"Login Fallito riprova.");
        else send_msg(client_desc,"Login avvenuto con successo");

    }while (!login);


    return utente;
}

void queue_routine(){

    cout << "Start message queue routine" << endl;

    while (1) {
        if (msg_queue->empty()) break;

        cout << "Ready for enqueue" << endl;

        msg msg = msg_queue->front();
        msg_queue->pop_front();

        cout << "Unqueue" << endl;

        TYPE type = msg.getType();
        string text;
        user destinatario;
        user mittente;
        string header;
        switch (type) {
            case BROADCAST:
                text = msg.getContent();
                header = "[SERVER] ";
                text = header.append(text);
                for (user &u : *active_users) {
                    send_msg(u.getFD(), text.c_str());
                }
                break;
            case UNICAST:
                text = msg.getContent();
                mittente = msg.getMittente();
                destinatario = msg.getDestinatario();
                header = "[" + mittente.getUsername() + "]" + text;
                send_msg(destinatario.getFD(), header.c_str());
                break;
        }
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

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

// database
list<chat*>* chats = new list<chat*>();
list<user*>* users = new list<user*>();
// end database


list<user>* active_users = new list<user>();
list<chat>* active_chat = new list<chat>();



int serverSd;


user * authentication(int client_desc);

void help_message(user *pUser);

void list_users(user *pUser);

void start_chat(user *pUser);

void start_video_chat(user *pUser);

bool quit_chat(user *pUser);

bool isAlreadyLogged(user usr);

void connection_handler(int client_desc){

    // autenticazione client
    user* newUser = authentication(client_desc);
    if(newUser == nullptr){
        //errore interno
    }

    // start chat session
    cout << newUser->getUsername() << " si e` autenticato." <<endl;

    help_message(newUser);

    char cmd_buf[CMD_SIZE];
    while (1){
        int byteRead = recv_msg(newUser->getFD(), cmd_buf, CMD_SIZE);
        if(byteRead > 0){
            if(strcmp(cmd_buf,"help") == 0) help_message(newUser);
            else if(strcmp(cmd_buf,"users") == 0){
                list_users(newUser);
                help_message(newUser);
            }
            else if(strcmp(cmd_buf,"chat") == 0) start_chat(newUser);
            else if(strcmp(cmd_buf,"video") == 0) start_video_chat(newUser);
            else if(strcmp(cmd_buf,"exit") == 0){
                if(quit_chat(newUser)) break;
                else help_message(newUser);
            }

        }
    }


    free(newUser);

    // avvertire i client e pulire la memoria

    cout << "end_chat_session"<<endl;

    close(client_desc);

}

bool quit_chat(user *pUser) {
    string quit_msg = "******************** [QUIT] ********************\n"
                      "* Sei sicuro di voler uscire? (y/n)            *";
    send_msg(pUser->getFD(), quit_msg.c_str());

    char buf[MSG_SIZE];
    int readBytes = recv_msg(pUser->getFD(), buf, MSG_SIZE);
    if(readBytes > 0){
        if(strcmp(buf,"y") == 0) {
            pUser->setLogged(false);
            active_users->remove(*pUser);
            return true;
        }
    }

    return false;
}

void start_video_chat(user *pUser) {
    send_msg(pUser->getFD(),"******************** [VIDEOCHAT] ********************");
}

void start_chat(user *newUser) {
    send_msg(newUser->getFD(),"******************** [CHAT] ********************");
    send_msg(newUser->getFD(),"* Seleziona l'utente con cui vuoi chattare *");

    user *destUser = nullptr;
    while (destUser == nullptr) {

        // mostro gli utenti online
        list_users(newUser);

        // aspetto che il client scelga l'utente
        char buf_username_dest[USERNAME_MAXLEN];
        int bytesRead = recv_msg(newUser->getFD(), buf_username_dest, USERNAME_MAXLEN);
        if (bytesRead > 0) {

            // il cliente ha scelto di tornare indietro
            if(strcmp(buf_username_dest, "return") == 0){
                break;
            }

            for (user &u : *active_users) {
                if (u.getUsername() == buf_username_dest) {
                    destUser = &u;
                    break;
                }
            }

            if (destUser == nullptr) send_msg(newUser->getFD(), "[SERVER] L'utente che hai selezionato non e` online");
            else send_msg(newUser->getFD(), "[SERVER] L'utente e` online, inzia a chattare");
        }

    }

    // verifico se c'e` gia una chat aperta
//    chat* newChat = nullptr;
//    for (chat &c : *active_chat) {
//        list<user> chat_users = c.getUsers();
//        user first = chat_users.front();
//        user second = chat_users.back();
//        if(first == *newUser && second == *destUser
//            || first == *destUser && second == *newUser){
//
//            // c'e` una chat attiva (salvero` i messaggi in tale chat)
//            newChat = &c;
//        }
//    }

//    if(newChat == nullptr) {
//        // non c'e` una chat attiva la creo
//        newChat = new chat(list<user>(newUser,destUser), list<string>());
//    }



    cout << "Chat attiva tra " << newUser->getUsername() << " e " << destUser->getUsername() << endl;

//    send_msg(destUser->getFD(), "[SERVER] - return - per uscire da questa chat");

    char message[MSG_SIZE];
    while (1){
        int bytesRead = recv_msg(newUser->getFD(), message, sizeof(message));
        if (bytesRead > 0) {
            if(strcmp(message,"return") == 0) {
                // salvo la chat nel db
//                 chats->push_back(newChat);
                break;
            }
            else{
                string msg = "[";
                msg.append(newUser->getUsername()).append("] ").append(message);
                send_msg(destUser->getFD(), msg.c_str());

                // metto i messaggi inviati nella chat
//                newChat->addMessage(msg);


            }
        }

    }

    help_message(newUser);

}

void list_users(user *pUser) {

    string list_msg = "******************** [USERS] ********************\n";

    if(active_users->size()>1){
        list_msg.append("* Utenti Online:                                \n*");
        for (user &user : *active_users) {
            if(user != *pUser) list_msg.append(string("* ").append(user.getUsername()).append("\n").c_str());
        }
    } else{
        list_msg.append("* Non ci sono altri utenti online             *");
    }

    send_msg(pUser->getFD(),list_msg.c_str());


}

void help_message(user *pUser) {

    string help_msg = "******************** [HELP] ********************\n"
                         "* Benvenuto/a nella chat.                      *\n"
                         "* Comandi disponibili:                         *\n"
                         "* - users - mostra gli utenti online           *\n"
                         "* - video - (coming soon)                      *\n"
                         "* - chat - apre una chat con un utente online  *\n"
                         "*   - return - esce da una chat aperta         *\n"
                         "* - help - mostra questo messaggio             *\n"
                         "* - exit - logout                              *\n"
                         "************************************************";

    send_msg(pUser->getFD(), help_msg.c_str());

}

// autenticazione client
user* authentication(int client_desc) {

    // invio messaggio benvenuto
    send_msg(client_desc, "******************** AUTHENTICATION ********************");
    send_msg(client_desc, "* Benvenuto, effettua il login per iniziare a chattare.*");

    user* utente = nullptr;
    while (utente == nullptr){

        // username
        send_msg(client_desc,"* Inserisci username: ");
        char username[USERNAME_MAXLEN];
        if(recv_msg(client_desc,username,sizeof(username)) < 0){
            cout << "errore username";
            return nullptr;
        }

        // password
        send_msg(client_desc,"* Inserisci password: ");
        char password[PASSWORD_MAXLEN];
        if(recv_msg(client_desc,password,sizeof(password))<0){
            //errore
            cout << "errore password";
            return nullptr;
        }

        // check username/password
        for(user* &u : *users){
            if(strcmp(u->getUsername().c_str(),username) == 0 &&
               strcmp(u->getPassword().c_str(),password) == 0){
                if(isAlreadyLogged(*u)) send_msg(client_desc, "* Sei gia loggato.");
                else{
                    // utente registrato
                    utente = new user(
                            u->getId(),
                            u->getUsername(),
                            u->getPassword(),
                            client_desc,
                            true
                    );
                    // lo metto nella lista di utenti attivi
                    active_users->push_back(*utente);
                }
                break;

            }
        }


        if(utente == nullptr) send_msg(client_desc,"[SERVER] Login Fallito riprova.");
        else send_msg(client_desc,"[SERVER] Login avvenuto con successo");

    }

    return utente;
}

bool isAlreadyLogged(user usr) {
    for (user &user : *active_users) if(user == usr) return true;
    return false;
}

//Server side
int main(int argc, char *argv[])
{
    user* u1 = new user(10,"leonardo","leonardo");
    user* u2 = new user(11,"francesco","francesco");
    user* u3 = new user(12,"matteo","matteo");
    user* u4 = new user(13,"federico","federico");
    user* u5 = new user(14,"maria","maria");
    user* u6 = new user(15,"paolo","paolo");
    user* u7 = new user(16,"giovanni","giovanni");
    user* u8 = new user(17,"nino","nino");

    users->push_back(u1);
    users->push_back(u2);
    users->push_back(u3);
    users->push_back(u4);
    users->push_back(u5);
    users->push_back(u6);
    users->push_back(u7);
    users->push_back(u8);


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

    //receive a request from client using accept
    //we need a new address to connect with the client
    sockaddr_in* newSockAddr = (sockaddr_in*) calloc(1,sizeof(sockaddr_in));
    socklen_t newSockAddrSize = sizeof(newSockAddr);

    // one thread for each incoming connection
    // list<thread*> connection;
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
        // thread t1 = thread(connection_handler,client_fd);
        // connection.push_back(t1);
        new thread(connection_handler,client_fd);

        newSockAddr = (sockaddr_in*) calloc(1,sizeof(sockaddr_in));

    }



    close(serverSd);
    cout << "Connection closed..." << endl;

    return 0;
}

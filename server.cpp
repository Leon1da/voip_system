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
list<chat*>* g_chats = new list<chat*>();
list<user*>* g_users = new list<user*>();
// end database


list<user*>* g_active_users = new list<user*>();
list<chat*>* g_active_chat = new list<chat*>();



int serverSd;


user * authentication(int client_desc);

void help_message(user *pUser);

void list_users(user *pUser);

void start_chat(user *pUser);

void start_video_chat(user *pUser);

bool quit_chat(user *pUser);

bool isAlreadyLogged(user usr);

// dato un utente restituisce tutte le chat attive aperte
list<chat*>* getUserChats(user* usr){
    list<chat*>* user_chats = new list<chat*>();
    for (chat* &c : *g_active_chat) {
        list<user> chat_users = c->getUsers();
        if(chat_users.front() == *usr || chat_users.back() == *usr) {
            user_chats->push_back(c);
        }
    }
    return user_chats;

}

void start_video_chat(user *pUser) {
    string video_chat_msg;
    video_chat_msg.append("********************* * VIDEOCHAT * ********************\n");
    video_chat_msg.append("********************************************************\n");

    send_msg(pUser->getFD(),video_chat_msg.c_str());
}

void list_users(user *pUser) {

    string list_msg = "************************* USERS ************************\n";

    if(g_active_users->size() > 1){
        list_msg.append("* Utenti Online:                                       *\n");
        for (user* &user : *g_active_users) {
            if(*user != *pUser){
                list_msg.append(string("* - "));
                list_msg.append(user->getUsername());
                list_msg.append("                                           *\n");
            }
        }
    } else{
        list_msg.append("* Non ci sono altri utenti online                      *\n");
    }

    list_msg.append("********************************************************\n");
    send_msg(pUser->getFD(),list_msg.c_str());


}

void help_message(user *pUser) {

    string help_msg_1 = "********************      HELP      ********************\n"
                        "* Benvenuto/a nella chat.                              *\n";

    string help_msg_2 = "* Comandi disponibili:                                 *\n"
                        "* - users - mostra gli utenti online                   *\n"
                        "* - video - (coming soon)                              *\n"
                        "* - chat - apre una chat con un utente online          *\n"
                        "*   - return - esce da una chat aperta                 *\n"
                        "* - help - mostra questo messaggio                     *\n"
                        "* - exit - logout                                      *\n"
                        "********************************************************\n";

    send_msg(pUser->getFD(), help_msg_1.c_str());
    send_msg(pUser->getFD(), help_msg_2.c_str());


}


void start_chat(user *newUser) {

    string chat_msg;
    chat_msg.append("********************* *   CHAT    * ********************\n");
    chat_msg.append("* Seleziona l'utente con cui vuoi chattare             *\n");
    chat_msg.append("* return - per tornare al menu` principale             *\n");
    chat_msg.append("* users - per vedere gli utenti online                 *\n");
    chat_msg.append("********************************************************\n");

    send_msg(newUser->getFD(),chat_msg.c_str());

    user* destUser = nullptr;
    while (destUser == nullptr) {

        char buf_username_dest[USERNAME_MAXLEN];
        int bytesRead = recv_msg(newUser->getFD(), buf_username_dest, USERNAME_MAXLEN);
        if (bytesRead > 0) {

            if(strcmp(buf_username_dest, "return") == 0) return; // torna indietro
            if(strcmp(buf_username_dest, "users") == 0) list_users(newUser); // mostra gli utenti online
            else{
                // verifico se l'utente scelto e` online
                for (user* &u : *g_active_users) {
                    if (u->getUsername() == buf_username_dest && u->getUsername() != newUser->getUsername()) {
                        destUser = u;
                        break;
                    }
                }
                if (destUser == nullptr) send_msg(newUser->getFD(), "[SERVER][WARNING] L'utente che hai selezionato non e` online");
                else{
                    string msg ="";
                    msg.append("[SERVER][SUCCESS] ").append(destUser->getUsername()).append(" e` online, scrivi qualcosa");
                    send_msg(newUser->getFD(), msg.c_str());
                    msg = "";
                    msg.append("[SERVER][INFO] new incoming chat from ").append(newUser->getUsername());
                    send_msg(destUser->getFD(), msg.c_str());
                }
            }
         }
    }

    cout << "[" << newUser->getUsername() << "] ha aperto la chat con [" << destUser->getUsername() << "]" << endl;

    char message[MSG_SIZE];
    while (1){
        int bytesRead = recv_msg(newUser->getFD(), message, sizeof(message));
        if (bytesRead > 0) {
            if(strcmp(message,"return") == 0) break;    // esco dalla chat
            else{
                string msg = "[";
                msg.append(newUser->getUsername()).append("] ").append(message);

                if(destUser->isLogged()){
                    // cout << destUser->getUsername() << " online." <<endl;
                    send_msg(destUser->getFD(), msg.c_str());
                    cout << newUser->getUsername() << " send message to " << destUser->getUsername() << endl;
                }
                else {
                    // cout << destUser->getUsername() << " offline." <<endl;
                    string alert_msg = "[SERVER][INFO] ";
                    alert_msg.append(destUser->getUsername());
                    alert_msg.append(" si e` disconnesso.");
                    send_msg(newUser->getFD(), alert_msg.c_str());
                    break;
                }
            }
        }

    }
    send_msg(newUser->getFD(), "Sei nel menu principale");
    help_message(newUser);

}


void connection_handler(int client_desc){
    // autenticazione client
    user* newUser = authentication(client_desc);
    if(newUser == nullptr){}

    // start chat session
    cout << newUser->getUsername() << " si e` autenticato." <<endl;

    // mostra azioni disponibili
    help_message(newUser);

    char cmd_buf[CMD_SIZE];
    while (1){
        int byteRead = recv_msg(newUser->getFD(), cmd_buf, CMD_SIZE);
        if(byteRead > 0){
            if(strcmp(cmd_buf,"help") == 0) help_message(newUser);
            else if(strcmp(cmd_buf,"users") == 0) list_users(newUser);
            else if(strcmp(cmd_buf,"chat") == 0) start_chat(newUser);
            else if(strcmp(cmd_buf,"video") == 0) start_video_chat(newUser);
            else if(strcmp(cmd_buf,"exit") == 0) break;
            else{
                string msg;
                msg.append("[SERVER][INFO] Comando sconosciuto\n");
                msg.append("[SERVER][INFO] help - mostra le azioni disponibili");
                send_msg(newUser->getFD(),msg.c_str());
            }

        }
    }

    cout << newUser->getUsername() << " ha lasciato la chat." << endl;

//    list<chat*> *chats = getUserChats(newUser);

    newUser->setLogged(false);
    cout << "newUser setLogged false" <<endl;
    g_active_users->remove(newUser);
    cout << "active users remove new user" <<endl;
//    free(newUser);
    close(client_desc);

}


// autenticazione client
user* authentication(int client_desc) {

    // invio messaggio benvenuto
    string auth_msg = "******************** AUTHENTICATION ********************\n"
                      "* Benvenuto, effettua il login per iniziare a chattare.*\n";
    send_msg(client_desc,auth_msg.c_str());

    user* utente = nullptr;
    while (utente == nullptr){

        // username
        auth_msg = "* Inserisci username:                                  *\n";
        send_msg(client_desc,auth_msg.c_str());
        char username[USERNAME_MAXLEN];
        if(recv_msg(client_desc,username,sizeof(username)) < 0){
            cout << "errore username";
            return nullptr;
        }

        // password
        auth_msg = "* Inserisci password:                                  *";
        send_msg(client_desc, auth_msg.c_str());
        char password[PASSWORD_MAXLEN];
        if(recv_msg(client_desc,password,sizeof(password))<0){
            //errore
            cout << "errore password";
            return nullptr;
        }

        // check username/password
        for(user* &u : *g_users){
            if(strcmp(u->getUsername().c_str(),username) == 0 &&
               strcmp(u->getPassword().c_str(),password) == 0){
                if(isAlreadyLogged(*u)) send_msg(client_desc, "[SERVER][INFO] Sei gia loggato.");
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
                    g_active_users->push_back(utente);
                }
                break;
            }
        }


        if(utente == nullptr) send_msg(client_desc,"[SERVER][WARNING] Login Fallito riprova.");
        else send_msg(client_desc,"[SERVER][SUCCESS] Login avvenuto con successo");

    }

    send_msg(client_desc,"********************************************************\n");

    return utente;
}

bool isAlreadyLogged(user usr) {
    for (user* &user : *g_active_users) if(*user == usr) return true;
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

    g_users->push_back(u1);
    g_users->push_back(u2);
    g_users->push_back(u3);
    g_users->push_back(u4);
    g_users->push_back(u5);
    g_users->push_back(u6);
    g_users->push_back(u7);
    g_users->push_back(u8);


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

    // now server is ready for accept new connections from clients

    // create a thread for manage server with shell
    // and another thread for accecpt incoming connection
    // [TODO]

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
        thread* client = new thread(connection_handler,client_fd);
        //client_thread.detach();
        connection.push_back(client);

        newSockAddr = (sockaddr_in*) calloc(1,sizeof(sockaddr_in));

    }


    close(serverSd);
    cout << "Connection closed..." << endl;



    return 0;
}

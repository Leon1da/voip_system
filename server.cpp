//
// Created by leonardo on 13/05/20.
//

#include <fstream>

#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <thread>
#include "utils.h"
#include "server_methods.h"

#include <map>

using namespace std;

// database
list<chat> g_chats;
list<user> g_users;
// end database


list<user*>* g_active_users = new list<user*>();
list<chat*>* g_active_chat = new list<chat*>();



bool running = true;
// one thread for each incoming connection
map<thread*,sockaddr_in*> connection;

int server_fd;


//gets user u1 and user u2
// return opened chat from u1 to u2 (or from u2 to u1)
chat* getChat(user u1, user u2){
    for (chat* &c : *g_active_chat) {
        list<user> chat_users = c->getUsers();
        user front = chat_users.front();
        user back = chat_users.back();
        if(front == u1 || back == u1 && front == u2 || back == u2) return c;
    }

    return nullptr;
}

void start_video_chat(user *pUser) {
    string video_chat_msg;
    video_chat_msg.append("********************* * VIDEOCHAT * ********************\n");
    video_chat_msg.append("********************************************************\n");

    send(pUser->getFD(), video_chat_msg.c_str());
}

void list_users(user *pUser) {

    string list_msg = "************************* USERS ************************\n";

    if(g_active_users->size() > 1){
        list_msg.append("* Utenti Online:                                       *\n");
        for (user* &user : *g_active_users) {
            if(*user != *pUser && user->isLogged()){
                list_msg.append(string("* - "));
                list_msg.append(user->getUsername());
                list_msg.append("                                           *\n");
            }
        }
    } else{
        list_msg.append("* Non ci sono altri utenti online                      *\n");
    }

    list_msg.append("********************************************************\n");
    send(pUser->getFD(), list_msg.c_str());


}

void help_message(user *pUser) {

    string help_msg = "********************      HELP      ********************\n"
                        "* Benvenuto/a nella chat.                              *\n"
                        "* Comandi disponibili:                                 *\n"
                        "* - users - mostra gli utenti online                   *\n"
                        "* - video - (coming soon)                              *\n"
                        "* - chat - apre una chat con un utente online          *\n"
                        "*   - return - esce da una chat aperta                 *\n"
                        "* - help - mostra questo messaggio                     *\n"
                        "* - exit - logout                                      *\n"
                        "********************************************************\n";

    send(pUser->getFD(), help_msg.c_str());

}

user* open_chat(user *newUser) {

    string chat_msg;
    chat_msg.append("********************* *   CHAT    * ********************\n");
    chat_msg.append("* Seleziona l'utente con cui vuoi chattare             *\n");
    chat_msg.append("* return - per tornare al menu` principale             *\n");
    chat_msg.append("* users - per vedere gli utenti online                 *\n");
    chat_msg.append("********************************************************\n");

    send(newUser->getFD(), chat_msg.c_str());

    user* destUser = nullptr;
    while (destUser == nullptr) {

        char buf_username_dest[USERNAME_MAXLEN];
        int bytesRead = recv(newUser->getFD(), buf_username_dest, USERNAME_MAXLEN);
        if (bytesRead > 0) {

            if(strcmp(buf_username_dest, "return") == 0) return nullptr; // back
            if(strcmp(buf_username_dest, "users") == 0) list_users(newUser); // show online users
            else{
                // check if users is online
                for (user* &u : *g_active_users) {
                    if (u->getUsername() == buf_username_dest && u->getUsername() != newUser->getUsername()) {
                        destUser = u;
                        break;
                    }
                }
                if (destUser == nullptr)
                    send(newUser->getFD(), "[SERVER][WARNING] L'utente che hai selezionato non e` online");
                else{
                    string alert_msg ="";
                    alert_msg.append("[SERVER][SUCCESS] ").append(destUser->getUsername()).append(" e` online, scrivi qualcosa");
                    send(newUser->getFD(), alert_msg.c_str());
                    alert_msg = "";
                    alert_msg.append("[SERVER][INFO] new incoming chat from ").append(newUser->getUsername());
                    send(destUser->getFD(), alert_msg.c_str());

                    // check if just exist a persistent chat
                    chat* l_chat = getChat(*newUser, *destUser);
                    if(l_chat == nullptr){
                        l_chat = new chat({*newUser,*destUser},{});
                        cout << "Push global and local " <<endl;
                        g_active_chat->push_back(l_chat);    // push into persistent chat
                        cout << "Pushed global and local" << endl;
                    }
                }
            }
        }
     }

    return destUser;
}

void start_comunication(user* newUser, user* destUser){

    cout << "[" << newUser->getUsername() << "] ha aperto la chat con [" << destUser->getUsername() << "]" << endl;

    char message[MSG_SIZE];
    while (1){
        int bytesRead = recv(newUser->getFD(), message, sizeof(message));
        if (bytesRead > 0) {
            if(strcmp(message,"return") == 0) break;    // back on main menu
            else{
                string l_msg = "[";
                l_msg.append(newUser->getUsername()).append("] ").append(message);

                if(destUser->isLogged()){
                   send(destUser->getFD(), l_msg.c_str());
                    cout << newUser->getUsername() << " sent message to " << destUser->getUsername() << endl;

                    cout << "create message" << endl;
                    /* insert message into chat */
                    msg mess(*newUser,*destUser,l_msg);

                    cout << "add message to chat" << endl;

                    getChat(*newUser, *destUser)->addMessage(mess);

                }
                else {

                    string alert_msg = "[SERVER][INFO] ";
                    alert_msg.append(destUser->getUsername());
                    alert_msg.append(" si e` disconnesso.");
                    send(newUser->getFD(), alert_msg.c_str());
                    break;
                }
            }
        }

    }
    send(newUser->getFD(), "Sei nel menu principale");
    help_message(newUser);

}

void connection_handler(int client_desc){
    /* start auth client */
    user* newUser = authentication(client_desc);
    if(newUser == nullptr){
        cout << "Errore inaspettato chiusura server" << endl;
        exit(EXIT_FAILURE);
    }
    /* end auth client */

    // start chat session
    cout << newUser->getUsername() << " si e` autenticato." <<endl;

    // mostra azioni disponibili
    help_message(newUser);

    string msg;

    char cmd_buf[CMD_SIZE];
    while (1){
        int byteRead = recv(newUser->getFD(), cmd_buf, CMD_SIZE);
        if(byteRead > 0){
            if(strcmp(cmd_buf,"help") == 0) help_message(newUser);
            else if(strcmp(cmd_buf,"users") == 0) list_users(newUser);
            else if(strcmp(cmd_buf,"chat") == 0){
                user* newDest = open_chat(newUser);
                if(newDest != nullptr) start_comunication(newUser, newDest);

            }
            else if(strcmp(cmd_buf,"video") == 0) start_video_chat(newUser);
            else if(strcmp(cmd_buf,"exit") == 0) {
                msg = "";
                msg.append("[SERVER] Alla prossima!");
                send(newUser->getFD(), msg.c_str());
                break;
            }
            else{
                msg = "";
                msg.append("[SERVER][INFO] Comando sconosciuto\n");
                msg.append("[SERVER][INFO] help - mostra le azioni disponibili");
                send(newUser->getFD(), msg.c_str());
            }

        }
    }

    cout << newUser->getUsername() << " ha lasciato la chat." << endl;
    newUser->setLogged(false);


}

// auth client
user* authentication(int client_desc) {

    // invio messaggio benvenuto
    string auth_msg = "******************** AUTHENTICATION ********************\n"
                      "* Benvenuto, effettua il login per iniziare a chattare.*\n";
    send(client_desc, auth_msg.c_str());

    user* utente = nullptr;
    while (utente == nullptr){

        // username
        auth_msg = "* Inserisci username:                                  *\n";
        send(client_desc, auth_msg.c_str());
        char username[USERNAME_MAXLEN];
        if(recv(client_desc, username, sizeof(username)) < 0){
            cout << "errore username";
            return nullptr;
        }

        // password
        auth_msg = "* Inserisci password:                                  *";
        send(client_desc, auth_msg.c_str());
        char password[PASSWORD_MAXLEN];
        if(recv(client_desc, password, sizeof(password)) < 0){
            //errore
            cout << "errore password";
            return nullptr;
        }

        // check username/password
        for(user &u : g_users){
            if(strcmp(u.getUsername().c_str(),username) == 0 &&
               strcmp(u.getPassword().c_str(),password) == 0){
                if(isAlreadyLogged(u)) send(client_desc, "[SERVER][INFO] Sei gia loggato.");
                else{
                    // utente registrato
                    utente = new user();
                    utente->setId(u.getId());
                    utente->setUsername(u.getUsername());
                    utente->setPassword(u.getPassword());
                    utente->setFD(client_desc);
                    utente->setLogged(true);

                    // lo metto nella lista di utenti attivi
                    g_active_users->push_back(utente);
                }
                break;
            }
        }


        if(utente == nullptr) send(client_desc, "[SERVER][WARNING] Login Fallito riprova.");
        else send(client_desc, "[SERVER][SUCCESS] Login avvenuto con successo");

    }

    send(client_desc, "********************************************************\n");

    return utente;
}

bool isAlreadyLogged(user usr) {
    for (user* &user : *g_active_users) if(*user == usr && user->isLogged()) return true;
    return false;
}

void shell_routine(){

    cout << "[Shell] start_shell_routine" << endl;

    while (1){
        string line;
        getline(cin,line);
        if(strcmp(line.c_str(),"shutdown") == 0){
            cout << "[Shell] Wait until all client leave chat.." << endl;

            // join thread
            for (const auto &pair : connection) pair.first->join();

            running = false;
            break;
        }

    }

    cout << "[Shell] end_shell_routine" << endl;

}


int save_chats_to_db(string filename, list<chat*>* p_chats) {
    // Open the File
    if(LOG) cout << "Writing chats to db" << endl;
    std::ofstream out(filename);
    for(chat* &u : *p_chats) out << *u;
    out.close();
    return EXIT_SUCCESS;
}


int read_users_from_db(string filename){

    if(LOG) cout << "reading users from db" << endl;
    std::ifstream in(filename);
    while(!in.eof()){
        user u;
        in >> u;
        g_users.push_back(u);
    }
    in.close();
    return EXIT_SUCCESS;

}

//Server side
int main(int argc, char *argv[])
{

    cout << "Reading users from db.. " << endl;
    if(read_users_from_db(USERS_DB) < 0){
        cerr << "Error during read users" << endl;
        return EXIT_FAILURE;
    }
    cout << "Users read!" << endl;


//    cout << "Reading chats from db.. " << endl;
//    if(read_chats_from_db(CHATS_DB) < 0){
//        cerr << "Error during read chat" << endl;
//        return EXIT_FAILURE;
//    }
//    cout << "Chats read!" << endl;

    for (chat &c : g_chats) {
        cout << c.getUsers().front().getUsername();
        cout << c.getUsers().back().getUsername();
    }

    /* connection setup */

    // setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERVER_PORT);

    // create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("Error establishing the server socket: ");
        exit(EXIT_FAILURE);
    }


    // set socket option (reuse address)
    int reuse = 1;
    int err = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (err < 0){
        perror("Error establishing the server socket: ");
        exit(0);
    }

    // bind the socket to local address
    int bindStatus = bind(server_fd, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if(bindStatus < 0){
        perror("Error binding socket to local address: ");
        exit(EXIT_FAILURE);
    }

    cout << "Waiting for a client to connect..." << endl;
    // listen on socket
    if(listen(server_fd, MAX_CONN_QUEUE) < 0){
        perror("Error listen on server socket: ");
        exit(EXIT_FAILURE);
    }

    /* end connection setup */

    /* ready to accept new connection */

    // now server is ready for accept new connections from clients

    // create a thread for manage server with shell
    // only possible command - shutdown - that shuts down the server

    thread* cmd_thread = new thread(shell_routine);


    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(server_fd, &set); // add server descriptor on set
    // set timeout
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ret;

    while (running){
        if(LOG) cout << "[Info] Ready to select server fd" << endl;

        ret = select(MAX_CONN_QUEUE + 1, &set, NULL, NULL, &timeout);
        if(ret == -1) {
            // error occurred
            perror("Error during server select operation: ");
            exit(EXIT_FAILURE);
        }
        else if(ret == 0) {
            // select timeout occurred
            timeout.tv_sec = 5;
            FD_ZERO(&set); // clear set
            FD_SET(server_fd, &set); // add server descriptor on set

            // timeout occurred
            if(!running) cout << "[Info] Timeout occurred, closing server fd" << endl;
        }else{
            // connection incoming
            if(running){
                if(LOG) cout << "[Info] Ready to accept connection." << endl;

                int client_fd = accept(server_fd, NULL, NULL);
                if(client_fd < 0){
                    perror("Error accepting request from client: ");
                    exit(EXIT_FAILURE);
                }

                cout << "[Success] Accepted new connection from a client." << endl;

                // client connection helndler
                thread* client = new thread(connection_handler,client_fd);
                connection[client] = NULL;

                //connection.push_back(client);
                if(LOG) cout << "[Info] Pushed new thread in connection list" << endl;
            }
            else{
                cout << " Connection incoming during server close. " << endl;
                break;
            }

        }
    }


    if(LOG) cout << "[Shell] end_connection_routine" << endl;

    cmd_thread->join();


    cout << "Connection closed..." << endl;
    if(close(server_fd) < 0){ // this cause an error on accept
        perror("Error during close server socket");
        exit(EXIT_FAILURE);
    }

    cout << "Saving chats to database.." << endl;
    if(save_chats_to_db("chats.txt", g_active_chat) < 0){
        cerr << "Error during save chats" << endl;
        return EXIT_FAILURE;
    }
    cout << "Chats saved!" << endl;

    delete cmd_thread;


    /* local */
    for (user* &u : *g_active_users) delete u;
    delete g_active_users;

    for (chat* &c : *g_active_chat) delete c;
    delete g_active_chat;
    /* end local */

    // free thread
    for (const auto &pair : connection) delete pair.first;

    return EXIT_SUCCESS;
}


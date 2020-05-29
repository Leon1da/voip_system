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

using namespace std;
bool running = true;

void sender(int client_desc){

    bool openChat = false;
    while (running){
        string line;
        getline(cin,line);
        if(strcmp(line.c_str(),"exit") == 0){
            if(openChat) running = false;
            else cout << "Chiudi la chat con digitando return prima di uscire" << endl;
        }else if(strcmp(line.c_str(),"chat") == 0) openChat = true;

        send_msg(client_desc,line.c_str());
    }

    cout << "Sender ready for connection closed" << endl;


}

void receiver(int client_desc){

    // create a message buffer
    char msg_buf[MSG_SIZE];

    while (running){
        memset(msg_buf,0, MSG_SIZE);
        // ricevo messaggio di benvenuto
        int bytesRead = recv_msg(client_desc, msg_buf, MSG_SIZE);
        if(bytesRead > 0) cout << msg_buf << endl;

    }

    cout << "Reciver ready for connection closed" << endl;
}

//Client side
int main(int argc, char *argv[])
{
    //setup a socket and connection tools
    struct hostent* host = gethostbyname(SERVER_ADDRESS);
    sockaddr_in sendSockAddr;
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
    sendSockAddr.sin_family = AF_INET;
    sendSockAddr.sin_addr.s_addr =
            inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(SERVER_PORT);
    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSd < 0){
        cerr << "Error during socket create!" << endl;
    }

    //try to connect...
    int status = connect(clientSd,
                         (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    if(status < 0){
        cerr << "Error during connecting to socket!"<<endl;
    }
    cout << "Connected to the server!" << endl;


    thread* t_rec = new thread(receiver,clientSd);
    thread* t_send= new thread(sender,clientSd);
    t_send->join();
    t_rec->join();


    close(clientSd);
    cout << "Connection closed" << endl;

    return 0;
}

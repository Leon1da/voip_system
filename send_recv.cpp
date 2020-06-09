//
// Created by leonardo on 20/05/20.
//

#include <string.h>
#include <sys/socket.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include "utils.h"


void send_msg(msg p_msg){
    int dest_fd = p_msg.getDestinatario().getFD();
    const char *message = p_msg.getContent().c_str();

    send(dest_fd,message);
}


// sends the message contained in the buffer msg at socket descriptor socket
void send(int socket, const char *msg) {
    int ret;
    char msg_to_send[MSG_SIZE];
    sprintf(msg_to_send, "%s\n", msg);
    int bytes_left = strlen(msg_to_send);
    int bytes_sent = 0;
    while (bytes_left > 0) {
        ret = send(socket, msg_to_send + bytes_sent, bytes_left, 0);
        if (ret == -1 && errno == EINTR) continue;
        if(ret < 0){
            perror("Error during send_msg operation!");
            exit(EXIT_FAILURE);
        }

        bytes_left -= ret;
        bytes_sent += ret;
    }
}




// receive message from socket descriptor socket and save that on buffer buf of size buf_size
size_t recv(int socket, char *buf, size_t buf_len) {
    int ret;
    int bytes_read = 0;


    while (bytes_read <= buf_len) {
        ret = recv(socket, buf + bytes_read, 1, 0);

        if (ret == 0) return -1; // client close the socket
        if (ret == -1 && errno == EINTR) continue; // error during recv
        if(ret < 0){
            perror("Error during recv_msg operation!");
            exit(EXIT_FAILURE);
        }


        if (buf[bytes_read] == '\n') break; //end of message

        bytes_read++;
    }

    // replace last character of message ('\n') with string terminator ('\0')
    buf[bytes_read] = '\0';
    return bytes_read;
}

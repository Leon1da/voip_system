//
// Created by leonardo on 11/10/20.
//

#include "Common.h"

void signal_handler_init() {
    /* signal handler */
    struct sigaction sigIntHandler{};
    sigIntHandler.sa_handler = sigintHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0; //SA_RESTART
    sigaction(SIGINT, &sigIntHandler, nullptr);
    /* end signal handler */

}


/*
 * Wrapper for select operation
 * allow not blocking operation
 * ( as recvfrom, getline, etc..)
 */
int available(int fd, int sec, int usec){
    fd_set set;
    FD_ZERO(&set); // clear set
    FD_SET(fd, &set); // add server descriptor on set
    // set timeout
    timeval timeout{};
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    int ret = select( fd + 1, &set, nullptr, nullptr, &timeout);
    if(ret < 0) perror("available - select");
    return ret;
}



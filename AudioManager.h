//
// Created by leonardo on 28/09/20.
//

#ifndef CHAT_UDP_AUDIOMANAGER_H
#define CHAT_UDP_AUDIOMANAGER_H

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <string>

#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;


class AudioManager {

    // char *buffer;
    snd_pcm_t *handle;
    // long loops;
    int rc;
    int size;
    unsigned int val;
    snd_pcm_hw_params_t *params;
    int dir;
    snd_pcm_uframes_t frames;

public:
    // char *getBuffer() const;

    int getSize() const;

    AudioManager();

    int initAudioManager();

    int closeAudioManager();

    int readAudio(char *buffer);

    int writeAudio(char *buffer);





};


#endif //CHAT_UDP_AUDIOMANAGER_H

//
// Created by leonardo on 04/10/20.
//

#ifndef VOIP2_0_AUDIOMANAGER_H
#define VOIP2_0_AUDIOMANAGER_H


#include <alsa/asoundlib.h>
#include <iostream>
#include "Common.h"

using namespace std;

class AudioManager {

private:

    snd_pcm_t *handle{};
    snd_pcm_uframes_t frames{};

    int size{};
    char *buffer{};

public:
    void init_playback();

    void destroy_playback();

    void write();

    void read();

    int getSize() const;

    void setSize(int size);

    char *getBuffer() const;

    void setBuffer(char *buffer);

    void init_capture();

    void destroy_capture();
};


#endif //VOIP2_0_AUDIOMANAGER_H

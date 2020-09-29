//
// Created by leonardo on 28/09/20.
//

#include "AudioManager.h"

AudioManager::AudioManager() {};

int AudioManager::initAudioManager() {

    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&handle, "default",
                      SND_PCM_STREAM_CAPTURE, 0);

    if(rc < 0) {
        cout << "Unable to open pcm device " << snd_strerror(rc) << endl;
        exit(EXIT_FAILURE);
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params,
                                 SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, 2);

    /* 44100 bits/second sampling rate (CD quality) */
    val = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                    &val, &dir);

    /* Set period size to 32 frames. */
    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle,
                                           params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if(rc < 0){
        cout << "Unable to set hw parameters: " << snd_strerror(rc) << endl;
        exit(EXIT_FAILURE);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params,
                                      &frames, &dir);

    size = frames * 4; /* 2 bytes/sample, 2 channels */

    // buffer = (char *) malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params,
                                      &val, &dir);
    // loops = 5000000 / val;

    return 0;
}

int AudioManager::closeAudioManager() {

    cout << "Audio Manager closing.." << endl;

    if(snd_pcm_drain(handle) < 0){
        cout << "Error during snd_pcm_drain." << endl;
        exit(EXIT_FAILURE);
    }

    cout << " - snd_pcm_drain" << endl;

    if(snd_pcm_close(handle) < 0){
        cout << "Error during snd_pcm_close." << endl;
        exit(EXIT_FAILURE);
    }

    cout << " - snd_pcm_close" << endl;

    cout << "Audio Manager closed." << endl;

    // free(buffer);

    return 0;
}

int AudioManager::readAudio(char *buffer) {

    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
        /* EPIPE means overrun */
        cout << "Overrun occurred." << endl;
        snd_pcm_prepare(handle);
    } else if (rc < 0) {
        cout << "Error during readi: " << snd_strerror(rc) << endl;
    } else if (rc != (int)frames) {
        cout << "Short read, read " << rc << "frames." << endl;
    }

    return 0;
}

int AudioManager::writeAudio(char *buffer) {

    rc = snd_pcm_writei(handle, buffer, frames);
    if (rc == -EPIPE) {
        /* EPIPE means underrun */
        cout << "Overrun occurred." << endl;
        snd_pcm_prepare(handle);
    } else if (rc < 0) {
        cout << "Error from write: " << snd_strerror(rc);
    } else if (rc != (int)frames) {
        cout << "Short write, write " << rc << "frames." << endl;
    }
    return 0;
}


int AudioManager::getSize() const {
    return size;
}

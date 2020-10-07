//
// Created by leonardo on 04/10/20.
//

#include "AudioManager.h"


void AudioManager::init_capture() {

    /* Open PCM device. */
    int rc = snd_pcm_open(&handle, "default",
                      SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        cout << "unable to open pcm device: " << snd_strerror(rc) << endl;
        exit(EXIT_FAILURE);
    }

    if(snd_config_update_free_global() < 0){
        cout << "snd_config_update_free_global error." << endl;
        exit(EXIT_FAILURE);
    }


    snd_pcm_hw_params_t *params{};
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


    int dir = 0;
    /* 44100 bits/second sampling rate (CD quality) */
    unsigned int val = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                    &val, &dir);

    /* Set period size to 32 frames. */
    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle,
                                           params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        cout << "unable to set hw parameters: " << snd_strerror(rc) << endl;
        exit(EXIT_FAILURE);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params,
                                      &frames, &dir);
    size = frames * 4; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params,
                                      &val, &dir);


}

void AudioManager::init_playback() {

    /* Open PCM device. */
    int rc = snd_pcm_open(&handle, "default",
                      SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        cout << "unable to open pcm device: " << snd_strerror(rc) << endl;
        exit(EXIT_FAILURE);
    }

    if(snd_config_update_free_global() < 0){
        cout << "snd_config_update_free_global error." << endl;
        exit(EXIT_FAILURE);
    }


    snd_pcm_hw_params_t *params{};
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


    int dir = 0;
    /* 44100 bits/second sampling rate (CD quality) */
    unsigned int val = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params,
                                    &val, &dir);

    /* Set period size to 32 frames. */
    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle,
                                           params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        cout << "unable to set hw parameters: " << snd_strerror(rc) << endl;
        exit(EXIT_FAILURE);
    }


    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames,
                                      &dir);
    size = frames * 4; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params,
                                      &val, &dir);



}

void AudioManager::destroy_capture() {
//    cout << "snd_pcm_drain.." << endl;
//    if(snd_pcm_drain(handle) < 0){
//        cout << "snd_pcm_drain error" << endl;
//        exit(EXIT_FAILURE);
//    }
//    cout << "snd_pcm_drain ok" << endl;

    if(LOG) cout << "snd_pcm_close.." << endl;
    if(snd_pcm_close(handle) < 0){
        cout << "snd_pcm_close error" << endl;
        exit(EXIT_FAILURE);
    }
    if(LOG) cout << "snd_pcm_close ok" << endl;


    if(LOG) cout << "free buffer.." << endl;
    free(buffer);
    if(LOG) cout << "buffer freed" << endl;

    if(LOG) cout << "snd_config_update_free_global()" <<  endl;
    if(snd_config_update_free_global() < 0){
        cout << "snd_config_update_free_global error." << endl;
        exit(EXIT_FAILURE);
    }
    if(LOG) cout << "snd_config_update_free_global() ok." <<  endl;

}

void AudioManager::destroy_playback() {

    if(snd_pcm_drain(handle) < 0){
        cout << "snd_pcm_drain error" << endl;
        exit(EXIT_FAILURE);
    }

    if(snd_pcm_close(handle) < 0){
        cout << "snd_pcm_close error" << endl;
        exit(EXIT_FAILURE);
    }

    free(buffer);

    if(snd_config_update_free_global() < 0){
        cout << "snd_config_update_free_global error." << endl;
        exit(EXIT_FAILURE);
    }


}

void AudioManager::write() {

    int rc = snd_pcm_writei(handle, buffer, frames);
    if (rc == -EPIPE) {
        /* EPIPE means underrun */
        if(LOG) cout << "underrun occurred" << endl;
        snd_pcm_prepare(handle);
    } else if (rc < 0) {
        if(LOG) cout << "error from writei: " << snd_strerror(rc) << endl;
    }  else if (rc != (int)frames) {
        if(LOG) cout << "short write, write " << rc << "frames." << endl;
    }

}

int AudioManager::getSize() const {
    return size;
}

void AudioManager::setSize(int size) {
    AudioManager::size = size;
}

char *AudioManager::getBuffer() const {
    return buffer;
}

void AudioManager::setBuffer(char *buffer) {
    AudioManager::buffer = buffer;
}

void AudioManager::read() {

    int rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
        /* EPIPE means overrun */
        if(LOG) cout << "overrun occurred." << endl;
        snd_pcm_prepare(handle);
    } else if (rc < 0) {
        if(LOG) cout << "error from read: " << snd_strerror(rc) << endl;
    } else if (rc != (int)frames) {
        if(LOG) cout << "short read, read " << rc << "frames." << endl;
    }

}




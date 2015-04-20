#ifndef ALSACLS_H
#define ALSACLS_H

#include <stdio.h>
#include <inttypes.h>
#include <alsa/asoundlib.h>
#include <stdio.h>

#define PCM_DEVICE "default"

class Alsacls
{
public:
    Alsacls();
    ~Alsacls();

    int open(int rate, int channels, int seconds, int);
    void close();
    int playbuffer(uint8_t * pb, int len, int perc);
private:
    int  _init();

    unsigned int _rate;
    int _channels;
    int _seconds;
    int _pcm;
    snd_pcm_t           *_pcm_handle=nullptr;
    snd_pcm_hw_params_t* _params;
    snd_pcm_uframes_t   _frames = 0;
    unsigned int        _pertime;
    uint8_t             *_playbuff = nullptr;
    size_t              _round_time = 0;
    unsigned long       _playsz = 0;
    int                 _bytes = 0;
    int                 _udpsz = 0;
    int                 _room = 0;
    int                 _overshoot=0;
};

#endif // ALSACLS_H

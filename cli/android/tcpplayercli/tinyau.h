#ifdef WITH_TINY_ALSA
#ifndef TINYAU_H
#define TINYAU_H

#include <cstdint>
#include <tinyalsa/pcm.h>
#include <tinyalsa/asoundlib.h>

class AoCls
{
public:
    AoCls(int,int,int);
    ~AoCls();
    int init_ao(int);
    void play(const uint8_t* pb, int len, int ql);

private:
    struct pcm_config   _config;
    struct pcm*         _pcm = nullptr;

};

#endif // TINYAU_H
#endif //#ifdef WITH_TINY_ALSA

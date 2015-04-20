#ifdef WITH_SDL

#ifndef  TINYAUAO_H
#define  TINYAUAO_H

#include <thread>
#include <mutex>
#include <cstdint>
#include <SLES/OpenSLES.h>
#include "srvq.h"

#define MaiWaveOutI MaiWaveOutSLES

class AoCls
{
public:
    AoCls(int,int,int);
    ~AoCls();
    int init_ao(int);
    void play(const uint8_t* pb, int len);
    void stop();
private:
    SLObjectItf _psl;
    SLObjectItf _pout_mix;
    SLBufferQueueItf _buff_qitf;
    SLObjectItf _player;
    SLPlayItf _pplay_itf;


    int _stopped = 0;
    size_t _one_sec_buff=0;
    //create sbuf
    size_t _buffers;
    size_t _buff_sz = TCP_SND_LEN;
    uint8_t* sbuf;
    size_t _sbuff_count = 0;
    std::mutex _mut;
    int _bits_per_sample,_sample_rate,_channels;
};

#endif // TINYAU_H
#endif //#ifdef WITH_TINY_ALSA

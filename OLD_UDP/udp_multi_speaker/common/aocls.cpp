
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "aocls.h"
#include "screenxy.h"
#include "clithread.h"
#include "main.h"

#define OPTIM_AO_BUFF   70000
#define MIN_AO_LEN      48000
#define Q_OPTIM_AO_PLAY 50
#define AO_RESET_BUF    64

AoCls::AoCls(int bits, int rate, int channels, int netplaysz)
{
    _format.bits = bits;
    _format.rate = rate;
    _format.channels = channels;
    _format.byte_format = AO_FMT_NATIVE;
    _format.matrix = 0;
    ::ao_initialize();

    int driver = ::ao_default_driver_id();
    _dev = ao_open_live(driver, &_format, NULL);
    if(_dev==nullptr)
    {
        perror("ao_open(): trying alsa");
        driver = ::ao_driver_id("alsa");
        _dev = ::ao_open_live(driver, &_format, NULL);
    }
    if(_dev==nullptr)
    {
        perror("ao_open(): ");
    }
    int a64k = OPTIM_AO_BUFF / netplaysz;
    _ibuffsz=_buffsz = a64k * netplaysz;
    //_playbuff = new uint8_t[_buffsz];
    //_bytes = 0;
    //_room = _buffsz;
}

AoCls::~AoCls()
{
    if(_dev)
        ::ao_close(_dev);
    ::ao_shutdown();
    delete []_playbuff;
}


int AoCls::play(const uint8_t* buff, size_t sz, const Qbuff* qb, bool pilot)
{
    int ret = 0;
    size_t rt = tick_count();
    size_t rtp = rt - _round_time;
    _round_time = rt;

    if(_playbuff==nullptr)
    {
        if(_dev)
        {
            if(!pilot && qb)
            {
                int delta = qb->optim()-qb->length();
                if(delta < -MIN_SKEW)           // we play to fast
                {
                    TERM_OUT(true,MSG_L3,"SLOWING....");
                    ::ao_play(_dev, (char*)buff, sz);
                    ::usleep(512);             //not audible
                }
                else if(delta > MIN_SKEW)      // we play to slow
                {
                    TERM_OUT(true,MSG_L3,"SPEEDING....");
                    ::ao_play(_dev, (char*)buff, sz-16);  //not audible
                }
                else {
                    ::ao_play(_dev, (char*)buff, sz);
                    TERM_OUT(true,MSG_L3,"ao-len: %04d ms  rt:%04d ms        ",tick_count()-rt, rtp);
                }
            }
            else{
                ::ao_play(_dev, (char*)buff, sz);
                TERM_OUT(true,MSG_L3,"ao-len: %04d ms  rt:%04d ms        ",tick_count()-rt, rtp);
                ::usleep(1000);
            }
        }
    }
#if 0
    else {
        if(_room>0)
        {
            ::memcpy(_playbuff+_bytes,buff,sz);
            _bytes += sz;
            _room -= sz;
        }
        else
        {
            if(_dev)
            {
                ::ao_play(_dev, (char*)_playbuff, _bytes);
                ::memcpy(_playbuff,buff,sz);
                _bytes = sz;
                _room = _buffsz-sz;

                TERM_OUT(true,MSG_L3,"ao-len: %04d ms  round-trip:%04d ms",tick_count()-rt, rtp);
            }
        }
    }
#endif // 0
    return ret;
}


/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/
#ifndef PLATFORM_ANDROID
#include <cerrno>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "aop.h"
#include "srvq.h"
#include "screenxy.h"
#include "main.h"



AoCls::AoCls(int bits, int rate, int channels)
{
    _format.bits        = bits;
    _format.rate        = rate;
    _format.channels    = channels;
    _format.byte_format = AO_FMT_NATIVE;
    _format.matrix      = nullptr;
    ::ao_initialize();

    int driver = ::ao_default_driver_id();
    _dev = ao_open_live(driver, &_format, nullptr);
    if(_dev==nullptr)
    {
        const char* drivers[]={"oss","wav","raw","au","sndio",nullptr};
        for(const char** pd = drivers; *pd!=nullptr && _dev==nullptr;*pd++)
        {
            TERM_OUT(true,ERR_L,"error:%d ao_open(): trying alsa", errno);
            driver = ::ao_driver_id(*pd);
            _dev = ::ao_open_live(driver, &_format, nullptr);
        }
    }
    if(_dev==nullptr)
    {
        TERM_OUT(true,ERR_L,"error:%d ao_open()" , errno);
        return;
    }
    // buffer len for 1 sec
    _one_sec_buff = (_format.bits * _format.rate * _format.channels) / 8; // per 1 second
    _scrap_buff = _one_sec_buff/200;
    _one10ms = new uint_8[_scrap_buff];
}

AoCls::~AoCls()
{
    delete [] _one10ms;
    if(_dev)
        ::ao_close(_dev);
    ::ao_shutdown();
}

int AoCls::play(uint8_t* buff, size_t sz)
{
    if(_dev)
    {
        ::ao_play(_dev, (char*)buff, sz);
    }
    return 0;
}

#endif //WITH_AO_LIB

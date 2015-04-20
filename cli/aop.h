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

#ifndef AOCLS_H
#define AOCLS_H

#include <cstdint>
#include <ao/ao.h>


#define CACHE_PLAY_BUFF 0xFFFF

class AoCls
{
public:
    AoCls(int bits, int rate, int channels);
    ~AoCls();
    int     play(uint8_t* buff, size_t sz);
    size_t  init_ao(int time)const{
        return _one_sec_buff*time;
    }
private:
    ao_device*       _dev = nullptr;
    ao_sample_format _format{};
    size_t           _one_sec_buff = 0;
    size_t           _scrap_buff = 0;
    uint8_t*         _one10ms;
};

#endif // AOCLS_H
#endif //WITH_AO_LIB

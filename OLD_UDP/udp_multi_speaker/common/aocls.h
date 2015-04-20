#ifndef AOCLS_H
#define AOCLS_H

#include <ao/ao.h>
class Qbuff;
class AoCls
{
public:
    AoCls(int bits, int rate, int channels, int netplaysz);
    ~AoCls();
    int play(const uint8_t* buff, size_t sz, const Qbuff* qb=nullptr, bool pilot=false);

private:

    ao_device*       _dev = nullptr;
    ao_sample_format _format;
    uint8_t*         _playbuff=nullptr;
    int              _bytes = 0;
    int              _buffsz = 0;
    int              _ibuffsz = 0;
    int              _reset = 0;
    int              _room = 0;
    size_t           _round_time = 0;
};

#endif // AOCLS_H

#ifndef PULSECLS_H
#define PULSECLS_H

#include <pulse/simple.h>

class pulsecls
{
public:
    pulsecls(int bits, int rate, int channels, int netplaysz);
    ~pulsecls();

     int play(uint8_t* buff, size_t sz, int percent);
private:
    pa_sample_spec    _pas;
    pa_simple         *_device;
};

#endif // PULSECLS_H

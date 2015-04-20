

#include "pulsecls.h"

pulsecls::pulsecls(int bits, int rate, int channels, int netplaysz)
{
    (void)(bits);
    (void)(netplaysz);
    _pas.rate=rate;
    _pas.format=PA_SAMPLE_S16LE;
    _pas.channels=channels;
    _device = pa_simple_new(NULL, NULL, PA_STREAM_PLAYBACK, NULL,
                "playback", &_pas, NULL, NULL, NULL);

}

int pulsecls::play(uint8_t* buff, size_t sz, int percent)
{
    pa_simple_write(_device, buff, sz, NULL);
}

pulsecls::~pulsecls()
{
    pa_simple_drain(_device, NULL);
    pa_simple_free(_device);
}

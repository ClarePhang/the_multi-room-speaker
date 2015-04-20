
#include <iostream>
#include "alsacls.h"
#include "os.h"
#include "screenxy.h"

Alsacls::Alsacls():_buff(nullptr)
{

}

Alsacls::~Alsacls()
{
    close();
}

void Alsacls::close()
{
    snd_pcm_drain(_pcm_handle);
    snd_pcm_close(_pcm_handle);
    delete[] _buff;
    _buff = nullptr;
}

int Alsacls::open(int rate, int channels, int seconds, int udbp)
{
    _rate =(rate);
    _channels=(channels);
    _seconds=(seconds);
    _udpsz=udbp;
    if((_pcm = snd_pcm_open(&_pcm_handle, PCM_DEVICE,
                            SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        printf("ERROR: Can't open \"%s\" PCM device. %s\n",
               PCM_DEVICE, snd_strerror(_pcm));
        return 0;
    }
    return _init();
}

int Alsacls::_init()
{
    unsigned int tmp, rate=_rate;
    int err, channels=_channels;
    snd_pcm_hw_params_t* params;

    snd_pcm_hw_params_alloca(&params);

    if ((err = snd_pcm_hw_params_any(_pcm_handle, params)) < 0)
        printf("Can't fill params: %d" , err);

    /* Set parameters */
    if ((err = snd_pcm_hw_params_set_access(_pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        printf("Can't set interleaved mode: %d" , err);

    snd_pcm_format_t snd_pcm_format = SND_PCM_FORMAT_S16_LE;

    err = snd_pcm_hw_params_set_format(_pcm_handle, params, snd_pcm_format);

    if ((err = snd_pcm_hw_params_set_channels(_pcm_handle, params, channels)) < 0)
        printf("Can't set channel count: %d" , err);

    if ((err = snd_pcm_hw_params_set_rate_near(_pcm_handle, params, &rate, nullptr)) < 0)
        printf("Can't set rate: %d" , err);

    unsigned int period_time;
    snd_pcm_hw_params_get_period_time_max(params, &period_time, nullptr);
    if (period_time > 65000)
        period_time = 65000;

    unsigned int buffer_time = 4 * period_time;

    snd_pcm_hw_params_set_period_time_near(_pcm_handle, params, &period_time, nullptr);
    snd_pcm_hw_params_set_buffer_time_near(_pcm_handle, params, &buffer_time, nullptr);

    if ((err = snd_pcm_hw_params(_pcm_handle, params)) < 0)
        printf("Can't set hardware parameters: %d" , err);

    /* Resume information */
    std::cout << "PCM name: " << snd_pcm_name(_pcm_handle) << "\n";
    std::cout << "PCM state: " << snd_pcm_state_name(snd_pcm_state(_pcm_handle)) << "\n";
    snd_pcm_hw_params_get_channels(params, &tmp);
    std::cout << "channels: " << tmp << "\n";

    snd_pcm_hw_params_get_rate(params, &tmp, nullptr);
    std::cout << "rate: " << tmp << " bps\n";

    /* Allocate buffer to hold single period */
    snd_pcm_hw_params_get_period_size(params, &_frames, nullptr);
    std::cout << "frames: " << _frames << "\n";

    snd_pcm_hw_params_get_period_time(params, &tmp, nullptr);
    std::cout << "period time: " << tmp << "\n";

    snd_pcm_sw_params_t* swparams;
    snd_pcm_sw_params_alloca(&swparams);
    snd_pcm_sw_params_current(_pcm_handle, swparams);

    snd_pcm_sw_params_set_avail_min(_pcm_handle, swparams, _frames);
    snd_pcm_sw_params_set_start_threshold(_pcm_handle, swparams, _frames);
    //	snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, frames_);
    snd_pcm_sw_params(_pcm_handle, swparams);

    snd_pcm_uframes_t bufsz = 0;
    snd_pcm_hw_params_get_buffer_size(params,&_ibuffsz);

    //snd_pcm_sw_params_free(swparams);
    //snd_pcm_hw_params_free(params);

    _buff = new uint8_t[_ibuffsz];
    _nbuf = 0;
    _room = _ibuffsz;
    return 1;
}

int Alsacls::playbuffer(uint8_t * pb, int sz)
{
    int ret = 0;
    if(_udpsz != sz)
    {
        return -1;
    }
    snd_pcm_sframes_t fr_delay;
    snd_pcm_sframes_t fr_avail;
    int result = snd_pcm_avail_delay(_pcm_handle, &fr_avail, &fr_delay);
    if(_buff==0)
    {
        if ((_pcm = snd_pcm_writei(_pcm_handle, pb, fr_avail)) == -EPIPE)
        {
            snd_pcm_prepare(_pcm_handle);
        }
    }
    else
    {
        if(_room >= sz)
        {
            ::memcpy(_buff+_nbuf,pb,sz);
            _nbuf += sz;
            _room -= sz;
        }
        else
        {
            size_t rt  = tick_count();
            size_t rtp = rt - _round_time;
            _round_time = rt;

            if ((_pcm = snd_pcm_writei(_pcm_handle, _buff, fr_avail)) == -EPIPE)
            {
                snd_pcm_prepare(_pcm_handle);
            }
            Term->pc(true,0,MSG_L3,"ao-len: %04d ms  rt:%04d ms         ",tick_count()-rt, rtp);
            ret = 1;
            ::memcpy(_buff,pb,sz);
            _nbuf = sz;
            _room = _ibuffsz-sz;

        }
    }

    if (_pcm < 0)
    {
        ::usleep(0xFF);
        fr_avail = snd_pcm_avail(_pcm_handle);
        result = snd_pcm_delay(_pcm_handle, &fr_delay);
        if ((result < 0) || (fr_avail <= 0) || (fr_delay <= 0))
        {
            ::usleep(0x2000);
            snd_pcm_prepare(_pcm_handle);
        }
    }
    if (fr_avail < static_cast<snd_pcm_sframes_t>(_frames))
    {
        ::usleep(0xFFF);
    }
    return ret; // -1 discard, 0 ok, 1 replay
}

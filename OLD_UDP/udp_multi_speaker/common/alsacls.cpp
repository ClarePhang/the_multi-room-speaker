#include <iostream>
#include "alsacls.h"
#include "screenxy.h"
#include "sock.h"
#include "main.h"

Alsacls::Alsacls()
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
    //delete[] _playbuff;
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

    snd_pcm_hw_params_get_buffer_size(params,&_playsz);

    //snd_pcm_sw_params_free(swparams);
    //snd_pcm_hw_params_free(params);

    _playbuff = new uint8_t[_playsz];
    _bytes = 0;
    _room = _playsz;
    return 1;
}


int Alsacls::open(int rate, int channels, int seconds, int udb)
{
    _rate =(rate);
    _channels=(channels);
    _seconds=(seconds);
    _udpsz = udb;

    if((_pcm = snd_pcm_open(&_pcm_handle, PCM_DEVICE,
                            SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        printf("ERROR: Can't open \"%s\" PCM device. %s\n",
               PCM_DEVICE, snd_strerror(_pcm));
        return 0;
    }

    return _init();
    snd_pcm_hw_params_alloca(&_params);
    snd_pcm_hw_params_any(_pcm_handle, _params);

    /* Set parameters */
    if ((_pcm = snd_pcm_hw_params_set_access(_pcm_handle, _params,
                                             SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(_pcm));

    if ((_pcm = snd_pcm_hw_params_set_format(_pcm_handle, _params,
                                             SND_PCM_FORMAT_S16_LE)) < 0)
        printf("ERROR: Can't set format. %s\n", snd_strerror(_pcm));

    if ((_pcm = snd_pcm_hw_params_set_channels(_pcm_handle, _params, _channels)) < 0)
        printf("ERROR: Can't set channels number. %s\n", snd_strerror(_pcm));

    if ((_pcm = snd_pcm_hw_params_set_rate_near(_pcm_handle, _params, &_rate, 0)) < 0)
        printf("ERROR: Can't set rate. %s\n", snd_strerror(_pcm));

    /* Write parameters */
    if ((_pcm = snd_pcm_hw_params(_pcm_handle, _params)) < 0)
        printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(_pcm));

    /* Resume information */
    printf("PCM name: '%s'\n", snd_pcm_name(_pcm_handle));

    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(_pcm_handle)));
    unsigned int tmp;
    snd_pcm_hw_params_get_channels(_params, &tmp);
    printf("channels: %i ", tmp);

    if (tmp == 1)
        printf("(mono)\n");
    else if (tmp == 2)
        printf("(stereo)\n");

    snd_pcm_hw_params_get_rate(_params, &tmp, 0);
    printf("rate: %d bps\n", tmp);

    printf("seconds: %d\n", _seconds);

    /* Allocate buffer to hold single period */

    snd_pcm_hw_params_get_period_size(_params, &_frames, 0);

    int tmpx = snd_pcm_hw_params_get_period_time(_params, &_pertime, NULL);
    UNUS(tmpx);
    int buff_size = _frames * _channels * sizeof(uint16_t);
    _playbuff = new uint8_t[buff_size + _udpsz];
    _playsz = _room = buff_size;
    _udpsz = udb;
    _bytes = 0;

    return buff_size;
}

int Alsacls::playbuffer(uint8_t * pb, int len, int perc)
{
    UNUS(perc);
    if(len!=_udpsz)
    {
        return -1;
    }
    int minb = 0;
    snd_pcm_sframes_t fr_delay=0;
    snd_pcm_sframes_t fr_avail=0;
    if(_playbuff)
    {
        if(_room>0)
        {
            minb = std::min(_room,len);
            ::memcpy(_playbuff+_bytes, pb, minb);
            _room -= minb;
            _bytes += minb;
            _overshoot = len-minb;
        }
        else
        {
            int result = snd_pcm_avail_delay(_pcm_handle, &fr_avail, &fr_delay);
            UNUS(result);
            size_t rt = tick_count();
            size_t rtp = rt - _round_time;
            _round_time = rt;
            if(int(fr_avail) >int(_frames))
                fr_avail = _frames;

            if ((_pcm = snd_pcm_writei(_pcm_handle, _playbuff, fr_avail)) == -EPIPE)
            {
                snd_pcm_prepare(_pcm_handle);
            }
            else if (_pcm < 0)
            {
                printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(_pcm));
            }
            TERM_OUT(true,MSG_L3,"ao-len: %04d ms  roun-dtrip:%04d ms  sz=%d      ",
                            tick_count()-rt, rtp, _playsz);

            _bytes = 0;
            _room = _playsz;
            if(_overshoot)
            {
                ::memcpy(_playbuff,_playbuff+_bytes,_overshoot);
                _bytes = _overshoot;
                _room-=_overshoot;
            }
            ::memcpy(_playbuff+_bytes, pb, len);
            _room -= len;
            _bytes += len;
        }
    }
    else {

        int result = snd_pcm_avail_delay(_pcm_handle, &fr_avail, &fr_delay);
        UNUS(result);
        if ((_pcm = snd_pcm_writei(_pcm_handle, pb, 2048)) == -EPIPE)
        {
            snd_pcm_prepare(_pcm_handle);
        }
        else if (_pcm < 0)
        {
            printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(_pcm));
        }
    }

    if (_pcm < 0)
    {
        ::usleep(0xFF);
        fr_avail = snd_pcm_avail(_pcm_handle);
        int result = snd_pcm_delay(_pcm_handle, &fr_delay);
        if ((result < 0) || (fr_avail <= 0) || (fr_delay <= 0))
        {
            ::usleep(0x1000);
            snd_pcm_prepare(_pcm_handle);
        }
    }
    if (fr_avail < static_cast<snd_pcm_sframes_t>(_frames))
    {
        ::usleep(0x2000);
        //::usleep(_pertime-256);
    }
   //
    return 0;
}

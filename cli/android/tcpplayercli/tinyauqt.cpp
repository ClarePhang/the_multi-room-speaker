#ifdef WITH_QT_SOUND
#include <cstdint>
#include <unistd.h>
#include "tinyauqt.h"
#include <QAudioDeviceInfo>
#include <QDebug>
#include <QSound>



#define ALSA_FRAMES 32


//int,int,int
AoCls::AoCls(int bits,int sample,int channels)
{
    _format.setChannelCount(channels);
    _format.setSampleRate(sample);
    _format.setSampleSize(bits);
    _format.setCodec("audio/pcm");
    _format.setByteOrder(QAudioFormat::LittleEndian);
    _format.setSampleType(QAudioFormat::SignedInt);
    _one_sec_buff = (bits * sample * channels) / 8; // per 1 second
    _scrap_buff = _one_sec_buff/200;
    _one10ms = new uint8_t[_scrap_buff];
    _qba = new QByteArray(4096,0);
    _qbuf = new QBuffer(_qba);
}

AoCls::~AoCls()
{
    _aout->stop();
    delete _one10ms;
    delete _qba;
    delete _qbuf;
    delete _aout;
}

int AoCls::init_ao(int time)
{
    _aout = new QAudioOutput(_format);
    _aout->setBufferSize(4096*2);
    return _one_sec_buff*time;
}

void AoCls::play(const uint8_t* pb, int len, int ql)
{
    if(len)
    {
        _qba->clear();
        _qba->append((const char*)pb,len);
        _aout->start(_qbuf);
    }
}

#endif //#ifdef WITH_QT_SOUND

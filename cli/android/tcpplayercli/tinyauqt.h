
#ifdef WITH_QT_SOUND

#ifndef TINYAUQT_H
#define TINYAUQT_H

#include <cstdint>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>

class AoCls
{
public:
    AoCls(int,int,int);
    ~AoCls();
    int init_ao(int);
    void play(const uint8_t* pb, int len, int ql);


private:
    QAudioFormat    _format;
    QAudioOutput*   _aout;
    //QMediaPlayer    _player;
    QByteArray*     _qba;
    QBuffer*        _qbuf;
    int             _one_sec_buff;
    int             _scrap_buff;
    uint8_t*        _one10ms;
};

#endif // TINYAUQT_H
#endif //#ifdef WITH_QT_SOUND

#ifndef MP3CLS_H
#define MP3CLS_H

#include <stdint.h>
#include <mpg123.h>

class Mp3Cls
{
public:
    Mp3Cls();
    ~Mp3Cls();


    bool open(const char* file);
    bool close();
    ulong getframe()const;
    void setframe(ulong sign);
    size_t read(uint8_t* buff, size_t maxlen);

    size_t openFeed(const uint8_t* in, size_t len );
    bool feed(const uint8_t* in, size_t len );

public:
    int             _frmlen=0;
    int             _bits=0;
    long int        _rate=0;
    int             _channels=0;

private:
    mpg123_handle*  _mh = nullptr;
    int             _err=0;

};

#endif // MP3CLS_H

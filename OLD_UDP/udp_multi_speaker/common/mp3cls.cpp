#include <iostream>
#include "mp3cls.h"

#define BITS 8

Mp3Cls::Mp3Cls()
{
    ::mpg123_init();
}

Mp3Cls::~Mp3Cls()
{
    close();
    ::mpg123_exit();
}

void Mp3Cls::setframe(ulong sign)
{
    if(_mh)
        ::mpg123_seek_frame(_mh, sign, SEEK_SET);
}

ulong Mp3Cls::getframe()const
{
    if(_mh)
        return  ::mpg123_tellframe(_mh);
    return 0;
}

size_t Mp3Cls::openFeed(const uint8_t* in, size_t len )
{
    if(MPG123_OK == mpg123_feed(_mh, in, len))
    {
        int encoding;
        if(mpg123_getformat(_mh, &_rate, &_channels, &encoding) == MPG123_OK)
        {
            return mpg123_outblock(_mh);
        }
    }
    return 0;
}

bool Mp3Cls::feed(const uint8_t* in, size_t len )
{
    (void)in; (void)len;
    return false;
}


bool Mp3Cls::open(const char* file)
{
    int encoding;
    if(_mh)
        close();
    _mh = ::mpg123_new(NULL, &_err);
    if(_mh==0)
    {
        std::cout << __FUNCTION__ << " error: " << _err << std::endl;
        return false;
    }
    _frmlen = ::mpg123_outblock(_mh);
    if(file)
    {
        _err = ::mpg123_open(_mh, file);
        if(-1==_err)
        {
            std::cout << __FUNCTION__ << " error: " << _err << std::endl;
            return false;
        }

        ::mpg123_getformat(_mh, &_rate, &_channels, &encoding);
        _bits = ::mpg123_encsize(encoding) * BITS;
        return _bits>0;
    }
    return mpg123_open_feed(_mh)==MPG123_OK;
}

bool Mp3Cls::close()
{
    if(_mh){
        ::mpg123_close(_mh);
        ::mpg123_delete(_mh);
        _mh = 0;
    }
    return true;
}

size_t Mp3Cls::read(uint8_t* buff, size_t maxlen)
{
    size_t chunk = 0;
    if(_mh)
    {
        _err = ::mpg123_read(_mh, buff, maxlen, &chunk);
    }
    return chunk;
}




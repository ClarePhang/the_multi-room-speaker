/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/

#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "pipein.h"
#include "screenxy.h"

CPipeIn::CPipeIn(const char* file)
{
   // ::umask(0x777);
    if(::access(file,0)==0)
    {
        ::unlink(file);
        ::sync();
        ::sleep(3);
    }
    if(::access(file,0)!=0)
    {
        int fi = ::mkfifo(file, O_RDWR|O_NONBLOCK| S_IRWXU|S_IRWXG|S_IRWXG  );
        if(fi<0)
        {
            TERM_OUT(true,ERR_L,"Error %s %d: %d, %s",__FILE__,__LINE__, errno, strerror(errno));
            return;
        }
        _own=true;
    }
    _fd = ::open (file, O_RDWR);
    if(_fd<0)
    {
        TERM_OUT(true,ERR_L,"Error %s %d: %d, %s",__FILE__,__LINE__, errno, strerror(errno));
    }
    _fn = file;
    FD_ZERO(&_rd);
    FD_SET(_fd, &_rd);
}


CPipeIn::~CPipeIn()
{
    if(_fd)
        ::close(_fd);
    if(_own)
        ::unlink(_fn.c_str());
}

int CPipeIn::peek(uint8_t* buff, int maxsz)
{
    timeval  tv   = {0, 0x1FF};
    int nfds = (int)_fd+1;
    FD_ZERO(&_rd);
    FD_SET(_fd, &_rd);
    int sel = ::select(nfds, &_rd, 0, 0, &tv);

    if(sel < 0)
    {
        TERM_OUT(true,ERR_L,"Error %s %d: %d, %s",__FILE__,__LINE__, errno, strerror(errno));
        return -1;
    }
    if(sel > 0 && FD_ISSET(_fd, &_rd))
    {
        maxsz = ::read(_fd, buff, maxsz);
        return maxsz;
    }
    return 0;
}


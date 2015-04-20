
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cpipein.h"

CPipeIn::CPipeIn(const char* file)
{
   // ::umask(0x777);
    if(::access(file,0)==0)
    {
        //check if the file is fifo
        ::unlink(file);
        /*
        struct stat st;
        stat(file, &st);
        if(!S_ISFIFO(st.st_mode))
        {
            std::cout << "file is not a fifo \n";
            ::unlink(file);
        }
        */
    }
    if(::access(file,0)!=0)
    {
        int fi = ::mkfifo(file, O_RDWR|O_NONBLOCK| S_IRWXU|S_IRWXG|S_IRWXG  );
        if(fi<0)
        {
            perror("mkfifo");
            return;
        }
        _own=true;
    }
    _fd = ::open (file, O_RDWR);
    if(_fd<0)
    {
        perror("file");
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
    timeval  tv   = {0, 1024};

    int nfds = (int)_fd+1;
    FD_ZERO(&_rd);
    FD_SET(_fd, &_rd);

    int sel = ::select(nfds, &_rd, 0, 0, &tv);

    if(sel < 0)
    {
        perror("select");
        return -1;
    }
    if(sel > 0 && FD_ISSET(_fd, &_rd))
    {
        maxsz = ::read(_fd, buff, maxsz);
        //std::cout << "got from pipe " << maxsz<< "\n";
        FD_CLR(_fd, &_rd);
        return maxsz;
    }
    return 0;
}


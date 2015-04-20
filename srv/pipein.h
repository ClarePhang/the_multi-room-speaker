/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/


#ifndef CPIPEIN_H
#define CPIPEIN_H

#include <string>
#include <stdarg.h>
#include <curses.h>

class CPipeIn
{
public:
    CPipeIn(const char* file);
    ~CPipeIn();

    int peek(uint8_t* buff, int maxsz);
    bool ok()const{return _fd>0;}
private:
    std::string _fn;
    int         _fd=0;
    bool        _own=false;
    fd_set      _rd;
};



#endif // CPIPEIN_H

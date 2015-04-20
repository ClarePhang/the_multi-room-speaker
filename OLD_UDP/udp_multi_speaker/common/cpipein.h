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

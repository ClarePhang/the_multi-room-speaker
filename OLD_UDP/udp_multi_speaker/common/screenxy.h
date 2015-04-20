/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/
#ifndef SCREENXY_H
#define SCREENXY_H

#ifndef TARGET_ANDROID
#include <curses.h>
#endif
#include <stdlib.h>
#include <cstring>

#define WIN_GL_W  1200
#define GRAPH_SAMPLES  1200
#define FRM_SCALE 5
#define VER_L  0
#define MAIN_L 1
#define QUE_L  2
#define CON_L  3
#define ERR_L  16
#define MSG_L  5
#define QOS_L  6
#define MSG_L1  7
#define MSG_L2  8
#define MSG_L3  9
#define MSG_L4  10
#define MSG_L5  11
#define CLI_L  13
#define CLI_H  12

#include <string>
#include <deque>
#include <mutex>

#ifdef TARGET_ANDROID

#define TERM_OUT(...)

#else
class screenxy
{
public:
    screenxy(int argc, char* argv[]);
    ~screenxy();
    void pc(bool force,  int line, const char *format, ...);
    void add(__useconds_t s, int diff);
    void swap();
private:
    WINDOW* _pw;
    std::mutex  _m;
#ifdef GL_DISPLAY
    std::string _texts[64];
    std::deque<__useconds_t>  _delays;
    std::deque<int>           _diff;
    float                     _max = 0;
#endif
};



inline void gbits(char* out, int x)
{
    int z;
    for (z = 128; z > 0; z >>= 1) {
        strcat(out, ((x & z) == z) ? "1" : "0");
    }

}
#define TERM_OUT Term->pc
extern screenxy* Term;
#endif //#ifndef !TARGET_ANDROID



#endif // SCREENXY_H

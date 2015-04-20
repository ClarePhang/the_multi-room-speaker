/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/
#ifndef PLAYER_H
#define PLAYER_H

#include <time.h>
#include "srvq.h"
#ifdef TARGET_ANDROID
#include "tinyauan.h"
#else 
#include "aop.h"
#endif

#define Q_LEN_MS    1000
#define INIT_Q_LEN  20
#define TWO_SEC     2000

class Circle;
class sclient;
class AoCls;
#ifdef TARGET_ANDROID
class Dialog;
#endif
class player
{
    enum player_t{V_SLOW,SLOW,NORMAL,FAST};
public:
    player(Circle* _rq, sclient* c);
    ~player();
#ifdef TARGET_ANDROID
    bool play(sclient& client,Dialog* pd);
#else
    bool play(sclient& client);
#endif
    void set_qlen(int len){_thisqlen=len;}
    size_t get_len_for(int)const;
    void start();

private:
    size_t _pid(int diff, int percent);
    size_t  _fill_q(bool& filling);
    void _play(Circle::Chunk*  pk);
    void _display(size_t curbps,    // bps
                          size_t curps,     // play speed time
                          size_t qoptim,    // optim q
                          int qerc,         // fill %
                          int qskew,        // where is q
                          const BufHdr& h);
private:
    int      _thisqlen = INIT_Q_LEN;
    Circle*  _rq = nullptr;
    sclient* _pc = nullptr;
    AoCls*   _ao = nullptr;
    player_t _player = NORMAL;
#ifdef TARGET_ANDROID
    Dialog*  _pd;
#endif
};

#endif // PLAYER_H


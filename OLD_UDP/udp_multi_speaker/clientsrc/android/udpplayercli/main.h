#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <mutex>


#define MSLEEP(k)   ::usleep(k*1000)

extern bool __alive;
inline uint64_t tick_count(int nanos=1000000)
{
    static struct timespec ts;

    if(clock_gettime(CLOCK_MONOTONIC,&ts) != 0) {
        assert(0);
        return ts.tv_sec*1000+ts.tv_nsec/nanos;
    }
    return ts.tv_sec*1000+ts.tv_nsec/nanos;
}

class Qbuff;
class Dialog;

extern bool __alive;
extern Dialog* PCTX ;


#endif

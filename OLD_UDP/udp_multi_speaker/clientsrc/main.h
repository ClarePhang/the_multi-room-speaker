#ifndef _MAIN_HD_
#define _MAIN_HD_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <iostream>   /* For open(), creat() */
#include <signal.h>   /* For open(), creat() */
#include <mutex>
#include "sock.h"
#include "aoutil.h"
#include "alsacls.h"
#include "clithread.h"
#include "aocls.h"
#include "mp3cls.h"
#include "cpipein.h"


//#define SIMUL
#define _VERSION     "0.0.1"

#define ALMOST_EMPTY 10
#define BITS         8
#define PIPE_IN      "/tmp/speakers"

#define BITS_AO     16
#define SAMPLE_AO   44100
#define CHANNELS_AO 2

extern bool __alive;

class RunCtx
{
public:
    RunCtx(RunCtx** p);
    ~RunCtx();
    void client();
    void server(int argc, char* argv[]);
    int  cli_frame()const{
        std::lock_guard<std::mutex> guard(_mut);
        return _frame;
    }
    virtual void thread_main(){};

private:
    size_t _fill_q(Qbuff& qb, bool& filling);

private:
    int _frame = 0;
    mutable std::mutex  _mut;
};

#define MSLEEP(k)   ::usleep(k*1000)

extern RunCtx* PCTX;
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

#endif //_MAIN_H_

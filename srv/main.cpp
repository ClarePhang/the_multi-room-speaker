/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/


#include <string>
#include <iostream>
#include <thread>
#include "udpdisc.h"
#include "tcpsrv.h"
#include "pipein.h"
#include "main.h"
#include "screenxy.h"
#include "pidaim.h"

bool __alive;
int main(int argc, char* argv[])
{
    UNUS(argv);
    UNUS(argc);
    screenxy        t(argc, argv);                  // inits the Term*
    CPipeIn         pipe(PIPE_IN);      // stream in file pipe where we cat mplayer sound.mpg >>
    TcpSrv          srv;                // the server
    int             frmdiff = 0;   // clients needs data

    __alive = true;
    if(pipe.ok() && srv.listen())
    {
        uint32_t        thisip    = (uint32_t)inet_addr(sock::GetLocalIP("127.0.0.1"));
        UNUS(thisip);
        uint64_t           t1,t2,t3=0,rt;   // timer
        std::thread     beacon (Udper::tmain);  // beacon for clients to discover this IP
        uint64_t        sleep_opt = 1000*((1000*TCP_SND_LEN) / (SAMPLE_AO * CHANNELS_AO * (BITS_AO/8)));
        int             frm = 0;            // frm
        int             lastbps=0,bps = 0;      // counters
        double          foptim;
        size_t          buf_len=0;
        uint64_t        slploop = sleep_opt;//P I D
        double          xpidv=0.0,pidv = 0.0,curperc;
        __useconds_t    us;
        int16_t         ao_rate  = int16_t(SAMPLE_AO);
        Circle::Chunk   _chunk(TCP_SND_LEN);
        // PID             pid(0.04, 99.0, 1, 0.2, 0.3, 0.003);
        double T=double(sleep_opt)/1000000.0;
        double P=0.20;
        double I=0.30;
        double D=0.001;

        PID             pid(T,
                            99.0, 1.0,
                            P,
                            I,
                            D);

        while(__alive)
        {
            //Term->start();
            Term->pc(true,0,"T:%f   P:%f   I:%f   D:%f  ", T, P, I, D);

            t1 = tick_count(1000);                                          //  ns
            _chunk._hdr->buf_len = pipe.peek(_chunk.payload(), _chunk.len());    //  read the pipe
            if(_chunk._hdr->buf_len==-1)
            {
                 pid.reset(T,P, I, D);
                break;
            }
            else if(_chunk._hdr->buf_len==0)
            {
                 pid.reset(T,P, I, D);
                ::usleep(0xFFF);
                continue;
            }
            if(_chunk._hdr->buf_len == _chunk.len())                          //  we read
            {
                _chunk._hdr->frm_sig = ALIVE_DATA;
                _chunk._hdr->ao_rate = ao_rate;
                _chunk._hdr->cur_seq = ++frm;
                bps+=_chunk._hdr->buf_len;
            }

            // send to all clients
            buf_len = 0;
            srv.qsend(_chunk, buf_len, t1);
            bps += buf_len;

            // calc time diff
            t2 = tick_count(1000);
            rt = (t2-t1);
            if(rt < sleep_opt)
            {
                slploop = sleep_opt - rt;
            }
            else
            {
                slploop = 0xF;
            }

            frmdiff = srv.pool();
            if(frmdiff && srv._pilot_qsz && rt < sleep_opt)
            {
                int test = srv._pilot_qlen-srv._pilot_qoptim; // same as frmdiff
                UNUS(test);
                curperc = ((double)srv._pilot_qlen * 100.0) / (double)srv._pilot_qsz;
                foptim  = ((double)srv._pilot_qoptim * 100.0) / (double)srv._pilot_qsz;
                xpidv = pid.calculate(foptim, curperc);
                pidv = 100-xpidv;
                pidv /= 100.0;
                slploop = slploop * pidv;
            }

            us = __useconds_t(double(slploop))+0xF;
            ::usleep(us);
            Term->add(us,(curperc-50)*FRM_SCALE);

            // calc bps
            t3 = tick_count();
            if(t3-t1 > ONE_SEC)
            {
                t3      = t1;
                lastbps = bps;
                bps     = 0;

            }
            TERM_OUT(true,MSG_L1,
                     " SLP: %06d PID: %3.08f  BPS=%d",
                      us, pidv,lastbps);

            //Term->swap();

        }
        __alive = false;
        beacon.join();
    }
    std::cout << "terminated \n";
    return 0;
}




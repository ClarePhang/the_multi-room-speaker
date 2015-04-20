
#include <set>
#include "main.h"
#include "cpipein.h"
#include "clithread.h"
#include "aoutil.h"
#include "mp3cls.h"
#include "alsacls.h"
#include "tcpsrv.h"
#include "screenxy.h"
#include "pidaim.h"

bool    __alive = true;
RunCtx* PCTX;



int main(int argc, char *argv[])
{
    RunCtx  ctx(&PCTX);
    screenxy   sxy(argc,argv);
    (void)argc;
    (void)argv;
    ctx.server(argc, argv);
}

RunCtx::RunCtx(RunCtx** p)
{
    *p=this;
//    block_signal(SIGPIPE);
//    block_signal(SIGINT);
}

RunCtx::~RunCtx()
{

}

void RunCtx::server(int argc, char* argv[])
{
    CPipeIn         pipe(PIPE_IN);
    TcpSrv          srv;
    udp_group_sock  mcast;
    Mp3Cls          mp3;
    double          offset;

    system("clear");
    Term->pc(true, VER_L, "spkserver ver: %s",_VERSION);
    mcast.create(MC_ADDR, MC_PORT);
    mcast.set_mtu(UDP_CHUNK_LEN);
    if(pipe.ok() && srv.listen())
    {
        Qbuff::Chunk    chunk(UDP_SOUND_BUFF);
        int             frm=0;
        uint32_t        thisip    = (uint32_t)inet_addr(sock::GetLocalIP("127.0.0.1"));
        SADDR_46        test(thisip,0);
        uint64_t        sleep_opt = 1024 * ((1000*UDP_SOUND_BUFF) / (SAMPLE_AO * CHANNELS_AO * (BITS_AO/8)));
        uint64_t        sleeptime = sleep_opt;
        uint64_t        poltime;
        int             missframes=0;

        // 0.014000     0.200000 0.004000 0.800000
        double T = 0.014;
        double P = 0.2;
        double I = 0.004;
        double D = 0.7;
        if(argc>=2) T = ::atof(argv[1]);
        if(argc>=3) P = ::atof(argv[2]);
        if(argc>=4) I = ::atof(argv[3]);
        if(argc>=5) D = ::atof(argv[4]);

        printf("T-PID=%f     %f %f %f\n",T,P,I,D);
        uint64_t        ct;                //P   D     I
        //PID pid = PID(0.1, 100, -100,    0.1, 0.01, 0.5);

        PID             pid(T, 99.99, 0.01, P, I, D);

        double          xpidv=0.0,pidv = 0.0;
        Term->pc(true, MAIN_L, "Listening on: %s",test.c_str());

        while(__alive)
        {
            ct = tick_count(1000);

            chunk._hdr->n_bytes = pipe.peek(chunk.payload(), chunk.len());
    
            if(chunk._hdr->n_bytes==0)
            {
                MSLEEP(0xFFF);
                continue;
            }
            chunk._hdr->sign = UNIQUE_HDR;
            chunk._hdr->saddr=thisip;
            chunk._hdr->pilot_frame=srv.maxfrm();
            chunk._hdr->pilot_ping=srv.maxping();
            chunk._hdr->pred=ALIVE_DATA;
            chunk._hdr->cli_frame=++frm;
            mcast.send((uint8_t*)chunk.itself(), chunk.size(), MC_PORT, MC_ADDR);

            poltime = tick_count(1000)-ct;
            if(poltime<sleep_opt)
                sleeptime = sleep_opt - poltime;
            else
                sleeptime = 0xFF;

            missframes = srv.pool(frm);
            if(srv._cliq_size)
            {
                int test = srv._cli_optim-srv._cli_qlen;
                UNUS(test);
                offset = ((double)srv._cli_qlen * 100.0) / (double)srv._cliq_size;
                //pid.reset();
                xpidv = pid.calculate(50.0, offset);
                pidv = 100-xpidv;
                pidv /= 100.0;
            }
            sleeptime = sleeptime * pidv;
            ::usleep(__useconds_t(double(sleeptime))+0xF);

            Term->add(sleeptime,(offset-50)*FRM_SCALE);

            Term->pc(true,MSG_L,"SEQ:%06d SLEEP:%06d MISS:[%03d] TIMADJ:%f PID(%02.6f->%02.6f)",
                                chunk._hdr->cli_frame,
                                sleeptime,
                                missframes,
                                pidv,
                                offset,
                                pidv);
            Term->swap();
        }
    }
    mcast.destroy();
}

void RunCtx::client()
{
}

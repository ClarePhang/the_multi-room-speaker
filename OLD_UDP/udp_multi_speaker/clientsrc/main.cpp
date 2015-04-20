
#include <curses.h>
#include "main.h"
#include "cpipein.h"
#include "clithread.h"
#include "aoutil.h"
#include "mp3cls.h"
#include "alsacls.h"
#include "pulsecls.h"
#include "aocls.h"
#include "tcpsrv.h"
#include "screenxy.h"

bool    __alive = true;
RunCtx* PCTX;

/*
 *             chk->_hdr->a_bits=16;
            chk->_hdr->a_byte=4;
            chk->_hdr->a_channels=2;
            chk->_hdr->a_rate=44100;

 */



int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if(argc==2 )
    {
        Mp3Cls   mp{};

        uint8_t  frm[4096];
        if(mp.open(argv[1]))
        {
            AoCls    ao(mp._bits, mp._rate, mp._channels, 4096);
            uint64_t now = tick_count();
            while(mp.read(frm,4096))
            {
                ao.play(frm,4096,nullptr,0);
            }
            std::cout << tick_count()-now << "\n";
        }
        return 0;
    }

    screenxy  s(argc, argv);
    RunCtx  ctx(&PCTX);
    ctx.client();
}

RunCtx::RunCtx(RunCtx** p)
{
    *p = this;
    //signal(SIGPIPE, notapl);
}

RunCtx::~RunCtx()
{

}

size_t RunCtx::_fill_q(Qbuff& qb, bool& filling)
{
    size_t qoptim = qb.optim();
    if(qb.length()==0)
    {
        filling = true;
    }
    if(filling)
    {
        if(qb.length()>qoptim)
        {
            filling = false;
        }
        else
        {
            ::usleep(0xFFF);
            TERM_OUT(true,QUE_L,"NO STREAM");
        }
    }
    return qoptim;
}

void RunCtx::client()
{
    int           buffsz = 4096;
#if defined(AO_LIB)
    AoCls         ao(BITS_AO, SAMPLE_AO, CHANNELS_AO, UDP_SOUND_BUFF);
#elif defined(PULSE_AUDIO)
    pulsecls      pu(BITS_AO, SAMPLE_AO, CHANNELS_AO, UDP_SOUND_BUFF);
#else
    Alsacls       alsa;
    buffsz = alsa.open(SAMPLE_AO, CHANNELS_AO, 1, UDP_SOUND_BUFF);
#endif
    int           onekchunk = buffsz/1024;
    Qbuff         qb(CHUNKS, UDP_SOUND_BUFF);
    Qbuff::Chunk* pk;
    CliThread     t(&qb);
    uint64_t      ct = tick_count();
    uint64_t      now = ct;
    size_t        qlen,bytesec = 0,qoptim=OPTIM_QUEUE_LEN_H;
    bool          filling = true;
    size_t        fillingsz = Q_ALMOST_EMPTY;
    size_t        sleept = (1000*UDP_SOUND_BUFF) / (SAMPLE_AO * CHANNELS_AO * (BITS_AO/8));

    qb.set_optim(PER_CENT(qoptim));
    TERM_OUT(true,VER_L, "VERSION: %s",_VERSION);

    t.run(0);
    while(__alive)
    {

        qlen = qb.length();
        if(qlen < fillingsz || filling)     // filling empty q
        {
            TERM_OUT(true,QUE_L,"NO STREAM");
            qb.set_optim(PER_CENT(qoptim));
            fillingsz = _fill_q(qb, filling);
            continue;
        }
        fillingsz = Q_ALMOST_EMPTY;

        pk =  qb.q_consume();
        if(pk != nullptr)
        {
            if(pk->_hdr->n_bytes && pk->_hdr->pred==ALIVE_DATA)
            {
#ifdef AO_LIB
                ao.play(pk->payload(),pk->len(), &qb, t.is_pilot() );
#elif defined(PULSE_AUDIO)
                optim = pu.play(pk->payload(),pk->_hdr->n_bytes, qb);
#else
                optim = alsa.playbuffer(pk->payload(),pk->_hdr->n_bytes, qb);
#endif
                bytesec += pk->_hdr->n_bytes;
                pk->_hdr->n_bytes = 0;
                now = tick_count();
                if(now-ct>=QUART_SEC)
                {
                    TERM_OUT(true, QUE_L ,"SEQ:%05d RATE:%d Kby/s QL-[%05d] QO-[%05d] C_SKEW %05d S_SKE %05d",
                                        pk->_hdr->cli_frame,
                                        bytesec / QUART_SEC,
                                        qb.length() , qb.optim(),
                                        int(qb.optim() - qb.length()),
                                        t.srv_skew());
                    ct      = now;
                    bytesec = 0;
                }
            }
            pk->_hdr->n_bytes=0;
            do{
                std::lock_guard<std::mutex> guard(_mut);
                _frame = pk->_hdr->cli_frame;
            }while(0);
            qb.release(pk);
        }else {
            ::usleep(0x1FF);
        }
    }
    __alive=false;
}


#include "main.h"
#include "aoutil.h"
#include "sock.h"
#include <assert.h>
#include <cstdlib>
#include "clithread.h"
#include "pidaim.h"
#include "screenxy.h"
#include "main.h"
#include "dialog.h"

#define PRE_WORK_TO 256
#define AUDIBLE_DELAY 8
#define MIN_PLAY_BLOCKS 40
#define SELECT_TIME 0xFFF
#define TCP_SET     0x1
#define UDP_SET     0x2

CliThread::CliThread(Qbuff* qbuff)
{
    _pq = qbuff;
    _now = tick_count();
    _drainbuff = new uint8_t[UDP_SOUND_BUFF+sizeof(BufHdr)];
}


CliThread::~CliThread()
{
    if(_pt)
        _pt->join();
    delete _pt;
    delete[] _drainbuff;
}

bool CliThread::_reconnect(tcp_cli_sock& tcp, udp_group_sock& m)
{
    if(!tcp.is_really_connected())
    {
        _ping_us     = 0;
        _max_ping    = 0;
        _max_frame   = 0;
        if(!_connecting(m, tcp))
        {
            MSLEEP(PRE_WORK_TO);
            return false;
        }
    }
    return true;
}

#ifdef STONG_DEBUG
static struct {
    int pred;
    int cli_frame;
}   __few[8];
#endif //

bool CliThread::_netReceive(tcp_cli_sock& tcp, udp_group_sock& m, int sel)
{
    timeval        tv;
    fd_set         rd;

    FD_ZERO(&rd);
    FD_SET(tcp.socket(), &rd);
    FD_SET(m.socket(), &rd);
    int tmpv = std::max(m.socket(), tcp.socket())+1;
    tv.tv_sec = 0; tv.tv_usec=(sel);
    if((tmpv=::select(tmpv, &rd, 0, 0, &tv))<0)
    {
        perror("select");
        __alive=false;
        return false;
    }
    if(FD_ISSET(tcp.socket(), &rd))
    {
        tmpv = tcp.receiveall(_hdr);
        if(tmpv==int(sizeof(BufHdr)))
        {
            if(_store_qos())
            {
                _reply_qos(tcp);
            }
        }
        FD_CLR(tcp.socket(), &rd);
    }
    if(FD_ISSET(m.socket(), &rd))
    {
        Qbuff::Chunk*  chunk = _pq->obtain();
        if(chunk != nullptr)
        {
            int n_bytes = m.receive((uint8_t*)chunk->itself(),
                                    _pq->payload(),MC_PORT,MC_ADDR);
            if(n_bytes == _pq->payload())
            {
                if(chunk->_hdr->pred == ALIVE_DATA)
                {
                    ++_cur_frame;
                    if(chunk->_hdr->cli_frame != _cur_frame)
                    {
                        TERM_OUT(true,ERR_L,"MISSING SEQUENCE %d",_cur_frame);
                    }
                    _pq->q_feed(chunk);
                    _cur_frame = chunk->_hdr->cli_frame;
                }
            }
            else
            {
                _pq->release(chunk);
            }
        }
        else
        {
            m.receive(_drainbuff, UDP_SOUND_BUFF+sizeof(BufHdr),MC_PORT,MC_ADDR);
            if(_pq->percent() > _pq->optim())
            {
                int discard = _pq->percent() - _pq->optim();
                _pq->remove_back(discard);
            }
        }
        FD_CLR(m.socket(), &rd);
    }

    if(tick_count() - _now > 5000)
    {
        _now       = tick_count();
        _hdr.sign  = UNIQUE_HDR;
        _hdr.pred  = EMPTY_DATA;
        size_t alive = tcp.send(_hdr);
        if(alive != sizeof (_hdr))
        {
            tcp.destroy();
        }
    }
    return true;
}

void CliThread::_reply_qos(tcp_cli_sock& tcp)
{
    static          uint32_t saddr = inet_addr(sock::GetLocalIP("*"));

    _hdr.sign      = UNIQUE_HDR;
    _hdr.pred      = SYNC_DATA;
    _hdr.saddr     = saddr;
    _hdr.cli_frame   = PCTX->cli_frame();
    _hdr.cli_len     = _pq->length();
    _hdr.cli_opt     = _pq->optim();
    _hdr.cli_size    = _pq->maxsz();

    int tmpv = tcp.send((uint8_t*)&_hdr, sizeof(BufHdr));
    if(tmpv<=0)
    {
        TERM_OUT(true,CON_L,"Server not found. Disconnected    ");
        tcp.destroy();
    }
    TERM_OUT(true,CON_L,"SIG:%d SYNC_SEQ:%d ",
             _hdr.pred,
             int(_hdr.cli_frame));
}

/**
 * @brief CliThread::_tmain
 * makes sure the receiving buffer is > 80% full
 */
void CliThread::_tmain()
{
    tcp_cli_sock   tcp;
    udp_group_sock m;

    _frm_ms = (1000*UDP_SOUND_BUFF) / (SAMPLE_AO * CHANNELS_AO * (BITS_AO/8));
    while(__alive)
    {
        if(!_reconnect(tcp, m))
            continue;

        _netReceive(tcp, m, SELECT_TIME);
        ::usleep(0xFFF);
    }
}

bool CliThread::_connecting(udp_group_sock& m,
                            tcp_cli_sock& tcp)
{
    int tmpv;

    _max_frame=0;
    _max_ping=0;
    m.drop();
    m.destroy();
    MSLEEP(256);
    TERM_OUT(true,CON_L,"Join UDP: %s", MC_ADDR);
    if(m.join(MC_ADDR, MC_PORT)!=NOERROR)
    {
        perror(" Join...");
        TERM_OUT(true,CON_L,"Error: Join UDP: %s", MC_ADDR);
        return false;
    }
    TERM_OUT(true,CON_L,"Waiting for UDP GROUP");
    if(!m.is_set(256000) && __alive)
    {
        MSLEEP(0xFF);
        return false;
    }
    m.set_mtu(UDP_CHUNK_LEN);
    Qbuff::Chunk*  chunk =  _pq->obtain();
    if(chunk && __alive)
    {
        tmpv = m.receive(chunk->_playbuff,_pq->payload(),MC_PORT,MC_ADDR);
        if(tmpv !=_pq->payload())
        {
            _pq->release(chunk);
            TERM_OUT(true,ERR_L,"Invalid FRAME: %d %d", tmpv , _pq->payload());
            MSLEEP(0x1);
            return false;
        }
    }
    else
    {
        return false;
    }

    TERM_OUT(true, CON_L,"Connecting to: %s:%d", (const char*)Ip2str(chunk->_hdr->saddr),SRV_PORT+1);
    if(!tcp.is_really_connected() &&
            !tcp.isconnecting())
    {
        tcp.raw_connect(chunk->_hdr->saddr, SRV_PORT+1);
        MSLEEP(256);
        if(!tcp.is_really_connected() || !__alive)
        {
            _pq->release(chunk);
            tcp.destroy();
            TERM_OUT(true,CON_L,"FAILE TO CONNECT: %s:%d",
                     (const char*)Ip2str(chunk->_hdr->saddr),
                     SRV_PORT+1);
            return false;
        }
        TERM_OUT(true,CON_L,"CONNECTION TO: %s:%d",
                 (const char*)Ip2str(chunk->_hdr->saddr),
                 SRV_PORT+1);
    }
    tcp.set_blocking(1);
    _reply_qos(tcp);
    _pq->release(chunk);
    return __alive;
}

// command for player
bool CliThread::_store_qos()
{
    if(_hdr.sign == UNIQUE_HDR)
    {
        if(SYNC_DATA == _hdr.pred)
        {
            std::lock_guard<std::mutex> guard(_mut);
            if(_hdr.pilot_frame)
            {
                _pilot     = _hdr.cli_pilot;
                _max_ping  = _hdr.pilot_ping;
                _max_frame = _hdr.pilot_frame;
                _ping_us   = _hdr.cli_ping;
                _cli_skew  = _hdr.cli_opt - _hdr.cli_len;
                _frmdiff  = _hdr.pilot_frame-_hdr.cli_frame;

                if(_hdr.cli_pilot==false)
                {
                    if(std::abs( int(_hdr.pilot_q_perc - CLI_SYNC_PERCENT)) < SYNCHED_DELTA )
                    {
                        if(time(0)-_reque_time>MAX_REQUE_INTERVAL)
                        {
                            int newopt = PER_CENT(OPTIM_QUEUE_LEN_H) - _frmdiff;
                            if(newopt > SYNCHED_DELTA<<1)
                            {
                                _pq->force_optim(newopt);
                            }
                            else
                            {
                                TERM_OUT(true,ERR_L,"CANNOT SYNC Q TO SHORT OR DELAY FROM pILOT TO BIG");
                            }
                        }
                    }
                }
                else
                {
                    _pq->force_optim(CLI_SYNC_PERCENT);
                }


            }
            return true;
        }
        if(_hdr.pred == VOLUM_SIG)
        {
            char cmd[64];
            ::sprintf(cmd,"amixer set Master %d%%", _hdr.n_bytes);
            system(cmd);
        }
    }
    return false;
}

bool CliThread::run(size_t chunk_ms)
{
    _pt = new std::thread(&CliThread::_tmain, this);
    return true;
}



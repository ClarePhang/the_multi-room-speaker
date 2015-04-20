/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/

#include <unistd.h>
#include "main.h"
#include "sclient.h"
#include "srvq.h"
#include "pider.h"
#ifndef TARGET_ANDROID
#include "screenxy.h"
#endif

#define MAX_TRACK_SEQ  32

sclient::sclient(Circle* qb,
                 const std::set<std::string>& ips):_rq(qb),
                                                    _ips(ips)
{
    _now = tick_count();
    memset(&_qos,0,sizeof(_qos));
    _first_time = false;
}

sclient::~sclient()
{
    tcp_cli_sock::destroy();
    if(_pt)
        _pt->join();
    delete _pt;
}

bool sclient::run(size_t chunk_ms)
{
    _tperchunk = chunk_ms;
    _pt = new std::thread(&sclient::_tmain, this);
    return true;
}

/*THREAD*/
void sclient::_tmain()
{
    _reque_time = time(0) - MAX_REQUE_INTERVAL + 5;

    while(__alive)
    {
        if(!this->is_really_connected() &&
                !this->isconnecting())
        {
            for(const auto& a : _ips)
            {
                // reconnecting

                TERM_OUT(true,CON_L,"CONNECTING TO: %s", a.c_str());
                sleep(1);
                if(this->s4connect(a.c_str(), SPK_SRV_PORT)>0)
                {
                    // connected
                    TERM_OUT(true,CON_L,"CONNECTED TO: %s ",a.c_str());
                    break;
                }
                // prepare a reconnect
                this->destroy();
                TERM_OUT(true,CON_L,"CONNECTION LOST");
            }
            // use it
            this->set_mtu(TCP_MTU);
            this->set_blocking(1);
            _first_time = false;
            _get_data();        // loops for receive
        }
        TERM_OUT(true,CON_L,"CONNECTION LOST");
        this->destroy();
    }
}

bool sclient::_check_stream(Circle::Chunk* pk, bool drop)
{
    UNUS(drop);
    int buf_len = 0;
    if(sizeof(BufHdr) != this->receiveall((const uint8_t*)pk->hdr(),
                                          sizeof(BufHdr)))
    {
        return false;
    }
    _srvsync = true;
    if(pk->_hdr->buf_len)
    {
        buf_len = this->receiveall((const uint8_t*)pk->payload(),pk->len());
    }
    if(buf_len != pk->_hdr->buf_len)
    {
        TERM_OUT(true,ERR_L,"invalid buf_len recieved");
        return false;
    }
    _bps += buf_len;
    if(pk->_hdr->frm_sig==RESET_ALL)
    {
        _rq->reset();
        TERM_OUT(true,CON_L,"RESETING Q");
        return true;
    }

    if(pk->_hdr->frm_sig==SYNC_DATA && pk->_hdr->pilot_seq!=0)
    {
        _rewind = 0;

        _ping     = pk->_hdr->cli_ping;           // this cli_ping
        _pingdiff = pk->_hdr->pilot_ping;         // lazy client cli_ping
        _frmdiff  = pk->_hdr->pilot_seq - pk->_hdr->cur_seq;

        if(pk->_hdr->is_pilot==false)
        {
            if(std::abs(pk->_hdr->pilot_q_perc-CLI_SYNC_PERCENT)<SYNCHED_DELTA )
            {
                if(_frmdiff && time(0)-_reque_time>MAX_REQUE_INTERVAL)
                {
                    _reque_time = time(0);
                    size_t optlen = _rq->length();
                    int newopt = int(optlen) - _frmdiff;

                    if(newopt > SYNCHED_DELTA<<1)
                    {
                        _rq->force_optim(newopt);
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
            _rq->force_optim(CLI_SYNC_PERCENT);
        }

        TERM_OUT(true,CON_L,"FRM: %04d PFRM: %04d  FRM-DIFF= %04d  OPTIM: %04d    ",
                 pk->_hdr->cur_seq,
                 pk->_hdr->pilot_seq,
                 _frmdiff,
                 _rq->optim());

        std::lock_guard<std::mutex> guard(_mut);
        if(_qos.frm_sig)
        {
            _qos.srv_tick = pk->_hdr->srv_tick;
            this->sendall((const uint8_t*)&_qos,sizeof(_qos));
            _qos.frm_sig=0;
        }
    }
    _fut=tick_count();
    if(_fut - _now > ONE_SEC)
    {
        _now = _fut;
        buf_len = 0;
        std::lock_guard<std::mutex> guard(_mut);
        _bps = buf_len;
    }
    return true;
}

// io socket
void sclient::_get_data()
{
    Circle::Chunk*  chunk;
    timeval         tv;
    fd_set          rd;

    FD_ZERO(&rd);
    while(this->socket()>0)
    {
        FD_SET(this->socket(), &rd);
        int tmpv = this->socket()+1;
        tv.tv_sec = 0; tv.tv_usec=(0xFFFF);
        if((tmpv=::select(tmpv, &rd, 0, 0, &tv))<0)
        {
            TERM_OUT(true,ERR_L,"::Select error: %s. exits.",strerror(errno));
            __alive=false;
        }
        if(this->socket()<0){
            break;
        }
        if(FD_ISSET(this->socket(), &rd))
        {
            if(nullptr!=(chunk = _rq->q_feedup(false)))
            {
                if(_check_stream(chunk))
                {
                    FD_CLR(this->socket(), &rd);
                }
                else
                {
                    _rq->release(chunk);
                    this->destroy();
                    break;
                }
                _rq->release(chunk);
            }
            else    //  get and drop frame
            {
                chunk = _rq->dummy();
                _check_stream(chunk, true);
                _rq->release(chunk);
            }
        }
#ifdef TARGET_ANDROID
        ::usleep(0xF);
#else
        ::usleep(0xF);
#endif
    }
}

void sclient::update_frame(const BufHdr& cur)
{
    std::lock_guard<std::mutex> guard(_mut);
    _qos.frm_sig      =  SYNC_DATA;
    _qos.cur_seq      = cur.cur_seq;
    _qos.cli_qlen     = _rq->length();
    _qos.cli_qoptim   = _rq->optim();
    _qos.cli_qsize    = _rq->maxsz();
}


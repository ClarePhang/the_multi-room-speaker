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
#include <set>
#include <vector>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include "sock.h"
#include "main.h"
#include "tcpsrv.h"
#include "screenxy.h"
#include "srvq.h"


TcpSrv::TcpSrv()
{

}

TcpSrv::~TcpSrv()
{
    destroy();
}

bool TcpSrv::listen()
{
    int ntry = 0;
AGAIN:
    if(__alive && this->create(SPK_SRV_PORT, SO_REUSEADDR, 0)>0)
    {
        fcntl(this->socket(), F_SETFD, FD_CLOEXEC);
        if(this->tcp_srv_sock::listen(8)!=0)
        {
            std::cout <<"socket can't listen. Trying "<< (ntry+1) << " out of 10 " << std::endl;

            this->tcp_srv_sock::destroy();
            sleep(1);
            if(++ntry<10)
                goto AGAIN;
            return false;
        }
        TERM_OUT(true,1, "listening  %d " , SPK_SRV_PORT);
        return true;
    }
    return false;
}

size_t  TcpSrv::qsend(const Circle::Chunk& chunk, size_t& buf_len, uint64_t usecs)
{
    size_t          clis = 0;
    int             lbytes;
    uint8_t         frm_sig = ALIVE_DATA;

    if(_newclient){
        _newclient = 0;
        _pilotseq    = 0;
        _pilotping   = 0;
    }
    else if(usecs-_last_qos > QOS_TO)
    {
        _last_qos   = usecs;
        frm_sig    = SYNC_DATA;
    }

    chunk._hdr->pilot_seq  = _pilotseq;
    chunk._hdr->pilot_ping = _pilotping;
    chunk._hdr->frm_sig    = frm_sig;

    for (const auto& s : _clis)
    {
        if(chunk._hdr->buf_len)
        {
            Clictx* pcli = s.second;

            chunk._hdr->cli_qlen   = pcli->_req.cli_qlen;
            chunk._hdr->cli_qsize  = pcli->_req.cli_qsize;
            chunk._hdr->cli_qoptim = pcli->_req.cli_qoptim;
            chunk._hdr->cli_ping   = pcli->_req.cli_ping;
            chunk._hdr->pilot_q_perc = (_pilot_qlen*100)/((_pilot_qsz)+1);
            chunk._hdr->srv_tick   = tick_count(1000);
            chunk._hdr->is_pilot   = pcli->_req.is_pilot;
            buf_len += chunk.size();
            lbytes = pcli->sendall(chunk.buff(),chunk.size());
            if(lbytes != 0)
            {
                TERM_OUT(false,CLI_L+clis,
                         " FAILED TO SEND "),
                          s.second->destroy();
            }
            else{
                TERM_OUT(false,CLI_L+clis, "PIL:  %03d "
                                           "SRV-SK: %05d "
                                           "CLI-SK: %05d "
                                           "Q: S: %04d O: %04d L: %04d",
                         pcli->_req.is_pilot,
                         this->_pilotseq - pcli->_req.cur_seq,
                         pcli->_req.cli_qoptim - pcli->_req.cli_qlen,
                         pcli->_req.cli_qsize,
                         pcli->_req.cli_qoptim,
                         pcli->_req.cli_qlen);
            }
        }
        ++clis;
    }
    return clis;
}

int TcpSrv::pool()
{
    int     req=0;
    fd_set  rd;
    int     ndfs = this->socket();
    timeval tv {0, 0xFF};              // 1 ms max

    FD_ZERO(&rd);
    FD_SET(this->socket(), &rd);        // accept
    for(auto& s : _clis)
    {
        if(s.second->socket()>0)        // read from clients
        {
            FD_SET(s.second->socket(), &rd);
            ndfs = std::max(ndfs, s.second->socket());
        }
        else
            _dirty = true;              // cleanup flag
    }
    int is = ::select(ndfs+1, &rd, 0, 0, &tv);
    if(is ==-1) {
        // very bad
        TERM_OUT(true,ERR_L,"socket select() error: %d",errno);
        ::sleep(3);
        __alive=false;
        return -(errno);
    }
    if(is)
    {
        // accept new connection
        if(FD_ISSET(this->socket(), &rd))
        {
            Clictx* cs = new Clictx(this);
            if(this->accept(*cs)>0)
            {
                cs->Rsin().commit();
                cs->_ipaddr = cs->Rsin().c_str();
                cs->_index  = _clis.size();
                TERM_OUT(true,CLI_L+cs->_index,
                         "[%s] %d",cs->Rsin().c_str(), cs->_index);
                cs->dirty_it();
                cs->set_blocking(1);
                cs->set_mtu(TCP_MTU);
                if(_clis.find(cs->Rsin().ip4()) != _clis.end())
                {
                    // why we got here ? ...
                    _clis[cs->Rsin().ip4()]->destroy();
                    delete _clis[cs->Rsin().ip4()];
                }
                _clis[cs->Rsin().ip4()]=cs;
                _newclient = true;                  // resync all flag
                this->_pilotseq     = 0;
                this->_pilotping    = 0;
                this->_pilot_opt_skew = 0;
                this->_pilot_qlen   = 0;
                this->_pilot_qsz    = 0;
                this->_pilot_qoptim = 0;
            }
            else
            {
                delete cs;
            }
        }
        // got data frmo any

        for(auto& s : _clis)
        {
            if(FD_ISSET(s.second->socket(), &rd))
            {
                s.second->rec(this);
            }
        }
    }
    if(_dirty)
    {
        _clean();
    }
    return _pilot_qsz ? _pilot_qoptim-_pilot_qlen : 0;
}

void TcpSrv::_clean()
{
A:
    for(std::map<uint32_t,Clictx*>::iterator  s=_clis.begin();
        s!=_clis.end();s++)
    {
        if(s->second->socket()<=0)
        {
            delete (s->second);
            _clis.erase(s);
            TERM_OUT(true,CLI_L+s->second->_index,
                     "[%d/%s] %d: DISCONNECTED", s->second->_index, s->second->Rsin().c_str(),0);
            goto A;
        }
    }
    int k=0;
    for(std::map<uint32_t,Clictx*>::iterator  s=_clis.begin();
        s!=_clis.end();s++){
        s->second->_index=k++;
    }

    this->_pilotseq     = 0;
    this->_pilotping    = 0;
    this->_pilot_opt_skew = 0;
    this->_pilot_qlen   = 0;
    this->_pilot_qsz    = 0;
    this->_pilot_qoptim = 0;
    _dirty = false;     // all clean
}

bool TcpSrv::destroy(bool be)
{
    tcp_srv_sock::destroy(be);
    for(auto& s : _clis)
    {
        delete s.second;
    }
    return true;
}

Clictx::Clictx(TcpSrv*  psrv):_psrv(psrv)
{
    ::memset(&_req,0,sizeof(_req));
}

Clictx::~Clictx()
{

}

// delegation form server
int  Clictx::rec(TcpSrv* psrv)
{
    int rt = this->receiveall((uint8_t*)&_req,sizeof(_req));    // get the header
    if(rt==0) // con closed
    {
        TERM_OUT(true,CLI_L+this->_index,
                 "[%d] %d: CLIENT GONE %d",
                 this->_index,this->Rsin().ip4());
        this->destroy();
        _dirty = true;
        return 0;
    }
    if(_req.frm_sig==SYNC_DATA)
    {
        _req.cli_ping = tick_count(1000) - _req.srv_tick;

        if(this->_index==0)
        {
            psrv->_pilotseq         = _req.cur_seq;         // update seerver maxims
            psrv->_pilotping        = _req.cli_ping;        // same as above
            psrv->_pilot_opt_skew   = _req.cli_qoptim - _req.cli_qlen;   // pilot client skew from 50%
            psrv->_pilot_qlen       = _req.cli_qlen;
            psrv->_pilot_qsz        = _req.cli_qsize;
            psrv->_pilot_qoptim     = _req.cli_qoptim;
            _req.is_pilot           = true;
        }
        else
        {
            _req.is_pilot = false;
        }
        _req.buf_len    = 0;
        _req.pilot_ping = psrv->_pilotping;
        _req.pilot_seq  = psrv->_pilot_qsz;
        if(_index == 0)             //  we keep on sync first client only client
        {
            return psrv->_pilot_opt_skew;
        }
    }
    return 0;
}

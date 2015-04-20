/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/

#ifndef CLICTX_H
#define CLICTX_H

#include <map>
#include <atomic>
#include "sock.h"
#include "srvq.h"

#define MAX_SHARED (sizeof(BufHdr)<<8)

class TcpSrv;
class Clictx : public tcp_cli_sock
{
public:
    Clictx(TcpSrv*  psrv);
    virtual ~Clictx();

    void  dirty_it(){_dirty=true;}
    int  rec(TcpSrv* psrv);
#ifdef THREADED
#endif//
private:
    bool        _dirty = false;
    bool        _stats = false;
    TcpSrv*     _psrv;

public:
    BufHdr      _req;
    int         _index;
    std::string _ipaddr;
};

class TcpSrv : public tcp_srv_sock
{
public:
    friend int main(int, char*[]);
    friend class Clictx;
    TcpSrv();
    virtual ~TcpSrv();
    bool     destroy(bool be=true);
    bool     listen();
    int      pool();
    size_t   qsend( const Circle::Chunk& , size_t&, uint64_t msecs);
    void     set_max_frm(int max){_pilotseq = max;}
private:
    void    _clean();

private:
    std::map<uint32_t,Clictx*> _clis;
    int                  _cli = 0;
    int                  _s = 0;
    bool                 _dirty=false;
    bool                 _newclient=false;
    uint64_t             _last_qos=0;
    std::mutex           _datalock;
    uint32_t             _pilotseq = 0;
    uint64_t             _pilotping = 0;
    size_t               _qos_time = QOS_TO;
    bool                 _srvsync = false;
    int                  _pilot_opt_skew=0;
    int                  _pilot_qlen=0;
    int                  _pilot_qsz=0;
    int                  _pilot_qoptim=0;
};



#endif // CLICTX_H

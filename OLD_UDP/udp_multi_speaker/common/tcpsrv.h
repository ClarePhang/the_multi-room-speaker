#ifndef CLICTX_H
#define CLICTX_H

#include <vector>
#include <atomic>
#include "sock.h"
#include "clithread.h"

#define MAX_SHARED (sizeof(BufHdr)<<8)
class TcpSrv;
class Clictx : public tcp_cli_sock
{
public:
    Clictx(TcpSrv*  psrv);
    virtual ~Clictx();
    int     treceive(uint8_t* s, int sz);
    void     keeptrack(TcpSrv* psrv);
    void    set_dirty(){_dirty=true;};
    bool    is_dirty()const{return _dirty;}
    void    notified(){
        _notified=true;
    };
    void    un_notified(){
        _notified=false;
    };
private:
    void    _html_send(char* pc);
    BufHdr  _hdr;
    bool    _dirty = false;
public:
    bool        _stats = false;
    int64_t     _ping = 0;
    TcpSrv*     _psrv = nullptr;
    bool        _player  = false;
    int         _srv_frame_skew=0;
    int64_t     _tick_skew = 0;
    int         _cli_frm_skew=0;
    uint32_t    _cliframe = 0;
    uint32_t    _srvframe = 0;

    int         _volume = 30;
    int         _index = 0;
    bool        _notified=false;
};

class TcpSrv : public tcp_srv_sock
{
public:
    friend class Clictx;
    TcpSrv();
    virtual ~TcpSrv();
    bool    destroy(bool be=true);
    bool    listen();
    int     pool(uint32_t frm);
    uint32_t maxping()const{return _pilot_ping;};
    uint32_t maxfrm()const{return _pilot_frm;};
    const std::vector<Clictx*>& clis()const{return _clis;};
    void setVolume(uint32_t htip, int vol);
    int  getVolume(uint32_t htip)const;

private:
    void    _clean();

private:
    std::vector<Clictx*> _clis;
    int                 _cli = 0;
    int                 _s = 0;
    bool                _dirty=false;
    uint64_t            _pilot_ping=0;
    uint32_t            _pilot_frm=0;
    uint64_t            _curt = 0;
    bool                _newclient=false;
    int                 _notified=0;
    int                 _counter = 0;
    uint64_t            _qostout = QOS_TOUTMIN;
public:
    uint32_t            _cli_optim = 0;
    uint32_t            _cli_qlen = 0;
    uint32_t            _cliq_size = 0;
    uint32_t            _pilot_q_perc = 0;
    int                 _cliq_cli_skew = 0;
};



#endif // CLICTX_H

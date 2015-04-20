
#include <unistd.h>
#include <set>
#include <vector>
#include <sstream>
#include <fcntl.h>
#include "sock.h"
#include "main.h"
#include "tcpsrv.h"
#include "screenxy.h"

static size_t split(const std::string& s, char delim, std::vector<std::string>& a);

TcpSrv::TcpSrv()
{
    _curt = tick_count()-QOS_TOUTMAX;
    _pilot_ping = 0;
}

TcpSrv::~TcpSrv()
{
    destroy();
}

bool TcpSrv::listen()
{
    int ntry = 0;
AGAIN:
    if(__alive==false)
        return false;
    if(this->create(SRV_PORT+1, SO_REUSEADDR, 0)>0)
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
        std::cout << "listening \n";
        return true;
    }
    return false;
}

int TcpSrv::pool(uint32_t sentfrm)
{
    int     missclifrm=0;
    fd_set  rd;
    int     ndfs = this->socket();// _s.sock()+1;
    timeval tv {0, 0x1FFF};
    uint8_t  loco[MAX_SHARED];

    FD_ZERO(&rd);
    FD_SET(this->socket(), &rd);
    if(_clis.size())
    {
        for(auto& s : _clis)
        {
            if(s->socket()>0)
            {
                FD_SET(s->socket(), &rd);
                ndfs = std::max(ndfs, s->socket());
                if(s->_index==0){
                    missclifrm = s->_cli_frm_skew;
                }
            }
            else
                _dirty = true;
        }
    }
    int is = ::select(ndfs+1, &rd, 0, 0, &tv);
    if(is ==-1) {
        TERM_OUT(true,ERR_L,"socket select() error: %d",errno);
        ::sleep(3);
        __alive=false;
        throw (errno);
    }
    if(is)
    {
        if(FD_ISSET(this->socket(), &rd))
        {
            Clictx* cs = new Clictx(this);
            if(this->accept(*cs)>0)
            {
                //std::cout << "new connection \n";
                cs->_index = _clis.size();
                TERM_OUT(true,CLI_L+cs->_index,
                         "[%d] %d: NEW CLIENT: %d",
                         cs->_index, cs->Rsin().ip4());
                cs->set_dirty();
                cs->set_blocking(1);

                _clis.push_back(cs);

                _curt = tick_count()-QOS_TOUTMAX;
                _pilot_ping   = 0;
                _pilot_frm    = 0;
                _newclient = true;
                _counter   = 0;
            }
            else
            {
                delete cs;
            }
        }

        TERM_OUT(true,CLI_H,"PIVOT  |Q-LEN  |Q-OPTIM|PING   |MAXPING|FRM    |MAX_FRM|CLI_SKE|SRV_SKEW|");

        for(auto& s : _clis)
        {
            if(s->socket()<=0){
                _dirty=true;
                continue;
            }
            if(FD_ISSET(s->socket(), &rd))
            {
                int rt = s->treceive(loco, MAX_SHARED-1);
                if(rt==0) //con closed
                {
                    TERM_OUT(true,CLI_L+s->_index,
                             "[%d] %d: CLIENT GONE %d",s->_index, s->Rsin().ip4());
                    s->destroy();
                    _dirty = true;
                }
                else if(rt > 0)
                {
                    s->keeptrack(this);
                }
            }
            if(s->_index==0){
                missclifrm = s->_cli_frm_skew;
            }
        }
    }
    uint64_t curt = tick_count();
    if(curt - _curt > _qostout )
    {
        BufHdr  h = {};

        h.sign         = UNIQUE_HDR;
        h.pred         = SYNC_DATA;
        h.n_bytes      = 0;
        h.pilot_ping   = _pilot_ping;
        h.srv_frame    = sentfrm;
        h.pilot_q_perc = _pilot_q_perc;

        for(auto& s : _clis)
        {
            if(s->_index==0){
                h.pilot_frame = s->_cliframe;
                h.cli_pilot   = true;
            }
            else{
                h.pilot_frame  = _pilot_frm;
                h.cli_pilot   = false;
            }
            h.srv_tick    = tick_count(1000);
            h.cli_ping    = s->_ping;
            h.cli_frame   = s->_cliframe;
            if(s->send(h) !=sizeof(h))
            {
                s->destroy();
            }
            else{
                s->notified();
                ++_notified;
            }
        }
        _curt       = curt;
    }
    if(_dirty)
        _clean();
    return missclifrm;
}

void TcpSrv::_clean()
{
AGAIN:
    int index = 0;
    for(std::vector<Clictx*>::iterator s=_clis.begin();s!=_clis.end();++s)
    {
        if((*s)->socket()<=0)
        {
            delete (*s);
            _clis.erase(s);
            TERM_OUT(true,CLI_L+(*s)->_index,
                     "[%d] %d: CIENT %d GONE", (*s)->_index,(*s)->Rsin().ip4());
            _pilot_ping  = 0;
            _pilot_frm   = 0;
            goto AGAIN;
        }
        (*s)->_index = index++;
    }
    _dirty=false;
}


void TcpSrv::setVolume(uint32_t htip, int vol)
{
    BufHdr h;
    h.sign     = UNIQUE_HDR;
    h.pred    = VOLUM_SIG;
    h.n_bytes   = vol;
    for(auto& s : _clis)
    {
        if(s->Rsin().ip4() == htip)
        {
            s->send(h);
            s->_volume = vol;
        }
    }
}

int  TcpSrv::getVolume(uint32_t htip)const
{
    for(auto& s : _clis)
    {
        if(s->Rsin().ip4() == htip)
        {
            return s->_volume;
        }
    }
    return 0;
}


bool TcpSrv::destroy(bool be)
{
    tcp_srv_sock::destroy(be);
    for(auto& s : _clis)
    {
        delete s;
    }
    return true;
}


Clictx::Clictx(TcpSrv*  psrv):_psrv(psrv)
{
    memset(&_hdr,0,sizeof(_hdr));
}

Clictx::~Clictx()
{

}

void Clictx::keeptrack(TcpSrv* psrv)
{

    if(this->_hdr.pred == SYNC_DATA )
    {
        un_notified();
        if(std::abs(int(psrv->_pilot_frm - _hdr.cli_frame)) > CHUNKS)
        {
            //we cnannot sync
            psrv->_pilot_frm=0;
        }
        uint64_t now = tick_count(1000);
        _ping = now - _hdr.srv_tick;
        if(_ping<0)
        {
            _ping = 0;
        }
        if(_index == 0 )
        {
            psrv->_pilot_ping = _ping;
            _hdr.pilot_ping = psrv->_pilot_ping;
            psrv->_pilot_frm = _hdr.cli_frame;
            _hdr.pilot_frame = psrv->_pilot_frm;
            _hdr.cli_pilot = true;
            psrv->_pilot_q_perc = (_hdr.cli_len) * 100 / _hdr.cli_size;
        }
        else{
            _hdr.cli_pilot = false;
        }
        _srv_frame_skew =  psrv->_pilot_frm - _hdr.cli_frame;
        _tick_skew      =  psrv->_pilot_ping - _hdr.cli_ping;
        _cliframe       = _hdr.cli_frame;

        if(_srv_frame_skew == 0){
            psrv->_cli_optim   = _hdr.cli_opt;
            psrv->_cli_qlen    = _hdr.cli_len;
            psrv->_cliq_size   = _hdr.cli_size;
            _cli_frm_skew      = _hdr.cli_opt - _hdr.cli_len;
        }
        else{
            _cli_frm_skew = 0;
        }
        //PIVOT  |Q-LEN  |Q-OPTIM|PING   |MAXPING|FRM    |MAX_FRM|CLI_SKE|SRV_SKEW|
        TERM_OUT(true,CLI_L+_index, "%05d  "
                                    "|%05d  "
                                    "|%05d  "
                                    "|%05d  "
                                    "|%05d  "
                                    "|%05d  "
                                    "|%05d  "
                                    "|%05d  "
                                    "|%05d  ",
                                    _hdr.cli_pilot,
                                    _hdr.cli_len,
                                    _hdr.cli_opt,
                                    _ping,
                                    psrv->_pilot_ping,
                                    _hdr.cli_frame,
                                    psrv->_pilot_frm,
                                    _hdr.cli_opt-_hdr.cli_len,
                                    _srv_frame_skew);

    }
}


/**
 * @brief Clictx::receive, hope to exhaust this, and read all
 * @param pbuff
 * @param sz
 * @return
 */
int   Clictx::treceive(uint8_t* pbuff, int sz)
{
    _player=false;
    ::memset(&_hdr,0,sizeof(_hdr));
    int n_bytes = this->receive((unsigned char*)pbuff, sz);
    if(n_bytes >= int(sizeof(BufHdr)))
    {
        BufHdr* ph = (BufHdr*)pbuff;
        if(ph->sign == UNIQUE_HDR)
        {
            //std::cout <<"cli_frame" << n_bytes << " cli_frame" << int(ph->pred) <<"\n";
            ::memcpy(&_hdr,pbuff,sizeof(_hdr));
            _player=true;
        }
        else
        {
            char* pc = (char*)pbuff;
            if(!strncmp(pc,"GET ",4))
            {
                pc[n_bytes]=0;
                _html_send(pc);
            }
            this->destroy();
            return 0;
        }
        return n_bytes;
    }
    return 0;
}

void Clictx::_html_send(char* missclifrm)
{
    std::vector<std::string> preds;
    std::string reply = "";
    char* pt = ::strtok(missclifrm," ");
    pt = ::strtok(nullptr," ");
    if(pt)
    {
        std::string missclifrm = pt;
        std::string resp;
        split(missclifrm,'/',preds);

        if(preds[0]=="speakers")
        {
            const std::vector<Clictx*>& cs = _psrv->clis();
            for(const auto& c : cs)
            {
                if(c->_player==false)
                    continue;
                resp+=std::to_string(c->Rsin().ip4());
                resp+=",";
            }
        }
        if(preds[0]=="speaker" && preds.size()>1)
        {
            std::string ip = preds[1];
            if(preds.size()>2)
            {
                _psrv->setVolume(::atol(preds[1].c_str()),
                        ::atol(preds[2].c_str()));
            }
            else //get
            {
                resp+=_psrv->getVolume(std::stoi(preds[1]));
            }
        }
        std::string htreply  = "HTTP/1.1 200 OK\r\n";
        htreply += "Cache-Control : no-cache, private\r\n";
        htreply += "Content-Length :" + std::to_string(resp.length());
        htreply +=  "\r\n";
        htreply += "Content-Type: application/text\r\n\r\n";
        htreply += resp;
        this->send(htreply.c_str(), htreply.length());
    }
}


static size_t split(const std::string& s, char delim, std::vector<std::string>& a)
{
    std::string st;
    std::istringstream ss(s);
    a.clear();
    while (getline(ss, st, delim))
    {
        if(!st.empty())
            a.push_back(st);
    }
    return a.size();
}


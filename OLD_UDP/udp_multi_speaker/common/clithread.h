#ifndef RECTHraccess_H
#define RECTHraccess_H

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include "sock.h"

#define SRV_PORT        18722
#define CLI_PORT        18720
#define MC_ADDR         "238.8.255.255"
#define UNIQUE_HDR      0xA5
#define MC_PORT         10007
#define QOS_TOUTMIN        500
#define QOS_TOUTMAX        1000
#define VOLUM_SIG       10
#define SYNC_DATA       11
#define FRM_SIG         12
#define SETT_SIG        13
#define RESTA_SIG       14
#define SYNC_ALL        15
#define ALIVE_DATA      16
#define RESET_ALL       17
#define NEED_DATA       18
#define EMPTY_DATA      19



#define     TCP_SND_LEN     4096
#define     SRV_CHUNKS      32

#define     BITS_AO         16
#define     SAMPLE_AO       44100
#define     CHANNELS_AO     2
#define     MAX_SKEW        8


#define _UDP_HDR            28

#define ONE_SEC               1000
#define FOUR_SEC              4000
#define QUART_SEC              250
#define CHUNKS                100
#define OPTIM_QUEUE_LEN_H     50
#define CLI_SYNC_PERCENT      50
#define Q_ALMOST_FULL         95
#define Q_ALMOST_EMPTY        5
#define Q_LOW_ACC             25
#define Q_HI_ACC              75
#define MIN_SKEW              6

#define MAX_REQUE_INTERVAL  240
#define SYNCHED_DELTA       4


#define PER_CENT(x_) (x_*CHUNKS / 100)

typedef struct
{
    uint8_t  sign;
    bool     locked;
    uint8_t  pred;       //allways FD
    bool     cli_pilot;
    uint32_t cli_frame;
    uint32_t srv_frame;
    uint64_t srv_tick;
    uint64_t cli_ping;
    uint64_t pilot_ping;
    uint32_t pilot_frame;
    uint32_t pilot_q_perc;
    uint32_t saddr;
    uint32_t cli_len;
    uint32_t cli_opt;
    uint32_t cli_size;
    int      n_bytes;
    int      au_rate;
}  __attribute__ ((packed)) BufHdr ;

#define UDP_SOUND_BUFF           (1024<<3)
#define UDP_CHUNK_LEN            (UDP_SOUND_BUFF+sizeof(BufHdr))
#define MAX_REQUE_TIME           240


class Qbuff
{
public:

    struct Chunk
    {
        explicit Chunk(int sz):_sz(sz + sizeof(BufHdr)){
            _playbuff = new uint8_t [sz + sizeof(BufHdr)];
            ::memset(_playbuff,0,sz);
            _hdr = (BufHdr*)_playbuff;
        }
        ~Chunk(){
            delete[] _playbuff;
        }
        uint8_t*  payload()const{return _playbuff+sizeof(BufHdr);}
        uint8_t*  itself()const{return _playbuff;}
        int       size()const{return _sz;}
        int       len()const{return _sz-sizeof(BufHdr);}
        BufHdr*   _hdr=nullptr;
        uint8_t*  _playbuff=nullptr;
        int       _sz;
    };

    Qbuff(int chunks, int chunk):_maxqsz(chunks){
        _chunksz = (sizeof(BufHdr)+chunk);
        for(int i=0;i<chunks;i++)
        {
            Chunk* k = new  Chunk(chunk);
            _pool.push_back(k);
        }

    }
    ~Qbuff()
    {
        reset();
        for(auto& a : _pool){
           delete a;
        }
    }
    void resize(size_t max)
    {
        if(max < _maxqsz )
        {
            remove_back((_maxqsz-max)+1);
            _maxqsz=max;
        }
    }

    uint32_t maxsz()const{return _maxqsz;}
    int payload()const{ return _chunksz;}
    int percent()const{ return (_queue.size()*100) / _maxqsz; }
    int almostFull()const{return _full;}
    int ctitical()const{return _optim/2;}
    void setFull(int f){_full=f;}
    bool set_optim(size_t f){
        std::lock_guard<std::mutex> guard(_m);
        if(f!=_optim){
            _optim=f;
            return true;
        }
        return false;
    }
    int optim(){
        std::lock_guard<std::mutex> guard(_m);
        return _optim;
    }

    void remove_back(int els)
    {
        Chunk*  k;
        std::lock_guard<std::mutex> guard(_m);
        while(--els>0 && _queue.size())
        {
            k = _queue.front();
            _queue.pop_front();
            k->_hdr->locked=false;
            k->_hdr->n_bytes = 0;
            _pool.push_back(k);
        }
    }

    void reset()
    {
        std::lock_guard<std::mutex> guard(_m);
        while(_queue.size())
        {
            Chunk* k = _queue.front();
            _queue.pop_front();
            k->_hdr->locked=false;
            _pool.push_back(k);
        }
    }
    size_t length()const{
        std::lock_guard<std::mutex> guard(_m);
        return _queue.size();
    }

    Chunk*  obtain()
    {
        if(_pool.size())
        {
            std::lock_guard<std::mutex> guard(_m);
            Chunk* p = _pool.back();
            p->_hdr->locked=true;
            _pool.pop_back();
            return p;
        }
        return nullptr;
    }

    size_t optim()const{
        std::lock_guard<std::mutex> guard(_m);
        return _optim;
    }

    void q_feed(Chunk* p)
    {
        std::lock_guard<std::mutex> guard(_m);
        _queue.push_back(p);
        if(_block_get)
        {
            _block_get = _queue.size() < _optim;
        }
    }

    Chunk* q_consume()
    {
        std::lock_guard<std::mutex> guard(_m);
        if(_queue.size() && !_block_get)
        {
            Chunk* pk = _queue.front();
            _queue.pop_front();
            return pk;
        }
        return nullptr;
    }

    void release(Chunk* p)
    {
        std::lock_guard<std::mutex> guard(_m);
        p->_hdr->locked=false;
        _pool.push_back(p);
    }


    void force_optim(int optlen)
    {
        size_t newoptim = optlen; // (optlen * _maxqsz) / 100 ;
        if(_optim != newoptim)
        {
            std::lock_guard<std::mutex> guard(_m);
            _optim = newoptim;
            if(_queue.size() > _optim)
            {
                //drop some fames
                while(_queue.size() > _optim)
                {
                    Chunk* k = _queue.front();
                    _queue.pop_front();
                    k->_hdr->locked=false;
                    k->_hdr->n_bytes = 0;
                    _pool.push_back(k);
                }
            }
            else
            {
                //wait for be filled up to
                _block_get=true;
            }
        }
    }


private:
    std::vector<Chunk*>  _pool;
    std::deque<Chunk*>   _queue;
    size_t               _chunksz;
    size_t               _maxqsz;
    mutable std::mutex   _m;
    int                  _full=Q_ALMOST_FULL;
    size_t               _optim=OPTIM_QUEUE_LEN_H;
    bool                 _block_get = false;
};


class CliThread
{
public:
    CliThread(Qbuff* qbuff);//Qbuff* qbuff
    virtual ~CliThread();//Qbuff* qbuff
    void _tmain();
    int srv_skew()const{
        std::lock_guard<std::mutex> guard(_mut);
        return _frmdiff;
    }
    void setpilot(bool pilot){
        std::lock_guard<std::mutex> guard(_mut);
        _pilot = pilot;
    }
    bool is_pilot()const {
        std::lock_guard<std::mutex> guard(_mut);
        return _pilot;
    }

    bool run(size_t chunk_ms);

private:
    bool _connecting(udp_group_sock& m,
                     tcp_cli_sock& tcp);
    bool  _store_qos();
    void _reply_qos(tcp_cli_sock& tcp);
    bool _reconnect(tcp_cli_sock& tcp, udp_group_sock& m);
    bool _netReceive(tcp_cli_sock& tcp, udp_group_sock& m, int sel);
    void _netSend(tcp_cli_sock& tcp, int& onceinawhile);

private:
    uint8_t* _drainbuff=nullptr;
    Qbuff*   _pq = nullptr;
    BufHdr   _hdr;
    uint64_t _ping_us     = 0;
    uint64_t _max_ping    = 0;
    uint64_t _requed_time = 0;
    uint32_t _max_frame   = 0;
    int32_t  _frmdiff    = 0;
    int      _cli_skew    = 0;
    time_t   _reque_time  = 0;
    int64_t  _frm_ms;
    uint64_t _now = 0;
    uint32_t _cur_frame   = 0;
    bool     _pilot = false;
    std::thread*  _pt = nullptr;
    mutable std::mutex _mut;
public:
    int      _index;
};


#define CLOSE_BY(a,b,c) ((a > b-c) && (a<b+c))

#endif // RECTHraccess_H

#ifndef SRVQ_H
#define SRVQ_H


#include <inttypes.h>
#include <cstring>
#include <cassert>
#include <vector>
#include <mutex>
#include "screenxy.h"

#define ONE_SEC         1000000
#define HALF_SEC        500
#define QOS_TO          250000
#define SYNC_DATA       1
#define ALIVE_DATA      2
#define RESET_ALL       3


#define     TCP_SND_LEN     4096
#define     SRV_CHUNKS      32

#define     BITS_AO         16
#define     SAMPLE_AO       44100
#define     CHANNELS_AO     2
#define     MAX_SKEW        8

#define     SPK_SRV_PORT   9980

#define     CLI_SYNC_PERCENT    50
#define     Q_ALMOST_FULL       98
#define     Q_ALMOST_EMPTY      4
#define     Q_MIN_PLAY          8

#define MAX_REQUE_INTERVAL  30
#define SYNCHED_DELTA       4

#define     SLOW_FLAG 0x1
#define     FAST_FLAG 0x2

template <typename T> T CLAMP(const T& value,
                              const T& low,
                              const T& high)
{
    return value < low ? low : (value > high ? high : value);
}

#define PIPE_IN      "/tmp/speakers"

// play buffer header
typedef struct
{
    uint8_t   frm_sig;        // SYNC_DATA,...
    uint32_t  cur_seq;        // 0 1...
    bool      is_pilot;
    uint64_t  cli_ping;       // roun trip tick_count()-srv_tick
    uint64_t  srv_tick;       // curent time
    uint64_t  pilot_ping;     // max cli_ping among all clients
    uint32_t  pilot_seq;      // all cients lazy player sequece played

    uint16_t  cli_qlen;
    uint16_t  cli_qsize;
    uint16_t  cli_qoptim;
    uint16_t  pilot_q_perc;

    int       buf_len;          // play buf_len buffer length
    int16_t   ao_rate;          // allways 2 channels, allways 16 bits
}  __attribute__ ((packed)) BufHdr ;

#define TCP_MTU TCP_SND_LEN+sizeof(BufHdr)

// circular buffer.
class Circle
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
        uint8_t*  buff()const{return _playbuff;}
        uint8_t*  hdr()const{return (uint8_t*)_hdr;}
        int       size()const{return _sz;}
        int       len()const{return _sz-sizeof(BufHdr);}
        BufHdr*   _hdr=nullptr;
        uint8_t*  _playbuff=nullptr;
        int       _sz=0;
        int       _idx=0;
        bool      _locked=false;
    };

    Circle():_scrap(nullptr){}

    ~Circle()
    {
        reset();
        for(auto& a : _queue){
            delete a;
        }
        delete _scrap;
    }

    size_t capacity()const
    {
        return _maxqsz;
    }

    // reserves the storage
    void reserve(int chunks, int chunk){
        _maxqsz = chunks;
        _chunksz = chunk;
        assert(_queue.size()==0);
        for(int i=0;i<chunks;i++)
        {
            Chunk* k = new  Chunk(chunk);
            k->_idx=i;
            k->_locked = false;
            _queue.push_back(k);
        }
        _scrap = new Chunk(chunk);
    }

    int calc_optim(int optim)
    {
        return (optim * _maxqsz) / 100 ;
    }


    //sets the optim q len while playing
    bool set_optim_perc(int optim){
        std::lock_guard<std::mutex> guard(_m);
        size_t newoptim = (optim * _maxqsz) / 100 ;
        if(newoptim != _optim)
        {
            _optim = newoptim;
            return true;
        }
        return false;
    }



    int maxsz()const{return _maxqsz;}
    int payload()const{ return _chunksz;}
    size_t percent()const{
        std::lock_guard<std::mutex> guard(_m);
        return (_elems*100) / (_maxqsz+1);
    }

    //remove form back
    void remove_back(int els)
    {
        std::lock_guard<std::mutex> guard(_m);
        while(els && _elems)
        {
            --_elems;
            --els;
            _inc(_geti);
        }
    }

    // all reset
    void reset()
    {
        std::lock_guard<std::mutex> guard(_m);
        _geti = 0;
        _puti = 0;
        _elems = 0;
    }

    size_t optim()const {
        std::lock_guard<std::mutex> guard(_m);
        return _optim;
    }

    size_t length()const {
        std::lock_guard<std::mutex> guard(_m);
        return _length();
    }

    void release(Chunk* &pk)
    {
        if(pk)
        {
            std::lock_guard<std::mutex> guard(_m);
            pk->_locked=false;
            pk = nullptr;
        }
    }

    Chunk*  q_feedup(bool scrap)
    {
        std::lock_guard<std::mutex> guard(_m);

        if(_elems<=(_maxqsz))
        {
            if(_queue[_puti]->_locked==false)
            {
                size_t c = _puti;
                _inc(_puti);
                ++_elems;
                _queue[c]->_locked=true;
                if(_block_get)
                {
                    _block_get = (_elems < _optim);
                }
                return _queue[c];
            }
        }
        if(scrap){
            return _scrap;
        }
        return nullptr;
    }

    Chunk* get_off_put(int index)const
    {
        std::lock_guard<std::mutex> guard(_m);
        int mindex = (_puti + index) % _maxqsz;
        if(_queue[mindex]->_locked==false)
        {
            _queue[mindex]->_locked=true;
            return _queue[mindex];
        }
        return nullptr;
    }

    Chunk* q_consume()
    {
        std::lock_guard<std::mutex> guard(_m);

        if(_elems && !_block_get)
        {
            size_t c = _geti;
            if(_queue[c]->_locked==false)
            {
                _inc(_geti);
                --_elems;
                _queue[c]->_locked=true;
                return _queue[c];
            }
        }
        if(_block_get)
        {
            _block_get = (_elems < _optim);
        }
        return nullptr;
    }

    Chunk const* pick_get()
    {
        std::lock_guard<std::mutex> guard(_m);
        if(_elems)
        {
            return _queue[_geti];
        }
        _scrap->_locked=true;
        return _scrap;
    }

    Circle::Chunk* dummy(){
        _scrap->_locked=true;
        return _scrap;
    }
    size_t get_optim()const{return _optim;}

    void force_optim(int optlen)
    {
        size_t newoptim = optlen;
        if(newoptim >= _maxqsz)
            newoptim = _maxqsz-_maxqsz/20; // cannot
        if(_optim != newoptim)
        {
            std::lock_guard<std::mutex> guard(_m);
            _optim = newoptim;
            if(_elems > _optim)
            {
                //drop some fames
                while(_elems > _optim)
                {
                    _inc(_geti);
                    --_elems;
                }
            }
            else
            {
                //wait for be filled up to
                _block_get=true;
            }
        }
    }

    void display()
    {
        char ql[512] = {0};
        std::lock_guard<std::mutex> guard(_m);
        for(size_t idx=0;idx<_maxqsz;++idx)
        {
            if(_puti==idx)
                ql[idx]='p';
            else if(_geti==idx)
                ql[idx]='g';
            else if (_puti > _geti){
                if(idx<=_puti && idx>=_geti)
                    _queue[idx]->_locked ? ql[idx]='X' :   ql[idx]='x' ;
                else
                    _queue[idx]->_locked ? ql[idx]='^' :   ql[idx]='.' ;
            }
            else if(_puti < _geti){
                if(idx<=_puti || idx>=_geti)
                    _queue[idx]->_locked ? ql[idx]='X' :   ql[idx]='x' ;
                else
                    _queue[idx]->_locked ? ql[idx]='^' :   ql[idx]='.' ;
            }else {
                if(_elems)
                    _queue[idx]->_locked ? ql[idx]='X' :   ql[idx]='x' ;
                else
                    _queue[idx]->_locked ? ql[idx]='^' :   ql[idx]='.' ;
            }
        }
        TERM_OUT(true,MSG_L5,"%s, %d",ql,0);
    }

private:
    inline void _inc(size_t& e)const{
        ++e%=_maxqsz;
    }
    inline size_t _length()const{
        return _elems;
    }

private:
    Chunk*              _scrap=nullptr;
    std::vector<Chunk*>  _queue;
    mutable std::mutex  _m;
    size_t              _chunksz=0;
    size_t              _maxqsz=0;
    size_t              _optim=0;
    size_t              _puti = 0;
    size_t              _geti = 0;
    size_t              _elems = 0;
    bool                _block_get = false;
};



#include <time.h>

extern bool __alive;
inline size_t tick_count(size_t nsecs = 1000000)
{
    static size_t tc = 0;
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC,&ts) != 0) {
        assert(0);
        return tc;
    }
    tc = ts.tv_sec*1000+ts.tv_nsec/nsecs;
    return tc;
}



#endif // SRVQ_H

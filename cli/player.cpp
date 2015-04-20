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
#include "player.h"
#include "srvq.h"
#include "main.h"
#include "screenxy.h"
#include "sclient.h"
#include "pider.h"
#ifdef TARGET_ANDROID
#include "dialog.h"
#endif

// player ctor
player::player(Circle* rq, sclient* pc):_rq(rq),_pc(pc)
{
    // ao lib
    _ao = new AoCls(BITS_AO, SAMPLE_AO, CHANNELS_AO);

}

void player::start()
{

}


size_t player::get_len_for(int secs)const
{
    return _ao->init_ao(secs);
}

player::~player()
{
    delete _ao;
}

size_t player::_fill_q(bool& filling)
{
    size_t qoptim = _rq->get_optim();
    if(_rq->length()==0)
    {
        filling = true;
    }
    if(filling)
    {
        if(_rq->length()>qoptim)
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

void player::_play(Circle::Chunk*  pk)
{
    if(_ao && pk->_hdr->buf_len )
    {
        _ao->play(pk->payload(), pk->_hdr->buf_len);
    }

}

// loops until alive
#ifdef TARGET_ANDROID
bool player::play(sclient& client, Dialog* pd)
#else
bool player::play(sclient& client)
#endif
{
    Circle::Chunk*  pk;
    size_t          bytesec,curbps = 0;
    int             played = 0;
    size_t          qoptim = 0;
    size_t          st = tick_count();
    size_t          now=st,playspeed,curps;
    int             qlen=0,qlenskew=0;
    BufHdr          hdr;
    bool            filling = true;
    int             fillingsz = Q_ALMOST_EMPTY;

#ifdef TARGET_ANDROID
    _pd = pd;
#endif
    while(__alive)
    {
        now = tick_count();
        qlen = _rq->length();
        if(qlen < fillingsz || filling)     // filling empty q
        {
            fillingsz = _fill_q(filling);
            ::usleep(0x1FF);
            continue;
        }
        fillingsz = Q_ALMOST_EMPTY;

        if(nullptr!=(pk = _rq->q_consume()))
        {
            ::memcpy(&hdr, pk->_hdr, sizeof(BufHdr));
            /////////////////////////////////////////////////////
            _play(pk);
            pk->_hdr->buf_len = 0;
            pk->_hdr->frm_sig = 0;
            _rq->release(pk);
        }
        else
        {
            TERM_OUT(false, ERR_L, " NO STREAM");
            ::usleep(0xFFF);
            continue;
        }
        client.update_frame(hdr);

        if(now - st > ONE_SEC)                // quality of service once 100 ms second
        {
            st        = now;
            curbps    = bytesec;
            curps     = playspeed;
            playspeed = 0;
            bytesec   = 0;
            _display(curbps,curps,qoptim,qlen,qlenskew,hdr);
        }
        _rq->display();
    }
    return played!=0;
}

void player::_display(size_t curbps,    // bps
                      size_t curps,     // play speed time
                      size_t qoptim,    // optim q
                      int qerc,         // fill %
                      int qskew,        // where is q
                      const BufHdr& h)  // last frm
{
#ifdef TARGET_ANDROID
    Q_UNUSED(curbps);
    Q_UNUSED(curps);
    Q_UNUSED(qerc);
    Q_UNUSED(h);
    _pd->update_ui(_rq->length(), qoptim, _rq->maxsz(), qskew);
#else
    TERM_OUT(true,5,">BPS: %06d PLSPEED: %06d QSZ: %03d / %03d " , int(curbps), int(curps), _rq->maxsz(), _rq->length());
    TERM_OUT(true,6,">OPTLEN: %06d QPERC: %06d SKEW: %06d ",int(qoptim), qerc, qskew);
    TERM_OUT(true,7,">FRM: %06d FRMDIF: %06d  PING: %06d MAX_PING: %d ",h.cur_seq,
                                                                 h.pilot_seq-h.cur_seq,
                                                                 h.cli_ping,
                                                                 h.pilot_ping);
#endif
}




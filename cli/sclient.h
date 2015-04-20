/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/
#ifndef SCLIENT_H
#define SCLIENT_H

#include <string>
#include <thread>
#include <set>
#include <mutex>
#include "srvq.h"
#include "sock.h"



class Circle;
class sclient : public tcp_cli_sock
{
public:
    explicit sclient(Circle* qb, const std::set<std::string>& srvips);
    ~sclient();
    bool run(size_t);
    void update_frame(const BufHdr& cur);
    int get_rewind()const {
        std::lock_guard<std::mutex> guard(_mut);
        return _rewind;
    }
    useconds_t tune_ms()const{return _tuneup;}
    size_t bps()const{
        std::lock_guard<std::mutex> guard(_mut);
        return _bps;
    }
private:
    void _tmain();
    void _get_data();
    bool _check_stream(Circle::Chunk* pk, bool drop=false);

private:
    mutable std::mutex      _mut;
    Circle*                  _rq;
    const std::set<std::string>&  _ips;
    std::thread*            _pt = nullptr;
    BufHdr                  _qos;
    useconds_t              _tuneup = 0xFFF;
    uint32_t                _ping = 0;
    size_t                  _pingdiff = 0;
    int                     _frmdiff = 0;
    size_t                  _tperchunk = 0;
    size_t                  _rewind;
    size_t                  _now,_fut;
    time_t                  _reque_time = 0;
    int                     _bps = 0;
    bool                    _srvsync = false;
    int                     _percs = 0;
    int                     _phits = 0;
    bool                    _first_time = false;
};

#endif // SCLIENT_H

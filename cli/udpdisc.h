/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/
#ifndef UDPDISC_H
#define UDPDISC_H
#include <stdlib.h>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <resolv.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#ifdef TARGET_ANDROID
#include <ifaddrs.h>
#else
#include <linux/if.h>
#endif
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <iostream>


class Udper
{
    std::string _iface;
    std::string _ip;

public:
    explicit Udper(std::string& iface);
    ~Udper();
    int discover();
    const std::set<std::string>& rem_ips()const{return _remips;}
    void q_consume(std::set<std::string>& cop);
    static void join(std::set<std::string>& ifs);
    static bool get_local_ifaces(std::set<std::string>& ifaces);
private:
    bool _broad();
    void _reuse();
    bool _create(int& s);
    void _destroy();
    int _is_signaled(int s);
    bool _bind_to_addr(int s, struct sockaddr_in& skaddr, int addr = INADDR_ANY, int port=0);

private:
    int          _sock;
    int          _rock;

    std::set<std::string>   _remips;
    sockaddr_in             _srv_addr;
};


#endif // UDPDISC_H

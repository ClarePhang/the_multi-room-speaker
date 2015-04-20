/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/

#include <set>
#include "udpdisc.h"
#include "main.h"

#define SRV_PORT  19211
#define CLI_PORT  19212
#define PACKET_LEN  32
#define SPK_DISC    "XYZK"

void Udper::join(std::set<std::string>& ipis)
{
    std::set<std::string> ifs;
    std::vector<Udper*>  udps;

    ifs.clear();
    Udper::get_local_ifaces(ifs);

    if(ifs.size()==0)
    {
        ifs.insert("wlan0");
        ifs.insert("wlan1");
        ifs.insert("eth0");
        ifs.insert("eth1");
    }

    std::set<std::string>::iterator i = ifs.begin();
    for(; i!=ifs.end(); ++i)
    {
        std::string is = *i;
        Udper* pu = new Udper(is);
        udps.push_back(pu);
    }

    std::vector<Udper*>::iterator u ;
    for(int k=0; k<4; ++k)
    {
        u = udps.begin();
        for(; u!=udps.end(); ++u)
        {
            Udper* pu = (*u);
            int rk = pu->discover();
            if(rk>0)
            {
                pu->q_consume(ipis);
            }
        }
    }

    u = udps.begin();
    for(; u!=udps.end(); ++u)
    {
        delete *u;
    }
}

void Udper::q_consume(std::set<std::string>& cop)
{
    for(const auto& a : _remips)
        cop.insert(a);
}

int Udper::discover()
{
    if(_sock==0)
    {
        if(_create(_sock))
        {
            if(!_broad()){
                std::cout  << "cannot create broad:" << errno << "\n";
                return -1;
            }
        }
        else{
            std::cout << "cannot create l socket:" << errno << "\n";
            return -1;
        }
        std::cout << "try port:" <<    SRV_PORT <<" on "<< _iface <<"\n";
    }
    if(_rock==0)
    {
        if(_create(_rock))
        {
            if(!_bind_to_addr(_rock, _srv_addr,INADDR_ANY, CLI_PORT))
            {
                std::cout << "cannot bind:" << errno << "\n";
                return -1;
            }
        }
        else{
            std::cout << "cannot create r socket:" << errno << "\n";
            return -1;
        }
    }

    // broadcast on _sock
    struct sockaddr_in skaddr;

    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = INADDR_BROADCAST;
    skaddr.sin_port = htons(SRV_PORT);


    ::sendto(_sock,SPK_DISC,::strlen(SPK_DISC),0,(struct sockaddr*) &skaddr,sizeof(skaddr));
    for(int k=0; k<64; ++k)
    {
        if(k%4==0)
            ::sendto(_sock,SPK_DISC,::strlen(SPK_DISC),0,(struct sockaddr*) &skaddr,sizeof(skaddr));

        //_wait on _rock
        int n = _is_signaled(_rock);
        if(n==-1)
        {
            break;
        }
        if(n>0)
        {
            char                buff[128];
            struct sockaddr_in  remote;
            socklen_t           len = sizeof(remote);
            n=::recvfrom(_rock,buff,sizeof(buff)-1,0,(struct sockaddr *)&remote,&len);
            if(n>0)
            {
                buff[n] = 0;
                if(_remips.find(buff)==_remips.end())
                {
                    char* pip = strchr(buff,':');
                    if(pip)
                    {
                        _remips.insert(pip+2);
                    }
                }
            }
        }
        usleep(0xFFF);
        //++progress;
        //pd->ui->progressBar->setValue(progress);
    }

    return _remips.size();
}

Udper::Udper(std::string& iface):_sock(0),_rock(0)
{
    char loco[128];
    ::strcpy(loco,iface.c_str());
    if(strchr(loco,','))
    {
        _ip=::strtok(loco,",");
        _iface=::strtok(0,",");
    }
}

Udper::~Udper()
{
    _destroy();
}


bool Udper::_broad()
{
    int optval=1;
    socklen_t slen = sizeof(optval);
    return -1!=setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, &optval, slen);
}

void Udper::_reuse()
{
    int optval=1;
    socklen_t slen = sizeof(optval);
    setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , slen);
}

bool Udper::_create(int& s)
{
    s = socket(AF_INET, SOCK_DGRAM, 0);
    return  s>0;
}


void Udper::_destroy()
{
    if(_sock>0)
    {
        ::shutdown(_sock, 0x3);
        ::close(_sock);
    }
    _sock=0;
    if(_rock>0)
    {
        ::shutdown(_rock, 0x3);
        ::close(_rock);
    }
    _rock=0;
}

int Udper::_is_signaled(int s)
{
    timeval  tv   = {0, 1000};
    fd_set   rd_set;

    FD_ZERO(&rd_set);
    FD_SET(s, &rd_set);
    int nfds = (int)s+1;
    int sel = ::select(nfds, &rd_set, 0, 0, &tv);
    if(sel<0)
    {
        perror ("select");
        return -1;
    }
    return sel > 0 && FD_ISSET(s, &rd_set);

}

bool Udper::_bind_to_addr(int s, struct sockaddr_in& skaddr,
                          int addr, int port)
{
	const int trueFlag = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int));
	setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &trueFlag, sizeof(int));

    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = htonl(addr);
    skaddr.sin_port = htons(port);
    if (bind(s, (struct sockaddr *) &skaddr, sizeof(skaddr))<0)
    {
        printf("bind on port error %d\n",port);
        perror("bind ");
        return false;
    }
    return true;
}



bool Udper::get_local_ifaces(std::set<std::string>& ifaces)
{
#ifndef TARGET_ANDROID
    char    tmp[128];
    struct  ifaddrs* ifAddrStruct = 0;
    void    *tmpAddrPtr = 0;

    ifaces.clear();
    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct != 0)
    {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET &&
                strncmp(ifAddrStruct->ifa_name, "lo", 2)!=0)
        {
            memset(tmp,0,sizeof(tmp));
            tmpAddrPtr = (void*)&((sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, tmp, (sizeof(tmp)-1));
            strcat(tmp,",");
            strcat(tmp,ifAddrStruct->ifa_name);
            ifaces.insert(tmp);
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    freeifaddrs(ifAddrStruct);
#else
    (void)(ifaces);
#endif
    return true;
}

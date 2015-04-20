/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/

#include "udpdisc.h"
#include "main.h"

#define SPK_SRV_PORT  19211
#define CLI_PORT  19212
#define PACKET_LEN  32
#define SPK_DISC    "XYZK"

void Udper::tmain()
{
    std::vector<Udper*>  udps;
    std::set<std::string>  ifs;

    int hmin=0xFFFF;
    while(__alive)
    {
        if((hmin & 0xFFFF)==0xFFFF)
        {
            Udper::re_interface(udps,ifs);
        }
        std::vector<Udper*>::iterator u = udps.begin();
        for(; u!=udps.end(); ++u)
        {
            Udper* pu = (*u);
            pu->listen_and_respond();
        }
        usleep(0x3FFF);
        ++hmin;
    }

    std::vector<Udper*>::iterator u = udps.begin();
    for(; u!=udps.end(); ++u)
    {
        delete *u;
    }
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
        std::cout << "try port:" <<    SPK_SRV_PORT <<" on "<< _iface <<"\n";
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
    skaddr.sin_port = htons(SPK_SRV_PORT);


    ::sendto(_sock,SPK_DISC,::strlen(SPK_DISC),0,(struct sockaddr*) &skaddr,sizeof(skaddr));
    for(int k=0; k<512; ++k)
    {
        if(k%8==0)
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
                    _remips.insert(buff);
                }
            }
        }
        usleep(1024);
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


bool Udper::get_local_ifaces(std::set<std::string>& ifaces)
{
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
    return true;
}

int Udper::listen_and_respond()
{
    if(_sock==0)
    {
        if(_create(_sock))
        {
            _reuse();
            if(!_bind_to_addr(_sock, _srv_addr,INADDR_ANY, SPK_SRV_PORT))
                return -1;
        }
        else
            return -1;
        //std::cout << "watching for net on port:" <<    SPK_SRV_PORT << "\n";
    }
    int s = _is_signaled(_sock);
    if(s>0)
    {
        char                buff[128];
        struct sockaddr_in  remote;
        socklen_t           len = sizeof(remote);
        char                hostname[256]= {0};
        char                out[256]= {0};

        gethostname(hostname, 255);

        sprintf(out,"%s: %s", hostname, _ip.c_str());
        int n=::recvfrom(_sock,buff,sizeof(buff)-1,0,(struct sockaddr *)&remote,&len);
        if(n>0)
        {
            buff[n] = 0;
            //std::cout << "received: " << buff << "\n";
            if(!strcmp(buff,SPK_DISC))
            {
                remote.sin_port = htons(CLI_PORT);
                n=::sendto(_sock,out,::strlen(out),0,(struct sockaddr *)&remote,len);
                if(n==(int)_ip.length())
                {
                    // std::cout<<"ip:" << _ip << " was sent back \n";
                }
            }
        }
    }
    else if(s<0)
        return -1;
    return 0;
}

const std::set<std::string>& Udper::rem_ips()const
{
    return _remips;
}

void Udper::re_interface(std::vector<Udper*>& udps, std::set<std::string>&  ifs)
{
    static struct stat prev={};
    struct stat cur={};

    ::stat("/var/log/syslog", &cur);
    if(cur.st_mtime != prev.st_mtime)
    {
        Udper::get_local_ifaces(ifs);
        std::vector<Udper*>::iterator u = udps.begin();
        for(; u!=udps.end(); ++u)
        {
            delete *u;
        }
        udps.clear();
        std::set<std::string>::iterator i = ifs.begin();
        for(; i!=ifs.end(); ++i)
        {
            std::string is = *i;
            //std::cout << "Adding udp:" << is << "\n";
            Udper* pu = new Udper(is);
            udps.push_back(pu);
        }
        prev.st_mtime=cur.st_mtime;
        sleep(1);
    }
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

void Udper::_bind_to_iface()
{
    struct ifreq interface;
    memset(&interface, 0, sizeof(interface));
    ::strncpy(interface.ifr_ifrn.ifrn_name, _iface.c_str(), IFNAMSIZ);
    if (-1 == setsockopt(_sock,
                         SOL_SOCKET,
                         SO_BINDTODEVICE,
                         &interface,
                         sizeof(interface)))

    {
        perror("bind 2 dev");
    }
}

bool Udper::_bind_to_addr(int s, struct sockaddr_in& skaddr,
                          int addr, int port)
{
    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = htonl(addr);
    skaddr.sin_port = htons(port);
    if (bind(s, (struct sockaddr *) &skaddr, sizeof(skaddr))<0)
    {
        perror("bind");
        return false;
    }
    return true;
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


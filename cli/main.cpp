/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/

#include <string>
#include <iostream>
#include <thread>
#include "udpdisc.h"
#include "sclient.h"
#include "srvq.h"
#include "player.h"
#include "screenxy.h"

bool __alive = true;


int main(int argc, char* argv[])
{
    UNUS(argc);
    UNUS(argv);
    screenxy    t(argc,argv);  // singleton to Shell 
    bool        serveron = false;
    std::set<std::string> ifs;

    while(__alive)
    {
        if(!serveron)
        {
            Udper::join(ifs);       // search servers
            serveron = ifs.size()>0;
        }
        if(serveron)
        {
            Circle        qb;                   // play queue
            sclient       client(&qb, ifs);     // client thread to queue
            player        player(&qb, &client); // player main thread from queue
            const size_t bl =  player.get_len_for(4);               // allocate 2 seconds play
            const size_t chunks = (bl+TCP_SND_LEN) / TCP_SND_LEN;   // round it up
            const size_t tper_chunk = 1000/chunks;

            Term->pc(true,MAIN_L,"PLAY QUEUE: %d OF %d BYTES", chunks, TCP_SND_LEN);
            qb.reserve(chunks, TCP_SND_LEN);    // reserve the queue
            qb.set_optim_perc(CLI_SYNC_PERCENT);
            player.start();

            if(client.run(tper_chunk))          // tcp thread
            {
                player.play(client);            // main loop
            }
        }
    }
    return 0;
}

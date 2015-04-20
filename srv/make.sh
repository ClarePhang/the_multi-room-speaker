#!/bin/bash

#   Marius C. O. (circinusX1) all rights reserved
#   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
#   1. Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#   2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.


rm -rf  *.o
rm -rf  ../common/*.o
rm -rf ./tcpspksrv
# install glut gl and mesa 

g++ *.cpp ../common/*.cpp -DGL_DISPLAY  \
-I../common  \
-lncurses -lpthread -lGL -lglut -lGLU -o  \
./tcpspksrv

# uncomment for no GL PID window
# g++ *.cpp ../common/*.cpp  -I../common  -lncurses -lpthread -o ./tcpspksrv

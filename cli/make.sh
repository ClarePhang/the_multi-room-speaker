#!/bin/bash
#
#   Marius C. O. (circinusX1) all rights reserved
#   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
#   1. Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#   2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.


rm -rf  *.o
rm -rf ./tcpspkcli

g++ *.cpp ../common/*.cpp -std=c++14 -I../common -lncurses -lmpg123 -lao -lpthread -o ./tcpspkcli

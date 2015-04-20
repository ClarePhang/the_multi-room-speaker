cmake_minimum_required(VERSION 3.18)

PROJECT(spkserver)

ADD_DEFINITIONS(-DGL_DISPLAY)

add_executable (spkserver main.cpp 
../common/alsacls.cpp 
../common/aocls.cpp 
../common/aoutil.cpp 
../common/tcpsrv.cpp 
../common/clithread.cpp 
../common/cpipein.cpp 
../common/pidaim.cpp 
../common/mp3cls.cpp 
../common/screenxy.cpp 
../common/sock.cpp)
SET( CMAKE_CXX_FLAGS -std=c++11)
include_directories (./../common ./ )
TARGET_LINK_LIBRARIES(spkserver pthread ao mpg123 asound ncurses GL GLU glut)



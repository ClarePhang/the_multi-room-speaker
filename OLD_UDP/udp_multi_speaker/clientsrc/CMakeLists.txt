
PROJECT(spkclient)
add_definitions( -DAO_LIB )

add_executable (spkclient main.cpp 
../common/alsacls.cpp
../common/aocls.cpp
../common/aoutil.cpp
../common/screenxy.cpp
../common/pidaim.cpp
../common/clithread.cpp
../common/cpipein.cpp
../common/mp3cls.cpp
../common/sock.cpp)

SET( CMAKE_CXX_FLAGS -std=c++11)
include_directories (./../common ./ )
TARGET_LINK_LIBRARIES(spkclient pthread ao mpg123 asound ncurses)



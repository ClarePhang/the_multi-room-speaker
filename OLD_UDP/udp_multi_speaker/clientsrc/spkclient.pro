#QT += core
QT -= gui

TARGET = spkclient
CONFIG += console
CONFIG -= app_bundle
QMAKE_CXXFLAGS +=  -std=c++11 -Wno-unused-paramater
TEMPLATE = app
INCLUDEPATH = ./../common ./
DEFINES += AO_LIB
# DEFINES += PULSE_AUDIO
SOURCES += main.cpp \
    ../common/alsacls.cpp \
    ../common/aocls.cpp \
    ../common/aoutil.cpp \
    ../common/clithread.cpp \
    ../common/cpipein.cpp \
    ../common/mp3cls.cpp \
    ../common/pidaim.cpp \
    ../common/pulsecls.cpp \
    ../common/screenxy.cpp \
    ../common/sock.cpp
HEADERS += \
    ../common/alsacls.h \
    ../common/aocls.h \
    ../common/aoutil.h \
    ../common/clithread.h \
    ../common/cpipein.h \
    ../common/main.h \
    ../common/mp3cls.h \
    ../common/pidaim.h \
    ../common/pulsecls.h \
    ../common/screenxy.h \
    ../common/sock.h
#sudo apt-get install libmpg123-dev libao-dev





LIBS +=  -lasound -lpulse -lpulse-simple -lmpg123 -lao -lncurses

#QT += core
QT -= gui

TARGET = spkserver
CONFIG += console
CONFIG -= app_bundle
QMAKE_CXXFLAGS +=  -std=c++11 -Wno-unused-paramater
TEMPLATE = app
INCLUDEPATH = ./../common ./

DEFINES += QT_DEPRECATED_WARNINGS GL_DISPLAY

SOURCES += main.cpp \
    ../common/alsacls.cpp \
    ../common/aocls.cpp \
    ../common/aoutil.cpp \
    ../common/pidaim.cpp \
    ../common/screenxy.cpp \
    ../common/tcpsrv.cpp \
    ../common/cpipein.cpp \
    ../common/mp3cls.cpp \
    ../common/sock.cpp

HEADERS += \
    ../common/alsacls.h \
    ../common/aocls.h \
    ../common/aoutil.h \
    ../common/clithread.h \
    ../common/pidaim.h \
    ../common/screenxy.h \
    ../common/tcpsrv.h \
    ../common/cpipein.h \
    ../common/main.h \
    ../common/mp3cls.h \
    ../common/sock.h

#sudo apt-get install libmpg123-dev libao-dev


LIBS += -lao
LIBS += -lmpg123 -lncurses

INCLUDEPATH += $$PWD/../../../../../usr/lib/x86_64-linux-gnu
DEPENDPATH += $$PWD/../../../../../usr/lib/x86_64-linux-gnu

LIBS +=  -lasound
LIBS += -lGL -lglut -lGLU

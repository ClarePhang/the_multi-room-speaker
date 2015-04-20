#-------------------------------------------------
#
# Project created by QtCreator 2020-10-28T11:18:51
#
#-------------------------------------------------
QT       += core gui
QT += opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tcpplayercli
TEMPLATE = app
# DEFINES += __ANDROID_API__=24
DEFINES += ANDROID_NDK_PLATFORM=android-19
DEFINES += TARGET_ANDROID

DEFINES += WITH_SDL

INCLUDEPATH += ../../../common
INCLUDEPATH += ../../../cli

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11



SOURCES += \
        ../../../common/sock.cpp \
        ../../pider.cpp \
        ../../player.cpp \
        ../../sclient.cpp \
        ../../udpdisc.cpp \
        main.cpp \
        dialog.cpp \
        tinyauan.cpp

HEADERS += \
        ../../../common/screenxy.h \
        ../../../common/sock.h \
        ../../../common/srvq.h \
        ../../pider.h \
        ../../player.h \
        ../../sclient.h \
        ../../udpdisc.h \
        dialog.h \
        main.h \
        tinyauan.h

FORMS += \
        dialog.ui

CONFIG += mobility
MOBILITY = 


target.path = /opt/$${TARGET}/bin
INSTALLS += target

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml



LIBS += -lOpenSLES -lGLESv1_CM

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

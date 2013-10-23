TEMPLATE = lib

CONFIG += qt dll
QT += core network
DEFINES += METADLL

ROOTDIR = ../..
MAINTARGET = metalib

DESTDIR = $$ROOTDIR/bin

CONFIG(debug, debug|release){
    DEFINES += DEBUG
    TARGET = $$MAINTARGET
}

CONFIG(release, debug|release){
    TARGET = $$MAINTARGET
    DEFINES += QT_NO_DEBUG_OUTPUT
}

SOURCES += \
    qtcptransport.cpp \
    qproxyobject.cpp \
    qmetahost.cpp \
    qmetaclient.cpp \
    qmetaevent.cpp

HEADERS += \
    qproxyobject.h \
    qmetahost.h \
    proto.h \
    global.h \
    qmetaclient.h \
    qtcptransport.h \
    qmetaevent.h

TEMPLATE = lib

CONFIG += qt dll
QT += core network
DEFINES += METADLL

ROOTDIR = $$PWD/../..
DEBUGTARGET = metalibd
RELEASETARGET = metalib

DESTDIR = $$ROOTDIR/lib
DLLDESTDIR = $$ROOTDIR/bin

CONFIG(debug, debug|release){
    DEFINES += DEBUG
    TARGET = $$DEBUGTARGET
}

CONFIG(release, debug|release){
    TARGET = $$RELEASETARGET
    DEFINES += QT_NO_DEBUG_OUTPUT
}

SOURCES += \
    qtcptransport.cpp \
    qproxyobject.cpp \
    qmetahost.cpp \
    qmetaclient.cpp \
    qmetaevent.cpp \
    paramholder.cpp

HEADERS += \
    qproxyobject.h \
    qmetahost.h \
    proto.h \
    global.h \
    qmetaclient.h \
    qtcptransport.h \
    qmetaevent.h \
    paramholder.h

include_files.path = $$ROOTDIR/include
include_files.files = $$HEADERS

INSTALLS += include_files

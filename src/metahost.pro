TEMPLATE = app

CONFIG += qt
QT += core

ROOTDIR = ..
MAINTARGET = metahost

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
	main.cpp \
	moctest.cpp \
    qproxyobject.cpp

HEADERS += \
	moctest.h \
    proto.h \
    qproxyobject.h
	
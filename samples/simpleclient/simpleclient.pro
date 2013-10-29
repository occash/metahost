TEMPLATE = app

CONFIG += qt
QT += core network

ROOTDIR = $$PWD/../..
DEBUGTARGET = simpleclientd
RELEASETARGET = simpleclient

DESTDIR = $$ROOTDIR/bin

INCLUDEPATH += $$ROOTDIR/include

LIBS += -L$$ROOTDIR/lib -lmetalib

CONFIG(debug, debug|release){
    DEFINES += DEBUG
    TARGET = $$DEBUGTARGET
}

CONFIG(release, debug|release){
    TARGET = $$RELEASETARGET
    DEFINES += QT_NO_DEBUG_OUTPUT
}

HEADERS += \
	clientwidget.h

SOURCES += \
	clientwidget.cpp \
        main.cpp

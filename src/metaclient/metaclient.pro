TEMPLATE = app

CONFIG += qt
QT += core network

ROOTDIR = ../..
MAINTARGET = metaclient

DESTDIR = $$ROOTDIR/bin

INCLUDEPATH += ../metalib

LIBS += -L$$ROOTDIR/bin -lmetalib

CONFIG(debug, debug|release){
    DEFINES += DEBUG
    TARGET = $$MAINTARGET
}

CONFIG(release, debug|release){
    TARGET = $$MAINTARGET
    DEFINES += QT_NO_DEBUG_OUTPUT
}

HEADERS += \
	clientwidget.h

SOURCES += \
	clientwidget.cpp \
    main.cpp

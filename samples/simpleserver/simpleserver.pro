TEMPLATE = app

CONFIG += qt
QT += core network

ROOTDIR = $$PWD/../..
DEBUGTARGET = simpleserverd
RELEASETARGET = simpleserver

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

SOURCES += \
    main.cpp \
    moctest.cpp \
    serverwidget.cpp

HEADERS += \
    moctest.h \
    serverwidget.h

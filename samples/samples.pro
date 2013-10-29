TEMPLATE = subdirs
SUBDIRS = \
    simpleserver \
    simpleclient

BINDIR = $$PWD/../bin

qtlibs.path = $$BINDIR
qtlibs.files = $$QMAKE_LIBDIR_QT
qtlibs.CONFIG = no_check_exist

INSTALLS += qtlibs

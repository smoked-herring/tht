TARGET = TickersDb
TEMPLATE = app

INCLUDEPATH += ../..
DEPENDPATH += ../..

QT += core gui network sql
QTPLUGIN += qico

!exists($${_PRO_FILE_PWD_}/../../THT.pro) {
    error("This is not a THT source tree, don't know what to do")
}

DEFINES += TICKERS_DIR=$$sprintf("\"\\\"%1\\\"\"", $${_PRO_FILE_PWD_}/../../tickersdb)
DEFINES += PRO_FILE_PWD=$$sprintf("\"\\\"%1\\\"\"", $$_PRO_FILE_PWD_)

SOURCES += main.cpp\
        widget.cpp \
    ../../csvreader.cpp \
    ../../networkaccess.cpp

HEADERS  += widget.h \
    ../../csvreader.h \
    ../../networkaccess.h

FORMS    += widget.ui

RESOURCES += \
    tickersdb.qrc

win32 {
    RC_FILE = tickersdb.rc
}
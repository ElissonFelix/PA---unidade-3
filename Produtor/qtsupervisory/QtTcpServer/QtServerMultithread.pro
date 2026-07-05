QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtServerMultithread
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    myserver.cpp \
    mythread.cpp \
    datastorage.cpp

HEADERS  += mainwindow.h \
    myserver.h \
    mythread.h \
    datastorage.h

FORMS    += mainwindow.ui

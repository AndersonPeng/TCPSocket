#-------------------------------------------------
#
# Project created by QtCreator 2017-03-23T16:27:35
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TCPSocket
#CONFIG += console

TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        droparea.cpp \
    tcpclient.cpp \
    tcpserver.cpp \
    tcpcommunicator.cpp

HEADERS  += mainwindow.h \
        droparea.h \
    tcpclient.h \
    tcpserver.h \
    tcpcommunicator.h

FORMS    += mainwindow.ui

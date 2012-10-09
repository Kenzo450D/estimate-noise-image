#-------------------------------------------------
#
# Project created by QtCreator 2012-10-04T02:18:45
#
#-------------------------------------------------

QT       += core gui

TARGET = NoiseEstimate
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

INCLUDEPATH += "/usr/include/opencv/"

CONFIG += link_pkgconfig
PKGCONFIG += opencv

LIBS += -Icv -Ihighgui

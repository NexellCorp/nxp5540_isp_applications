#-------------------------------------------------
#
# Project created by QtCreator 2016-10-19T13:34:16
#
#-------------------------------------------------

QT       += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = btree_ui
TEMPLATE = app

CONFIG += console \
          qml_debug

SOURCES += main.cpp\
        mainwindow.cpp \
        deviceioctl.c

HEADERS  += mainwindow.h \
        deviceioctl.h

FORMS    += mainwindow.ui

#-------------------------------------------------
#
# Project created by QtCreator 2016-12-09T14:38:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = nx_isp_viewer
TEMPLATE = app


SOURCES += \
    nx_main_window.cpp \
    nx_main.cpp \
    nx_regedit_dialog.cpp \
    nx_yuv_to_rgb.cpp \
    nx_buf_control.cpp \
    nx_func_control.cpp \
    nx_device_control.c

HEADERS  += \
    nx_main_window.cpp.autosave \
    nx_main_window.h \
    nx_regedit_dialog.h \
    nx_str_common.h \
    nx_yuv_to_rgb.h \
    nx_buf_control.h \
    nx_common_enum.h \
    nx_debug.h \
    nx_device_control.h \
    nx_func_control.h

FORMS    += \
    nx_main_window.ui \
    nx_regedit_dialog.ui

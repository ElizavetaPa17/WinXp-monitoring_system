#-------------------------------------------------
#
# Project created by QtCreator 2023-11-18T14:52:53
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Lab8
TEMPLATE = app

INCLUDEPATH += $$PWD/ \
DEPENDPATH += $$PWD/

win32:
    LIBS += -lpsapi \


SOURCES += main.cpp \
        mainwindow.cpp \
        winutility.cpp \
        qcustomplot.cpp \

HEADERS  += mainwindow.h \
        winutility.h \
        qcustomplot.h \

FORMS    += mainwindow.ui



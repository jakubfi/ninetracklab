#-------------------------------------------------
#
# Project created by QtCreator 2016-05-29T09:27:39
#
#-------------------------------------------------

QT       += core gui
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qv9ttd
TEMPLATE = app


SOURCES +=\
    aboutdialog.cpp \
    blockstore.cpp \
    decodernrz1.cpp \
    decoderpe.cpp \
    histogramdialog.cpp \
    histview.cpp \
    main.cpp \
    mainwindow.cpp \
    preprocessdialog.cpp \
    tapedrive.cpp \
    tapeview.cpp

HEADERS  += mainwindow.h \
    tapeview.h \
    tapedrive.h \
    aboutdialog.h \
    preprocessdialog.h \
    histogramdialog.h \
    histview.h \
    decodernrz1.h \
    decoderpe.h \
    blockstore.h

FORMS    += mainwindow.ui \
    aboutdialog.ui \
    preprocessdialog.ui \
    histogramdialog.ui

DISTFILES +=

RESOURCES += \
    resources.qrc

#-------------------------------------------------
#
# Project created by QtCreator 2016-05-29T09:27:39
#
#-------------------------------------------------

QT       += core gui
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ninetracklab
TEMPLATE = app


SOURCES +=\
    aboutdialog.cpp \
    blockstore.cpp \
    decodernrz1.cpp \
    decoderpe.cpp \
    histogramdialog.cpp \
    histview.cpp \
    main.cpp \
    preprocessdialog.cpp \
    tapedrive.cpp \
    tapeview.cpp \
    ninetracklab.cpp \
    tdconf.cpp

HEADERS  += \
    tapeview.h \
    tapedrive.h \
    aboutdialog.h \
    preprocessdialog.h \
    histogramdialog.h \
    histview.h \
    decodernrz1.h \
    decoderpe.h \
    blockstore.h \
    ninetracklab.h \
    tdconf.h

FORMS    += \
    aboutdialog.ui \
    preprocessdialog.ui \
    histogramdialog.ui \
    ninetracklab.ui

DISTFILES += \
    TODO \
    README.md

RESOURCES += \
    resources.qrc

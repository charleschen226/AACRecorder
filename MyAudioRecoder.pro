#-------------------------------------------------
#
# Project created by QtCreator 2016-07-20T11:33:18
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MyAudioRecoder
TEMPLATE = app


SOURCES += main.cpp\
        audiorecorder.cpp \
    audioprocess.cpp \
    libfaac/aacquant.c \
    libfaac/backpred.c \
    libfaac/bitstream.c \
    libfaac/channels.c \
    libfaac/fft.c \
    libfaac/filtbank.c \
    libfaac/frame.c \
    libfaac/huffman.c \
    libfaac/ltp.c \
    libfaac/midside.c \
    libfaac/psychkni.c \
    libfaac/tns.c \
    libfaac/util.c

HEADERS  += audiorecorder.h \
    audioprocess.h \
    libfaac/aacquant.h \
    libfaac/backpred.h \
    libfaac/bitstream.h \
    libfaac/channels.h \
    libfaac/coder.h \
    libfaac/faac.h \
    libfaac/faaccfg.h \
    libfaac/fft.h \
    libfaac/filtbank.h \
    libfaac/frame.h \
    libfaac/huffman.h \
    libfaac/hufftab.h \
    libfaac/ltp.h \
    libfaac/midside.h \
    libfaac/psych.h \
    libfaac/tns.h \
    libfaac/util.h \
    libfaac/version.h

FORMS    += audiorecorder.ui

DISTFILES +=

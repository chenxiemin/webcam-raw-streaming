######################################################################
# Automatically generated by qmake (2.01a) ?? 6? 11 18:52:00 2013
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += capture.h sender.h types.h vcompress.h \
    avilib.h
SOURCES += capture.cpp \
           vcompress.cpp \
    main.cpp \
    avilib.c

INCLUDEPATH += ../UsageEnvironment/include
INCLUDEPATH += ../groupsock/include
INCLUDEPATH += ../liveMedia/include
INCLUDEPATH += ../BasicUsageEnvironment/include
LIBS += -L"../groupsock/"
LIBS += -L"../UsageEnvironment/"
LIBS += -L"../liveMedia/"
LIBS += -L"../BasicUsageEnvironment/"
LIBS += -lgroupsock
LIBS += -lUsageEnvironment
LIBS += -lliveMedia
LIBS += -lliveMedia
LIBS += -lBasicUsageEnvironment
LIBS += -lswscale -lavutil -lavcodec -lavformat -lx264

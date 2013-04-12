#-------------------------------------------------
#
# Project created by QtCreator 2012-03-14T11:47:05
#
#-------------------------------------------------

QT       -= core gui

TARGET = GSEngineLib
TEMPLATE = lib

DEFINES += GAUSS_LIBRARY

SOURCES +=\
    src/workspacemanager.cpp \
    src/geworkspace.cpp \
    src/gesymtype.cpp \
    src/gesymbol.cpp \
    src/gearray.cpp \
    src/gematrix.cpp \
    src/gestring.cpp \
    src/gestringarray.cpp \
    src/gauss.cpp

HEADERS +=\
        GSEngineLib_global.h \
    src/workspacemanager.h \
    src/geworkspace.h \
    src/gesymtype.h \
    src/gesymbol.h \
    src/gearray.h \
    src/gematrix.h \
    src/gestring.h \
    src/gestringarray.h \
    src/gefuncwrapper.h \
    src/gauss.h

INCLUDEPATH += . \
                include \
                /home/matt/dev/mteng13_64 \
                /home/matt/dev/boost_1_49_0

LIBS += -L/home/matt/dev/mteng13_64 -lmteng

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE62770A3
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = GSEngineLib.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    gauss.i

#-------------------------------------------------
#
# Project created by QtCreator 2018-10-28T10:44:34
#
#-------------------------------------------------

QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
INCLUDEPATH += ../../../tb_interface
DESTDIR = $$OUT_PWD/../../../bin/modules
TARGET = sink_plots
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    dialogplots.cpp \
    listen_thread.cpp \
    main.cpp \
    spectrogramctrl.cpp \
    spectrogramfft.cpp

HEADERS += \
    dialogplots.h \
    listen_thread.h \
    spectrogramcore.h \
    spectrogramctrl.h \
    spectrogramfft.h

FORMS += \
	dialogplots.ui \
	spectrogramctrl.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    sink_plots.json \
    sink_plots.zh_CN.json

RESOURCES += \
    dialogplots.qrc
win32{
    mkoptions = $$find(QMAKESPEC, "vc")
    count(mkoptions, 1){
    INCLUDEPATH +="$$PWD/../../3rdlibs/win32/fftw"
    contains(QT_ARCH, i386) {
	LIBS+=-L"$$PWD/../../3rdlibs/win32/fftw/x86" -llibfftw3-3
    } else {
	LIBS+=-L"$$PWD/../../3rdlibs/win32/fftw/x64" -llibfftw3-3
    }
    } else: LIBS+=-lfftw3
}else: LIBS+=-lfftw3

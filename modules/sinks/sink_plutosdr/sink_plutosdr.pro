QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	iio.cpp \
	listen_thread.cpp \
	main.cpp
INCLUDEPATH += ../../../tb_interface
DESTDIR = $$OUT_PWD/../../../bin/modules
win32{
    INCLUDEPATH +="$$PWD/../../3rdlibs/win32/libiio/include"
    mkoptions = $$find(QMAKESPEC, "vc")
    count(mkoptions, 1){
     contains(QT_ARCH, i386) {
	LIBS+=-L"$$PWD/../../3rdlibs/win32/libiio/MS32" -llibiio
    } else {
	LIBS+=-L"$$PWD/../../3rdlibs/win32/libiio/MS64" -llibiio
    }
    } else {
     contains(QT_ARCH, i386) {
	LIBS+=-L"$$PWD/../../3rdlibs/win32/libiio/MinGW32" -llibiio.dll
    } else {
	LIBS+=-L"$$PWD/../../3rdlibs/win32/libiio/MinGW64" -llibiio.dll
    }

    }
}else: LIBS+=-lfftw3

DISTFILES += \
	sink_plutosdr.json \
	sink_plutosdr.zh_CN.json

HEADERS += \
	listen_thread.h

RESOURCES += \
	resources.qrc

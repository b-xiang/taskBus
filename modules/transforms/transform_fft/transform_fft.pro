TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += ../../../tb_interface
DESTDIR = $$OUT_PWD/../../../bin/modules
SOURCES += \
	main.cpp \
    function_info.cpp

DISTFILES += \
    transform_fft.json

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

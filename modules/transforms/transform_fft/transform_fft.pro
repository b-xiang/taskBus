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

LIBS+=-lfftw3

TEMPLATE = app
QT -= gui
CONFIG += c++11 console
CONFIG -= app_bundle
INCLUDEPATH += ../../../tb_interface
DESTDIR = $$OUT_PWD/../../../bin/modules
SOURCES += \
	main.cpp \
    mod_fm.cpp

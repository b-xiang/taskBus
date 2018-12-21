#-------------------------------------------------
#
# Project created by QtCreator 2018-02-01T09:30:28
#
#-------------------------------------------------

QT       += core gui


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#Judge whether qtcharts is supported.
qtHaveModule(charts){
    QT += charts
    HEADERS += watchdog/status_charts/formstatus.h
    SOURCES += watchdog/status_charts/formstatus.cpp
    FORMS += watchdog/status_charts/formstatus.ui
    message("Qt with charts!");
    INCLUDEPATH += watchdog/status_charts
} else {
    HEADERS += watchdog/status_nocharts/formstatus.h
    SOURCES += watchdog/status_nocharts/formstatus.cpp
    FORMS += watchdog/status_nocharts/formstatus.ui
    message("Qt without charts!");
    INCLUDEPATH += watchdog/status_nocharts
}


DESTDIR = $$OUT_PWD/../bin

TARGET = taskBusPlatform
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += "../tb_interface"
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    core/tasknode.cpp \
    core/taskcell.cpp \
    core/taskproject.cpp \
    gui/taskbusplatformfrm.cpp \
    gui/taskbusplatformfrm_modules.cpp \
    gui/taskbusplatformfrm_project.cpp \
    gui/main.cpp \
    gui/pdesignerview.cpp \
    gui/taskmodule.cpp \
    gui/tgraphicstaskitem.cpp \
    core/process_prctl.cpp \
    watchdog/tbwatchdog.cpp \
    watchdog/watchmemmodule.cpp

HEADERS += \
    core/tasknode.h \
    core/taskcell.h \
    core/taskproject.h \
    gui/taskbusplatformfrm.h \
    gui/pdesignerview.h \
    gui/taskmodule.h \
    gui/tgraphicstaskitem.h \
    cmdlineparser.h \
    tb_interface.h \
    tb_datastruct.h \
    core/process_prctl.h \
    ../tb_interface/cmdlineparser.h \
    ../tb_interface/tb_interface.h \
    watchdog/tbwatchdog.h \
    watchdog/watchmemmodule.h

FORMS += \
    gui/taskbusplatformfrm.ui \
    gui/pdesignerview.ui

TRANSLATIONS +=\
	taskBusPlatform_zh_CN.ts

OTHER_FILES += \
	taskBusPlatform_zh_CN.ts

win32{
    VERSION = 1.0.0.0 # major.minor.patch.build
    VERSION_PE_HEADER = 1.0
    RC_ICONS += taskbusplatform.ico
}
else
{
    VERSION = 1.0.0    # major.minor.patch
}

RESOURCES += \
    taskbusplatform.qrc

DISTFILES += \
    test.json
message($$QT_ARCH)
contains(QT_ARCH,x86):CONFIG(release, debug|release): QMAKE_CXXFLAGS +=  -march=core2  -O3 -fexpensive-optimizations


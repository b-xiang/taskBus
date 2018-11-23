TEMPLATE = app
QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle
DESTDIR = $$OUT_PWD/../bin
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += "../tb_interface"

HEADERS += \
    core/taskcell.h \
    core/tasknode.h \
    core/taskproject.h \
    tui/listen_thread.h \
    core/process_prctl.h \
    watchdog/tbwatchdog.h

SOURCES += \
    core/taskcell.cpp \
    core/tasknode.cpp \
    core/taskproject.cpp \
    tui/subtask_warpper.cpp \
    tui/listen_thread.cpp \
    core/process_prctl.cpp \
    watchdog/tbwatchdog.cpp
CONFIG(release, debug|release): QMAKE_CXXFLAGS +=  -march=core2  -O3 -fexpensive-optimizations

FORMS +=

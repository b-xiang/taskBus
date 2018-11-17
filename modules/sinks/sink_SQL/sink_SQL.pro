#-------------------------------------------------
#
# Project created by QtCreator 2018-07-27T16:02:44
#
#-------------------------------------------------

QT       += core gui sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
INCLUDEPATH += ../../../tb_interface
DESTDIR = $$OUT_PWD/../../../bin/modules
TARGET = sink_SQL
TEMPLATE = app


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += "../../../tb_interface"

SOURCES += \
	main.cpp \
	dialogsql.cpp \
    listen_thread.cpp

HEADERS += \
	dialogsql.h \
    listen_thread.h

FORMS += \
	dialogsql.ui

DISTFILES += \
    sink_SQL.exe.json

RESOURCES += \
    sinksql.qrc

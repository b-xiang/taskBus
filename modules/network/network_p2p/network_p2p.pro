#-------------------------------------------------
#
# Project created by QtCreator 2018-08-01T10:57:20
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
INCLUDEPATH += ../../../tb_interface
DESTDIR = $$OUT_PWD/../../../bin/modules
TARGET = network_p2p
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
INCLUDEPATH += "../../../tb_interface"

SOURCES += \
	main.cpp \
	dialognetp2p.cpp \
    listen_thread.cpp

HEADERS += \
	dialognetp2p.h \
    listen_thread.h

FORMS += \
	dialognetp2p.ui

RESOURCES += \
    resource.qrc

DISTFILES += \
    Resources/_40Icon Silver Reverse.png \
    Resources/+_Sign.png \
    Resources/+_Sign_Alt.png \
    Resources/0_9.png \
    Resources/3floppy_mount.png \
    Resources/3floppy_mount-1.png \
    Resources/3floppy_mount-2.png \
    Resources/3floppy_mount-3.png \
    Resources/3floppy_unmount.png \
    Resources/3floppy_unmount-1.png \
    Resources/3floppy_unmount-2.png \
    Resources/3floppy_unmount-3.png \
    Resources/3floppy_unmount-4.png \
    Resources/010-3.png \
    Resources/10-3.png \
    Resources/019-1.png \
    Resources/019-3.png \
    Resources/19-1.png \
    Resources/27_Martin.png \
    Resources/033-1.png \
    Resources/37.png \
    Resources/049-1.png \
    Resources/0062.png \
    Resources/0098.png \
    Resources/0104.png \
    Resources/0109.png \
    Resources/0126.png \
    Resources/165c, 180c.png \
    Resources/3018.png \
    Resources/4003.png \
    Resources/4011.png \
    Resources/4013.png \
    Resources/4018.png \
    Resources/4021.png \
    Resources/A-1 009.png \
    Resources/Alienware (13).png \
    Resources/Alienware (27).png \
    Resources/Alienware (28).png \
    Resources/Alienware (29).png \
    Resources/Backup drive.png \
    Resources/Backup-1.png \
    Resources/BackUp-4.png \
    Resources/Battery (1).png \
    Resources/Battery Charged.png \
    Resources/Battery -No charge.png \
    Resources/Blizzard17.png \
    Resources/bluetooth256.png \
    Resources/Burn CD-1.png \
    Resources/cn1.png \
    Resources/cn2.png \
    Resources/cn3.png \
    Resources/cn4.png \
    Resources/cn5.png \
    Resources/cn6.png \
    Resources/cn7.png \
    Resources/cn8.png \
    Resources/cn9.png \
    Resources/cn10.png \
    Resources/cn11.png \
    Resources/cn12.png \
    Resources/cn13.png \
    Resources/coffee.png \
    Resources/Color Classic Green.png \
    Resources/Color Classic, Performa 250, 275.png \
    Resources/Color Classic.png \
    Resources/Color, Blueberry.png \
    Resources/Color, Bondi.png \
    Resources/Color, Bondi-1.png \
    Resources/Color, Grape.png \
    Resources/Color, Graphite.png \
    Resources/Color, Indigo.png \
    Resources/Color, Lemon.png \
    Resources/Color, Lime.png \
    Resources/Color, Ruby.png \
    Resources/Color, Sage.png \
    Resources/Color, Strawberry.png \
    Resources/Color, Tangerine.png \
    Resources/Color, Titanium.png \
    Resources/Crystal_folder09.png \
    Resources/Crystal_folder10.png \
    Resources/Crystal_folder18.png \
    Resources/Crystal_folder19.png \
    Resources/Digital Image Bmp.png \
    Resources/DimageViewer.png \
    Resources/Folder Graphite-1.png \
    Resources/Folder Online aqua.png \
    Resources/hanukkah_03.png \
    Resources/terminalserver.png \
    network_p2p.exe.json \
    network_p2p.zh_CN.json

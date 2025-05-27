QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
MOC_DIR     = temp/moc
RCC_DIR     = temp/rcc
UI_DIR      = temp/ui
OBJECTS_DIR = temp/obj
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    videoplayer.cpp

HEADERS += \
    mainwindow.h \
    videoplayer.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += \
    $$PWD/libs/ffmpeg/include  \
    $$PWD/libs/jthread/include \
    $$PWD/libs/jrtplib/include \
    $$PWD/libs/sdl2/include

win32 {
    LIBS += \
            -L$$PWD/libs/ffmpeg/lib -lavformat \
            -L$$PWD/libs/ffmpeg/lib -lavcodec \
            -L$$PWD/libs/ffmpeg/lib -lavutil \
            -L$$PWD/libs/ffmpeg/lib -lswscale \
            -L$$PWD/libs/ffmpeg/lib -lswresample \
            -L$$PWD/libs/ffmpeg/lib -lpostproc \
            -L$$PWD/libs/ffmpeg/lib -lavfilter \
            -L$$PWD/libs/ffmpeg/lib -lavdevice \
            -lws2_32 \
            -luser32
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


INCLUDEPATH += \


SOURCES += \
    SMC100C.cpp \
    dlp9000.cpp \
    main.cpp \
    mainwindow.cpp \
    manualstagecontrol.cpp \
    serialib.cpp

HEADERS += \
    PtnImage.h \
    SMC100C.h \
    dlp9000.h \
    mainwindow.h \
    manualstagecontrol.h \
    patternelement.h \
    serialib.h


FORMS += \
    mainwindow.ui \
    manualstagecontrol.ui

#For Lightcrafter API
INCLUDEPATH += "HiresLib"
INCLUDEPATH += "hidapi-master\\hidapi"
INCLUDEPATH += "hidapi-master\\windows\\Release"

HEADERS += \
    HiresLib/usb.h\
    HiresLib/Error.h\
    HiresLib/Common.h\
    HiresLib/firmware.h\
    HiresLib/splash.h \
    HiresLib/compress.h \
    HiresLib/flashloader.h \
    HiresLib/pattern.h \
    HiresLib/batchfile.h \
    HiresLib/API.h

SOURCES += \
    HiresLib/flashloader.c \
    HiresLib/batchfile.c \
    HiresLib/compress.c \
    HiresLib/Error.c \
    HiresLib/firmware.c \
    HiresLib/flashimage.c \
    HiresLib/API.c \
    HiresLib/usb.c \
    HiresLib/pattern.c \
    HiresLib/splash.c \


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Pictures.qrc
    HiRes.qrc

INCLUDEPATH += $$PWD/HiresLib
DEPENDPATH += $$PWD/HiresLib

win32: LIBS += -lSetupAPI

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/hidapi-master/lib4QT/ -lhidapi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/hidapi-master/lib4QT/ -lhidapi
else:unix: LIBS += -L$$PWD/hidapi-master/lib4QT/ -lhidapi

INCLUDEPATH += $$PWD/hidapi-master/lib4QT
DEPENDPATH += $$PWD/hidapi-master/lib4QT

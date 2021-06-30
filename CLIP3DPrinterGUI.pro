QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += "src"
INCLUDEPATH += "src/serialib"
INCLUDEPATH += "src/qcustomplot"

SOURCES += \
    src/SMC100C.cpp \
    src/dlp9000.cpp \
    src/imagepopout.cpp \
    src/imageprocessing.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/manualpumpcontrol.cpp \
    src/manualstagecontrol.cpp \
    src/qcustomplot/qcustomplot.cpp \
    src/serialib/serialib.cpp

HEADERS += \
    src/PtnImage.h \
    src/SMC100C.h \
    src/dlp9000.h \
    src/imagepopout.h \
    src/imageprocessing.h \
    src/mainwindow.h \
    src/manualpumpcontrol.h \
    src/manualstagecontrol.h \
    src/patternelement.h \
    src/qcustomplot/qcustomplot.h \
    src/serialib/serialib.h

FORMS += \
    src/imagepopout.ui \
    src/imageprocessing.ui \
    src/mainwindow.ui \
    src/manualpumpcontrol.ui \
    src/manualstagecontrol.ui

#For Lightcrafter API"
INCLUDEPATH += "src\\HiresLib"
INCLUDEPATH += "src\\hidapi-master\\hidapi"
INCLUDEPATH += "src\\hidapi-master\\windows\\Release"
INCLUDEPATH += $$PWD/src/HiresLib
DEPENDPATH += $$PWD/src/HiresLib

HEADERS += \
    src/HiresLib/API.h \
    src/HiresLib/BMPParser.h \
    src/HiresLib/Error.h \
    src/HiresLib/batchfile.h \
    src/HiresLib/common.h \
    src/HiresLib/compress.h \
    src/HiresLib/firmware.h \
    src/HiresLib/flashimage.h \
    src/HiresLib/flashloader.h \
    src/HiresLib/pattern.h \
    src/HiresLib/splash.h \
    src/HiresLib/usb.h

SOURCES += \
    src/HiresLib/API.c \
    src/HiresLib/BMPParser.c \
    src/HiresLib/Error.c \
    src/HiresLib/batchfile.c \
    src/HiresLib/compress.c \
    src/HiresLib/firmware.c \
    src/HiresLib/flashimage.c \
    src/HiresLib/flashloader.c \
    src/HiresLib/pattern.c \
    src/HiresLib/splash.c \
    src/HiresLib/usb.c

INCLUDEPATH += $$PWD/hidapi-master/lib4QT
DEPENDPATH += $$PWD/hidapi-master/lib4QT
win32: LIBS += -L$$PWD/src/hidapi-master/lib4QT/ -lhidapi

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images/CLIP3Dresources.qrc

win32: LIBS += -lSetupAPI
#OpenCV libs
INCLUDEPATH += $$PWD/src/OpenCV/install/include
DEPENDPATH += $$PWD/src/OpenCV/install/include

win32: LIBS += -L$$PWD/src/OpenCV/install/x64/mingw/lib/ -llibopencv_core401.dll
win32: LIBS += -L$$PWD/src/OpenCV/install/x64/mingw/lib/ -llibopencv_imgcodecs401.dll
win32: LIBS += -L$$PWD/src/OpenCV/install/x64/mingw/lib/ -llibopencv_imgproc401.dll
win32: LIBS += -L$$PWD/src/OpenCV/install/x64/mingw/lib/ -llibopencv_highgui401.dll

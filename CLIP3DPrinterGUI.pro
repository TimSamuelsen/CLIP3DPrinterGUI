QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += "src"
INCLUDEPATH += "src/3rdparty/serialib"
INCLUDEPATH += "src/3rdparty/qcustomplot"
INCLUDEPATH += "src/mainwindow"
INCLUDEPATH += "src/secondary"
INCLUDEPATH += "src/hardware"


RC_ICONS = DeSimoneLogo.ico

SOURCES += \
    src/hardware/SMC100C.cpp \
    src/hardware/dlp9000.cpp \
    src/mainwindow/graphics.cpp \
    src/mainwindow/printmonitoring.cpp \
    src/mainwindow/printsettings.cpp \
    src/secondary/imagepopout.cpp \
    src/secondary/imageprocessing.cpp \
    src/main.cpp \
    src/mainwindow/mainwindow.cpp \
    src/secondary/manualpumpcontrol.cpp \
    src/secondary/manualstagecontrol.cpp \
    src/secondary/prinstscriptdialog.cpp \
    src/hardware/printcontrol.cpp \
    src/hardware/pumpcommands.cpp \
    src/3rdparty/qcustomplot/qcustomplot.cpp \
    src/3rdparty/serialib/serialib.cpp \
    src/hardware/stagecommands.cpp

HEADERS += \
    src/PrintElements.h \
    src/3rdparty/PtnImage.h \
    src/hardware/SMC100C.h \
    src/hardware/dlp9000.h \
    src/mainwindow/graphics.h \
    src/mainwindow/printmonitoring.h \
    src/mainwindow/printsettings.h \
    src/secondary/imagepopout.h \
    src/secondary/imageprocessing.h \
    src/mainwindow/mainwindow.h \
    src/secondary/manualpumpcontrol.h \
    src/secondary/manualstagecontrol.h \
    src/3rdparty/patternelement.h \
    src/secondary/prinstscriptdialog.h \
    src/hardware/printcontrol.h \
    src/hardware/pumpcommands.h \
    src/3rdparty/qcustomplot/qcustomplot.h \
    src/3rdparty/serialib/serialib.h \
    src/hardware/stagecommands.h

FORMS += \
    src/mainwindow/graphics.ui \
    src/mainwindow/printmonitoring.ui \
    src/mainwindow/printsettings.ui \
    src/secondary/imagepopout.ui \
    src/secondary/imageprocessing.ui \
    src/mainwindow/mainwindow.ui \
    src/secondary/manualpumpcontrol.ui \
    src/secondary/manualstagecontrol.ui \
    src/secondary/prinstscriptdialog.ui

#For Lightcrafter API"
INCLUDEPATH += "src\\3rdparty\\HiresLib"
INCLUDEPATH += "src\\3rdparty\\hidapi-master\\hidapi"
INCLUDEPATH += "src\\3rdparty\\hidapi-master\\windows\\Release"
INCLUDEPATH += $$PWD/src/3rdparty/HiresLib
DEPENDPATH += $$PWD/src/3rdpartyHiresLib

HEADERS += \
    src/3rdparty/HiresLib/API.h \
    src/3rdparty/HiresLib/BMPParser.h \
    src/3rdparty/HiresLib/Error.h \
    src/3rdparty/HiresLib/batchfile.h \
    src/3rdparty/HiresLib/common.h \
    src/3rdparty/HiresLib/compress.h \
    src/3rdparty/HiresLib/firmware.h \
    src/3rdparty/HiresLib/flashimage.h \
    src/3rdparty/HiresLib/flashloader.h \
    src/3rdparty/HiresLib/pattern.h \
    src/3rdparty/HiresLib/splash.h \
    src/3rdparty/HiresLib/usb.h

SOURCES += \
    src/3rdparty/HiresLib/API.c \
    src/3rdparty/HiresLib/BMPParser.c \
    src/3rdparty/HiresLib/Error.c \
    src/3rdparty/HiresLib/batchfile.c \
    src/3rdparty/HiresLib/compress.c \
    src/3rdparty/HiresLib/firmware.c \
    src/3rdparty/HiresLib/flashimage.c \
    src/3rdparty/HiresLib/flashloader.c \
    src/3rdparty/HiresLib/pattern.c \
    src/3rdparty/HiresLib/splash.c \
    src/3rdparty/HiresLib/usb.c

INCLUDEPATH += $$PWD/3rdparty/hidapi-master/lib4QT
DEPENDPATH += $$PWD/3rdparty/hidapi-master/lib4QT
win32: LIBS += -L$$PWD/src/3rdparty/hidapi-master/lib4QT/ -lhidapi

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images/CLIP3Dresources.qrc \
    images/breeze.qrc

win32: LIBS += -lSetupAPI
#OpenCV libs
INCLUDEPATH += $$PWD/src/3rdparty/OpenCV/install/include
DEPENDPATH += $$PWD/src/3rdparty/OpenCV/install/include

win32: LIBS += -L$$PWD/src/3rdparty/OpenCV/install/x64/mingw/lib/ -llibopencv_core401.dll
win32: LIBS += -L$$PWD/src/3rdparty/OpenCV/install/x64/mingw/lib/ -llibopencv_imgcodecs401.dll
win32: LIBS += -L$$PWD/src/3rdparty/OpenCV/install/x64/mingw/lib/ -llibopencv_imgproc401.dll
win32: LIBS += -L$$PWD/src/3rdparty/OpenCV/install/x64/mingw/lib/ -llibopencv_highgui401.dll

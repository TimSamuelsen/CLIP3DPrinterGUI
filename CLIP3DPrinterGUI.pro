QT       += core gui printsupport opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


INCLUDEPATH += "src"


SOURCES += \
    src/SMC100C.cpp \
    src/dlp9000.cpp \
    src/imagepopout.cpp \
    src/imageprocessing.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/manualpumpcontrol.cpp \
    src/manualstagecontrol.cpp \
    src/qcustomplot.cpp \
    src/serialib.cpp

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
    src/qcustomplot.h \
    src/serialib.h


FORMS += \
    src/imagepopout.ui \
    src/imageprocessing.ui \
    src/mainwindow.ui \
    src/manualpumpcontrol.ui \
    src/manualstagecontrol.ui

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

#For Slic3r
INCLUDEPATH += "slic3r"
INCLUDEPATH += "slic3r\\libslic3r"

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

win32: INCLUDEPATH += C:/DEV/boost_1_75_0

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/hidapi-master/lib4QT/ -lhidapi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/hidapi-master/lib4QT/ -lhidapi
else:unix: LIBS += -L$$PWD/hidapi-master/lib4QT/ -lhidapi

INCLUDEPATH += $$PWD/hidapi-master/lib4QT
DEPENDPATH += $$PWD/hidapi-master/lib4QT

#OpenCV libs
INCLUDEPATH += $$PWD/OpenCV/install/include
DEPENDPATH += $$PWD/OpenCV/install/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_core401.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_core401.dll
else:unix: LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_core401.dll

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_imgcodecs401.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_imgcodecs401.dll
else:unix: LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_imgcodecs401.dll

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_imgproc401.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_imgproc401.dll
else:unix: LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_imgproc401.dll

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_highgui401.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_highgui401.dll
else:unix: LIBS += -L$$PWD/OpenCV/install/x64/mingw/lib/ -llibopencv_highgui401.dll

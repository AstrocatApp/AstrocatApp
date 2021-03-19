QT       += core gui sql testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    autostretcher.cpp \
    filerepository.cpp \
    fileviewmodel.cpp \
    filterwidget.cpp \
    fitsfile.cpp \
    fitsprocessor.cpp \
    foldercrawler.cpp \
    imageprocessor.cpp \
    main.cpp \
    mainwindow.cpp \
    newfileprocessor.cpp \
    searchfolderdialog.cpp \
    sortfilterproxymodel.cpp \
    xisfprocessor.cpp

HEADERS += \
    astrofile.h \
    autostretcher.h \
    fileprocessor.h \
    filerepository.h \
    fileviewmodel.h \
    filterwidget.h \
    fitsfile.h \
    fitsprocessor.h \
    foldercrawler.h \
    imageprocessor.h \
    mainwindow.h \
    newfileprocessor.h \
    searchfolderdialog.h \
    sortfilterproxymodel.h \
    xisfprocessor.h

FORMS += \
    mainwindow.ui \
    searchfolderdialog.ui

TRANSLATIONS += \
    AstrocatApp_en_US.ts

RESOURCES += \
    Resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# cfitsio
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/external/cfitsio/release/ -lcfitsio.9.3.49
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/external/cfitsio/debug/ -lcfitsio.9.3.49
else:unix: LIBS += -L$$PWD/external/cfitsio/ -lcfitsio.9.3.49

INCLUDEPATH += $$PWD/external/cfitsio
DEPENDPATH += $$PWD/external/cfitsio


# PCL-pxi
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/release/ -lPCL-pxi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/debug/ -lPCL-pxi
else:unix: LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/ -lPCL-pxi

INCLUDEPATH += $$PWD/../../github/PCL/include
DEPENDPATH += $$PWD/../../github/PCL/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/libPCL-pxi.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/libPCL-pxi.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/PCL-pxi.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/PCL-pxi.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/libPCL-pxi.a


# lz4-pxi
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/release/ -llz4-pxi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/debug/ -llz4-pxi
else:unix: LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/ -llz4-pxi

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/liblz4-pxi.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/liblz4-pxi.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/lz4-pxi.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/lz4-pxi.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/liblz4-pxi.a


# zlib-pxi
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/release/ -lzlib-pxi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/debug/ -lzlib-pxi
else:unix: LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/ -lzlib-pxi

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/libzlib-pxi.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/libzlib-pxi.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/zlib-pxi.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/zlib-pxi.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/libzlib-pxi.a


# lcms-pxi
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/release/ -llcms-pxi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/debug/ -llcms-pxi
else:unix: LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/ -llcms-pxi

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/liblcms-pxi.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/liblcms-pxi.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/lcms-pxi.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/lcms-pxi.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/liblcms-pxi.a


# RFC6234-pxi
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/release/ -lRFC6234-pxi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/debug/ -lRFC6234-pxi
else:unix: LIBS += -L$$PWD/../../github/PCL/lib/macos/x64/ -lRFC6234-pxi

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/libRFC6234-pxi.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/libRFC6234-pxi.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/release/RFC6234-pxi.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/debug/RFC6234-pxi.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../github/PCL/lib/macos/x64/libRFC6234-pxi.a

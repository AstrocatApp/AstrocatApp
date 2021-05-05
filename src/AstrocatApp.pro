QT       += core gui sql concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

VERSION = 0.0.1
DEFINES += CURRENT_APP_VERSION=\"\\\"$${VERSION}\\\"\"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutwindow.cpp \
    autostretcher.cpp \
    catalog.cpp \
    fileprocessfilter.cpp \
    filerepository.cpp \
    fileviewmodel.cpp \
    filterview.cpp \
    fitsfile.cpp \
    fitsprocessor.cpp \
    foldercrawler.cpp \
    imageprocessor.cpp \
    main.cpp \
    mainwindow.cpp \
    mock_newfileprocessor.cpp \
    newfileprocessor.cpp \
    searchfolderdialog.cpp \
    sortfilterproxymodel.cpp \
    thumbnailcache.cpp \
    xisfprocessor.cpp

HEADERS += \
    aboutwindow.h \
    astrofile.h \
    autostretcher.h \
    catalog.h \
    fileprocessfilter.h \
    fileprocessor.h \
    filerepository.h \
    fileviewmodel.h \
    filterview.h \
    fitsfile.h \
    fitsprocessor.h \
    foldercrawler.h \
    imageprocessor.h \
    mainwindow.h \
    mock_newfileprocessor.h \
    newfileprocessor.h \
    searchfolderdialog.h \
    sortfilterproxymodel.h \
    thumbnailcache.h \
    xisfprocessor.h

FORMS += \
    aboutwindow.ui \
    mainwindow.ui \
    searchfolderdialog.ui

TRANSLATIONS += \
    AstrocatApp_en_US.ts

RESOURCES += \
    Resources.qrc

LIBS += -L$$PWD/../external/build/libs/ -lpcl -llcms -llz4 -lRFC6234 -lcfitsio -lzlib

win32 {
    LIBS += -L$$PWD/../external/build/libs
    LIBS += -luser32 -luserenv -ladvapi32 -lpthreadVC2
    DEFINES += __PCL_WINDOWS WIN32 WIN64 __PCL_NO_WIN32_MINIMUM_VERSIONS UNICODE _UNICODE _WINDOWS _NDEBUG
    QMAKE_CXXFLAGS = "/EHsc /MP"
    RC_ICONS = resources/Icons/win.ico/app.ico
}
macx {
    DEFINES += __PCL_MACOSX
    ICON = resources/Icons/mac.icns
}
linux {
DEFINES += __PCL_LINUX
}
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../external/cfitsio
INCLUDEPATH += $$PWD/../external/pcl/include

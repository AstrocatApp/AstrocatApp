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

LIBS += -L$$PWD/../external/build/libs/ -llcms -llz4 -lpcl -lRFC6234 -lcfitsio -lzlib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../external/cfitsio
INCLUDEPATH += $$PWD/../external/pcl/include

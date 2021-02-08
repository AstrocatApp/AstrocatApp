QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    foldercrawler.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    astrofile.h \
    foldercrawler.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    AstrocatApp_en_US.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/external/cfitsio/release/ -lcfitsio.9.3.49
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/external/cfitsio/debug/ -lcfitsio.9.3.49
else:unix: LIBS += -L$$PWD/external/cfitsio/ -lcfitsio.9.3.49

INCLUDEPATH += $$PWD/external/cfitsio
DEPENDPATH += $$PWD/external/cfitsio

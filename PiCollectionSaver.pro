#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

QT       += sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PiCollectionSaver
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

HEADERS += src/CommonConstants.h\
    src/SqLiteManager.h\
    src/SerialPicsDownloader.h\
    src/HttpDownloader.h\
    src/AddNewUser.h \
    src/albummanager.h \
    src/DownloadPicById.h \
    src/CommonUtils.h \
    src/site.h \
    src/errhlpr.h \
    src/picollectionsaver.h \
    src/topleveiInterfaces.h \
    src/plugin_interface.h

SOURCES += src/CommonConstants.cpp\
    src/HttpDownloader.cpp\
    src/main.cpp\
    src/SerialPicsDownloader.cpp\
    src/SqLiteManager.cpp\
    src/AddNewUser.cpp \
    src/albummanager.cpp \
    src/DownloadPicById.cpp \
    src/CommonUtils.cpp \
    src/site.cpp \
    src/errhlpr.cpp \
    src/picollectionsaver.cpp

FORMS +=\
    ui/AddNewUserDlg.ui \
    ui/DownloadPicById.ui \
    ui/picollectionsaver.ui

RESOURCES += \
    ui/picollectionsaver.qrc

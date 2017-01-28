TEMPLATE = app
TARGET = cpi
CONFIG += console debug
CONFIG -= app_bundle
QT     -= gui
DEPENDPATH += .
INCLUDEPATH += .

isEmpty( target.path ) {
  win32 {
    target.path = C:/Windows
  } else {
    target.path = /usr/bin
  }
}
INSTALLS += target

# Input
SOURCES += main.cpp

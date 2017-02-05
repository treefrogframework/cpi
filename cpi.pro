TEMPLATE = app
TARGET = cpi
CONFIG += console c++11 debug
CONFIG -= app_bundle
QT     -= gui
DEPENDPATH += .
INCLUDEPATH += .

isEmpty( target.path ) {
  win32 {
    target.path = C:/Windows
  } else {
    target.path = /usr/local/bin
  }
}
INSTALLS += target

# Input
SOURCES += main.cpp
HEADERS += compiler.h
SOURCES += compiler.cpp
HEADERS += codegenerator.h
SOURCES += codegenerator.cpp
HEADERS += print.h
SOURCES += print.cpp
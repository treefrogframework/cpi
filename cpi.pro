TEMPLATE = app
TARGET = cpi
CONFIG += console c++17
CONFIG -= app_bundle
QT     -= gui
DEPENDPATH += .
INCLUDEPATH += .


windows {
  DESTDIR = $${OUT_PWD}
  EXEFILE = $${OUT_PWD}/cpi.exe
  QMAKE_POST_LINK = windeployqt.exe \"$$EXEFILE\"
} else {
  isEmpty( target.path ) {
    target.path = /usr/local/bin
  }
  INSTALLS += target
}

# Input
SOURCES += main.cpp
HEADERS += compiler.h
SOURCES += compiler.cpp
HEADERS += codegenerator.h
SOURCES += codegenerator.cpp
HEADERS += print.h
SOURCES += print.cpp
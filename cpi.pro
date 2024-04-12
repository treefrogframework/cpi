TEMPLATE = app
TARGET = cpi
CONFIG += console c++17
CONFIG -= app_bundle
QT     -= gui
DEPENDPATH += .
INCLUDEPATH += .


windows {
  CONFIG(debug, debug|release) {
    EXEFILE = $${OUT_PWD}/debug/cpi.exe
  } else {
    EXEFILE = $${OUT_PWD}/release/cpi.exe
  }
  QMAKE_POST_LINK = windeployqt.exe \"$$EXEFILE\"
}

!windows {
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
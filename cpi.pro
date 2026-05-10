TEMPLATE = app
TARGET = cpi
CONFIG += console c++20
CONFIG -= app_bundle
QT     -= gui
DEPENDPATH += .
INCLUDEPATH += .

windows {
  DESTDIR = $${OUT_PWD}
  LIBS += -luser32
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

windows {
  HEADERS += global.h
  SOURCES += global_win.cpp
  HEADERS += ptyprocess_win.h
  SOURCES += ptyprocess_win.cpp
} else {
  HEADERS += global.h
  SOURCES += global.cpp
  HEADERS += ptyprocess.h
  SOURCES += ptyprocess.cpp
}

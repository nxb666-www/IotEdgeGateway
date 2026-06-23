QT       += core gui widgets network serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS
SOURCES += main.cpp mainwidget.cpp
HEADERS += mainwidget.h
FORMS   += mainwidget.ui

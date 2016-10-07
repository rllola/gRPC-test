QT += core
QT -= gui

CONFIG += c++11

TARGET = hello
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    main.grpc.pb.cc \
    main.pb.cc

DISTFILES +=

LIBS += -lgrpc++ -lgrpc -lgpr -lgrpc++_reflection -lprotobuf

HEADERS += \
    main.grpc.pb.h \
    main.pb.h

CONFIG += testcase
TARGET = tst_qpacketprotocol
macx:CONFIG -= app_bundle

SOURCES += tst_qpacketprotocol.cpp

INCLUDEPATH += ../shared
include(../shared/debugutil.pri)

CONFIG += parallel_test
QT += qml-private network testlib
/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QTimer>
#include <QQmlContext>
#include <qqmlinfo.h>
#include "../../shared/util.h"

class tst_qqmlinfo : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlinfo() {}

private slots:
    void qmlObject();
    void nestedQmlObject();
    void nestedComponent();
    void nonQmlObject();
    void nullObject();
    void nonQmlContextedObject();
    void types();
    void chaining();

private:
    QQmlEngine engine;
};

void tst_qqmlinfo::qmlObject()
{
    QQmlComponent component(&engine, testFileUrl("qmlObject.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QString message = component.url().toString() + ":3:1: QML QtObject: Test Message";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    qmlInfo(object) << "Test Message";

    QObject *nested = qvariant_cast<QObject *>(object->property("nested"));
    QVERIFY(nested != 0);

    message = component.url().toString() + ":6:13: QML QtObject: Second Test Message";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    qmlInfo(nested) << "Second Test Message";
}

void tst_qqmlinfo::nestedQmlObject()
{
    QQmlComponent component(&engine, testFileUrl("nestedQmlObject.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QObject *nested = qvariant_cast<QObject *>(object->property("nested"));
    QVERIFY(nested != 0);
    QObject *nested2 = qvariant_cast<QObject *>(object->property("nested2"));
    QVERIFY(nested2 != 0);

    QString message = component.url().toString() + ":5:13: QML NestedObject: Outer Object";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    qmlInfo(nested) << "Outer Object";

    message = testFileUrl("NestedObject.qml").toString() + ":6:14: QML QtObject: Inner Object";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    qmlInfo(nested2) << "Inner Object";
}

void tst_qqmlinfo::nestedComponent()
{
    QQmlComponent component(&engine, testFileUrl("NestedComponent.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QObject *nested = qvariant_cast<QObject *>(object->property("nested"));
    QVERIFY(nested != 0);
    QObject *nested2 = qvariant_cast<QObject *>(object->property("nested2"));
    QVERIFY(nested2 != 0);

    QString message = component.url().toString() + ":10:9: QML NestedObject: Complex Object";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    qmlInfo(nested) << "Complex Object";

    message = component.url().toString() + ":16:9: QML Image: Simple Object";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    qmlInfo(nested2) << "Simple Object";
}

void tst_qqmlinfo::nonQmlObject()
{
    QObject object;
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML QtObject: Test Message");
    qmlInfo(&object) << "Test Message";

    QTimer nonQmlObject;
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML QTimer: Test Message");
    qmlInfo(&nonQmlObject) << "Test Message";
}

void tst_qqmlinfo::nullObject()
{
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Null Object Test Message");
    qmlInfo(0) << "Null Object Test Message";
}

void tst_qqmlinfo::nonQmlContextedObject()
{
    QObject object;
    QQmlContext context(&engine);
    QQmlEngine::setContextForObject(&object, &context);
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML QtObject: Test Message");
    qmlInfo(&object) << "Test Message";
}

void tst_qqmlinfo::types()
{
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: false");
    qmlInfo(0) << false;

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: 1.1");
    qmlInfo(0) << 1.1;

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: 1.2");
    qmlInfo(0) << 1.2f;

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: 15");
    qmlInfo(0) << 15;

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: 'b'");
    qmlInfo(0) << QChar('b');

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: \"Qt\"");
    qmlInfo(0) << QByteArray("Qt");

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: true");
    qmlInfo(0) << bool(true);

    //### do we actually want QUrl to show up in the output?
    //### why the extra space at the end?
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QUrl(\"http://qt.nokia.com\") ");
    qmlInfo(0) << QUrl("http://qt.nokia.com");

    //### should this be quoted?
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: hello");
    qmlInfo(0) << QLatin1String("hello");

    //### should this be quoted?
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: World");
    QString str("Hello World");
    QStringRef ref(&str, 6, 5);
    qmlInfo(0) << ref;

    //### should this be quoted?
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Quick");
    qmlInfo(0) << QString ("Quick");
}

void tst_qqmlinfo::chaining()
{
    //### should more of these be automatically inserting spaces?
    QString str("Hello World");
    QStringRef ref(&str, 6, 5);
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: false 1.1 1.2 15 hello 'b' QUrl(\"http://qt.nokia.com\") World \"Qt\" true Quick ");
    qmlInfo(0) << false << ' '
               << 1.1 << ' '
               << 1.2f << ' '
               << 15 << ' '
               << QLatin1String("hello") << ' '
               << QChar('b') << ' '
               << QUrl("http://qt.nokia.com")
               << ref
               << QByteArray("Qt")
               << bool(true)
               << QString ("Quick");
}

QTEST_MAIN(tst_qqmlinfo)

#include "tst_qqmlinfo.moc"
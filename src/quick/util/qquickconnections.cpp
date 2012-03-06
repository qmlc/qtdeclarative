/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickconnections_p.h"

#include <private/qqmlexpression_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlboundsignal_p.h>
#include <qqmlcontext.h>
#include <private/qqmlcontext_p.h>
#include <qqmlinfo.h>

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickConnectionsPrivate : public QObjectPrivate
{
public:
    QQuickConnectionsPrivate() : target(0), targetSet(false), ignoreUnknownSignals(false), componentcomplete(true) {}

    QList<QQmlBoundSignal*> boundsignals;
    QObject *target;

    bool targetSet;
    bool ignoreUnknownSignals;
    bool componentcomplete;

    QByteArray data;
};

/*!
    \qmlclass Connections QQuickConnections
    \inqmlmodule QtQuick 2
    \ingroup qml-utility-elements
    \brief A Connections element describes generalized connections to signals.

    A Connections object creates a connection to a QML signal.

    When connecting to signals in QML, the usual way is to create an
    "on<Signal>" handler that reacts when a signal is received, like this:

    \qml
    MouseArea {
        onClicked: { foo(parameters) }
    }
    \endqml

    However, it is not possible to connect to a signal in this way in some 
    cases, such as when:

    \list
        \i Multiple connections to the same signal are required
        \i Creating connections outside the scope of the signal sender
        \i Connecting to targets not defined in QML
    \endlist

    When any of these are needed, the Connections element can be used instead.

    For example, the above code can be changed to use a Connections object,
    like this:

    \qml
    MouseArea {
        Connections {
            onClicked: foo(parameters)
        }
    }
    \endqml

    More generally, the Connections object can be a child of some object other than
    the sender of the signal:

    \qml
    MouseArea {
        id: area
    }
    // ...
    \endqml
    \qml
    Connections {
        target: area
        onClicked: foo(parameters)
    }
    \endqml

    \sa QtQml
*/
QQuickConnections::QQuickConnections(QObject *parent) :
    QObject(*(new QQuickConnectionsPrivate), parent)
{
}

QQuickConnections::~QQuickConnections()
{
}

/*!
    \qmlproperty Object QtQuick2::Connections::target
    This property holds the object that sends the signal.

    If this property is not set, the \c target defaults to the parent of the Connection.

    If set to null, no connection is made and any signal handlers are ignored
    until the target is not null.
*/
QObject *QQuickConnections::target() const
{
    Q_D(const QQuickConnections);
    return d->targetSet ? d->target : parent();
}

void QQuickConnections::setTarget(QObject *obj)
{
    Q_D(QQuickConnections);
    d->targetSet = true; // even if setting to 0, it is *set*
    if (d->target == obj)
        return;
    foreach (QQmlBoundSignal *s, d->boundsignals) {
        // It is possible that target is being changed due to one of our signal
        // handlers -> use deleteLater().
        if (s->isEvaluating())
            s->deleteLater();
        else
            delete s;
    }
    d->boundsignals.clear();
    d->target = obj;
    connectSignals();
    emit targetChanged();
}

/*!
    \qmlproperty bool QtQuick2::Connections::ignoreUnknownSignals

    Normally, a connection to a non-existent signal produces runtime errors.

    If this property is set to \c true, such errors are ignored.
    This is useful if you intend to connect to different types of objects, handling
    a different set of signals for each object.
*/
bool QQuickConnections::ignoreUnknownSignals() const
{
    Q_D(const QQuickConnections);
    return d->ignoreUnknownSignals;
}

void QQuickConnections::setIgnoreUnknownSignals(bool ignore)
{
    Q_D(QQuickConnections);
    d->ignoreUnknownSignals = ignore;
}



QByteArray
QQmlConnectionsParser::compile(const QList<QQmlCustomParserProperty> &props)
{
    QByteArray rv;
    QDataStream ds(&rv, QIODevice::WriteOnly);

    for(int ii = 0; ii < props.count(); ++ii)
    {
        QString propName = props.at(ii).name();
        int propLine = props.at(ii).location().line;
        int propColumn = props.at(ii).location().column;

        if (!propName.startsWith(QLatin1String("on")) || !propName.at(2).isUpper()) {
            error(props.at(ii), QQuickConnections::tr("Cannot assign to non-existent property \"%1\"").arg(propName));
            return QByteArray();
        }

        QList<QVariant> values = props.at(ii).assignedValues();

        for (int i = 0; i < values.count(); ++i) {
            const QVariant &value = values.at(i);

            if (value.userType() == qMetaTypeId<QQmlCustomParserNode>()) {
                error(props.at(ii), QQuickConnections::tr("Connections: nested objects not allowed"));
                return QByteArray();
            } else if (value.userType() == qMetaTypeId<QQmlCustomParserProperty>()) {
                error(props.at(ii), QQuickConnections::tr("Connections: syntax error"));
                return QByteArray();
            } else {
                QQmlScript::Variant v = qvariant_cast<QQmlScript::Variant>(value);
                if (v.isScript()) {
                    ds << propName;
                    ds << rewriteSignalHandler(v, propName);
                    ds << propLine;
                    ds << propColumn;
                } else {
                    error(props.at(ii), QQuickConnections::tr("Connections: script expected"));
                    return QByteArray();
                }
            }
        }
    }

    return rv;
}

void QQmlConnectionsParser::setCustomData(QObject *object,
                                            const QByteArray &data)
{
    QQuickConnectionsPrivate *p =
        static_cast<QQuickConnectionsPrivate *>(QObjectPrivate::get(object));
    p->data = data;
}


void QQuickConnections::connectSignals()
{
    Q_D(QQuickConnections);
    if (!d->componentcomplete || (d->targetSet && !target()))
        return;

    QDataStream ds(d->data);
    while (!ds.atEnd()) {
        QString propName;
        ds >> propName;
        QString script;
        ds >> script;
        int line;
        ds >> line;
        int column;
        ds >> column;

        QQmlProperty prop(target(), propName);
        if (prop.isValid() && (prop.type() & QQmlProperty::SignalProperty)) {
            QQmlBoundSignal *signal =
                new QQmlBoundSignal(target(), prop.method(), this);

            QString location;
            QQmlContextData *ctxtdata = 0;
            QQmlData *ddata = QQmlData::get(this);
            if (ddata) {
                ctxtdata = ddata->outerContext;
                if (ctxtdata && !ctxtdata->url.isEmpty())
                    location = ddata->outerContext->urlString;
            }

            QQmlExpression *expression = ctxtdata ?
                QQmlExpressionPrivate::create(ctxtdata, 0, script, true, location, line, column) : 0;
            signal->setExpression(expression);
            d->boundsignals += signal;
        } else {
            if (!d->ignoreUnknownSignals)
                qmlInfo(this) << tr("Cannot assign to non-existent property \"%1\"").arg(propName);
        }
    }
}

void QQuickConnections::classBegin()
{
    Q_D(QQuickConnections);
    d->componentcomplete=false;
}

void QQuickConnections::componentComplete()
{
    Q_D(QQuickConnections);
    d->componentcomplete=true;
    connectSignals();
}

QT_END_NAMESPACE
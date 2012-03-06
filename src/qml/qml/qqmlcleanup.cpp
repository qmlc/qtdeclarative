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

#include "qqmlcleanup_p.h"

#include "qqmlengine_p.h"

QT_BEGIN_NAMESPACE

/*!
\internal
\class QQmlCleanup
\brief The QQmlCleanup provides a callback when a QQmlEngine is deleted. 

Any object that needs cleanup to occur before the QQmlEngine's V8 engine is
destroyed should inherit from QQmlCleanup.  The clear() virtual method will be
called by QQmlEngine just before it destroys the context.
*/


/*
Create a QQmlCleanup that is not associated with any engine.
*/
QQmlCleanup::QQmlCleanup()
: prev(0), next(0), engine(0)
{
}

/*!
Create a QQmlCleanup for \a engine
*/
QQmlCleanup::QQmlCleanup(QQmlEngine *engine)
: prev(0), next(0), engine(0)
{
    if (!engine)
        return;

    addToEngine(engine);
}

/*!
Adds this object to \a engine's cleanup list.  hasEngine() must be false
before calling this method.
*/
void QQmlCleanup::addToEngine(QQmlEngine *engine)
{
    Q_ASSERT(engine);
    Q_ASSERT(QQmlEnginePrivate::isEngineThread(engine));

    this->engine = engine;

    QQmlEnginePrivate *p = QQmlEnginePrivate::get(engine);

    if (p->cleanup) next = p->cleanup;
    p->cleanup = this;
    prev = &p->cleanup;
    if (next) next->prev = &next;
}

/*!
\fn bool QQmlCleanup::hasEngine() const

Returns true if this QQmlCleanup is associated with an engine, otherwise false.
*/

/*!
\internal
*/
QQmlCleanup::~QQmlCleanup()
{
    Q_ASSERT(!prev || engine);
    Q_ASSERT(!prev || QQmlEnginePrivate::isEngineThread(engine));

    if (prev) *prev = next;
    if (next) next->prev = prev;
    prev = 0; 
    next = 0;
}

QT_END_NAMESPACE
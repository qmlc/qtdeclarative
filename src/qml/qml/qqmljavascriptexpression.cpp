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

#include "qqmljavascriptexpression_p.h"

#include <private/qqmlexpression_p.h>

QT_BEGIN_NAMESPACE

bool QQmlDelayedError::addError(QQmlEnginePrivate *e)
{
    if (!e) return false;

    if (e->inProgressCreations == 0) return false; // Not in construction

    if (prevError) return true; // Already in error chain

    prevError = &e->erroredBindings;
    nextError = e->erroredBindings;
    e->erroredBindings = this;
    if (nextError) nextError->prevError = &nextError;

    return true;
}

QQmlJavaScriptExpression::QQmlJavaScriptExpression(VTable *v)
: m_vtable(v)
{
}

QQmlJavaScriptExpression::~QQmlJavaScriptExpression()
{
    clearGuards();
}

void QQmlJavaScriptExpression::setNotifyOnValueChanged(bool v)
{
    activeGuards.setFlagValue(v);
    if (!v) clearGuards();
}

void QQmlJavaScriptExpression::resetNotifyOnValueChanged()
{
    clearGuards();
}

v8::Local<v8::Value>
QQmlJavaScriptExpression::evaluate(QQmlContextData *context,
                                   v8::Handle<v8::Function> function, bool *isUndefined)
{
    Q_ASSERT(context && context->engine);

    if (function.IsEmpty() || function->IsUndefined()) {
        if (isUndefined) *isUndefined = true;
        return v8::Local<v8::Value>();
    }

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);

    Q_ASSERT(notifyOnValueChanged() || activeGuards.isEmpty());
    GuardCapture capture(context->engine, this);

    QQmlEnginePrivate::PropertyCapture *lastPropertyCapture = ep->propertyCapture;
    ep->propertyCapture = notifyOnValueChanged()?&capture:0;


    if (notifyOnValueChanged())
        capture.guards.copyAndClearPrepend(activeGuards);

    QQmlContextData *lastSharedContext = 0;
    QObject *lastSharedScope = 0;

    bool sharedContext = useSharedContext();

    // All code that follows must check with watcher before it accesses data members
    // incase we have been deleted.
    DeleteWatcher watcher(this);

    if (sharedContext) {
        lastSharedContext = ep->sharedContext;
        lastSharedScope = ep->sharedScope;
        ep->sharedContext = context;
        ep->sharedScope = scopeObject();
    }

    v8::Local<v8::Value> result;
    {
        v8::TryCatch try_catch;
        v8::Handle<v8::Object> This = ep->v8engine()->global();
        if (scopeObject() && requiresThisObject()) {
            v8::Handle<v8::Value> value = ep->v8engine()->newQObject(scopeObject());
            if (value->IsObject()) This = v8::Handle<v8::Object>::Cast(value);
        }

        result = function->Call(This, 0, 0);

        if (isUndefined)
            *isUndefined = try_catch.HasCaught() || result->IsUndefined();

        if (watcher.wasDeleted()) {
        } else if (try_catch.HasCaught()) {
            v8::Context::Scope scope(ep->v8engine()->context());
            v8::Local<v8::Message> message = try_catch.Message();
            if (!message.IsEmpty()) {
                QQmlExpressionPrivate::exceptionToError(message, delayedError()->error);
            } else {
                if (hasDelayedError()) delayedError()->error = QQmlError();
            }
        } else {
            if (hasDelayedError()) delayedError()->error = QQmlError();
        }
    }

    if (sharedContext) {
        ep->sharedContext = lastSharedContext;
        ep->sharedScope = lastSharedScope;
    }

    if (capture.errorString) {
        for (int ii = 0; ii < capture.errorString->count(); ++ii)
            qWarning("%s", qPrintable(capture.errorString->at(ii)));
        delete capture.errorString;
        capture.errorString = 0;
    }

    while (Guard *g = capture.guards.takeFirst())
        g->Delete();

    ep->propertyCapture = lastPropertyCapture;

    return result;
}

void QQmlJavaScriptExpression::GuardCapture::captureProperty(QQmlNotifier *n)
{
    if (expression) {

        // Try and find a matching guard
        while (!guards.isEmpty() && !guards.first()->isConnected(n))
            guards.takeFirst()->Delete();

        Guard *g = 0;
        if (!guards.isEmpty()) {
            g = guards.takeFirst();
            g->cancelNotify();
            Q_ASSERT(g->isConnected(n));
        } else {
            g = Guard::New(expression, engine);
            g->connect(n);
        }

        expression->activeGuards.prepend(g);
    }
}

void QQmlJavaScriptExpression::GuardCapture::captureProperty(QObject *o, int c, int n)
{
    if (expression) {
        if (n == -1) {
            if (!errorString) {
                errorString = new QStringList;
                QString preamble = QLatin1String("QQmlExpression: Expression ") +
                                   expression->m_vtable->expressionIdentifier(expression) +
                                   QLatin1String(" depends on non-NOTIFYable properties:");
                errorString->append(preamble);
            }

            const QMetaObject *metaObj = o->metaObject();
            QMetaProperty metaProp = metaObj->property(c);

            QString error = QLatin1String("    ") +
                            QString::fromUtf8(metaObj->className()) +
                            QLatin1String("::") +
                            QString::fromUtf8(metaProp.name());
            errorString->append(error);
        } else {

            // Try and find a matching guard
            while (!guards.isEmpty() && !guards.first()->isConnected(o, n))
                guards.takeFirst()->Delete();

            Guard *g = 0;
            if (!guards.isEmpty()) {
                g = guards.takeFirst();
                g->cancelNotify();
                Q_ASSERT(g->isConnected(o, n));
            } else {
                g = Guard::New(expression, engine);
                g->connect(o, n);
            }

            expression->activeGuards.prepend(g);
        }
    }
}

void QQmlJavaScriptExpression::clearError()
{
    if (m_vtable.hasValue()) {
        m_vtable.value().error = QQmlError();
        m_vtable.value().removeError();
    }
}

QQmlError QQmlJavaScriptExpression::error() const
{
    if (m_vtable.hasValue()) return m_vtable.constValue()->error;
    else return QQmlError();
}

QQmlDelayedError *QQmlJavaScriptExpression::delayedError()
{
    return &m_vtable.value();
}

void QQmlJavaScriptExpression::exceptionToError(v8::Handle<v8::Message> message, QQmlError &error)
{
    Q_ASSERT(!message.IsEmpty());

    v8::Handle<v8::Value> name = message->GetScriptResourceName();
    v8::Handle<v8::String> description = message->Get();
    int lineNumber = message->GetLineNumber();

    v8::Local<v8::String> file = name->IsString()?name->ToString():v8::Local<v8::String>();
    if (file.IsEmpty() || file->Length() == 0)
        error.setUrl(QUrl(QLatin1String("<Unknown File>")));
    else
        error.setUrl(QUrl(QV8Engine::toStringStatic(file)));

    error.setLine(lineNumber);
    error.setColumn(-1);

    QString qDescription = QV8Engine::toStringStatic(description);
    if (qDescription.startsWith(QLatin1String("Uncaught ")))
        qDescription = qDescription.mid(9 /* strlen("Uncaught ") */);

    error.setDescription(qDescription);
}

// Callee owns the persistent handle
v8::Persistent<v8::Function>
QQmlJavaScriptExpression::evalFunction(QQmlContextData *ctxt, QObject *scope,
                                       const char *code, int codeLength,
                                       const QString &filename, int line,
                                       v8::Persistent<v8::Object> *qmlscope)
{
    QQmlEngine *engine = ctxt->engine;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    v8::HandleScope handle_scope;
    v8::Context::Scope ctxtscope(ep->v8engine()->context());

    v8::TryCatch tc;
    v8::Local<v8::Object> scopeobject = ep->v8engine()->qmlScope(ctxt, scope);
    v8::Local<v8::Script> script = ep->v8engine()->qmlModeCompile(code, codeLength, filename, line);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function compilation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    v8::Local<v8::Value> result = script->Run(scopeobject);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function evaluation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    if (qmlscope) *qmlscope = qPersistentNew<v8::Object>(scopeobject);
    return qPersistentNew<v8::Function>(v8::Local<v8::Function>::Cast(result));
}

// Callee owns the persistent handle
v8::Persistent<v8::Function>
QQmlJavaScriptExpression::evalFunction(QQmlContextData *ctxt, QObject *scope,
                                       const QString &code, const QString &filename, int line,
                                       v8::Persistent<v8::Object> *qmlscope)
{
    QQmlEngine *engine = ctxt->engine;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    v8::HandleScope handle_scope;
    v8::Context::Scope ctxtscope(ep->v8engine()->context());

    v8::TryCatch tc;
    v8::Local<v8::Object> scopeobject = ep->v8engine()->qmlScope(ctxt, scope);
    v8::Local<v8::Script> script = ep->v8engine()->qmlModeCompile(code, filename, line);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function compilation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    v8::Local<v8::Value> result = script->Run(scopeobject);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function evaluation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    if (qmlscope) *qmlscope = qPersistentNew<v8::Object>(scopeobject);
    return qPersistentNew<v8::Function>(v8::Local<v8::Function>::Cast(result));
}

void QQmlJavaScriptExpression::clearGuards()
{
    while (Guard *g = activeGuards.takeFirst())
        g->Delete();
}

QT_END_NAMESPACE
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

#include "qqmlvmemetaobject_p.h"


#include "qqml.h"
#include <private/qqmlrefcount_p.h>
#include "qqmlexpression.h"
#include "qqmlexpression_p.h"
#include "qqmlcontext_p.h"
#include "qqmlbinding_p.h"
#include "qqmlpropertyvalueinterceptor_p.h"

#include <private/qv8variantresource_p.h>

Q_DECLARE_METATYPE(QJSValue);

QT_BEGIN_NAMESPACE

class QQmlVMEVariant
{
public:
    inline QQmlVMEVariant();
    inline ~QQmlVMEVariant();

    inline const void *dataPtr() const;
    inline void *dataPtr();
    inline int dataType() const;

    inline QObject *asQObject();
    inline const QVariant &asQVariant();
    inline int asInt();
    inline bool asBool();
    inline double asDouble();
    inline const QString &asQString();
    inline const QUrl &asQUrl();
    inline const QColor &asQColor();
    inline const QTime &asQTime();
    inline const QDate &asQDate();
    inline const QDateTime &asQDateTime();
    inline const QJSValue &asQJSValue();

    inline void setValue(QObject *);
    inline void setValue(const QVariant &);
    inline void setValue(int);
    inline void setValue(bool);
    inline void setValue(double);
    inline void setValue(const QString &);
    inline void setValue(const QUrl &);
    inline void setValue(const QColor &);
    inline void setValue(const QTime &);
    inline void setValue(const QDate &);
    inline void setValue(const QDateTime &);
    inline void setValue(const QJSValue &);
private:
    int type;
    void *data[4]; // Large enough to hold all types

    inline void cleanup();
};

class QQmlVMEMetaObjectEndpoint : public QQmlNotifierEndpoint
{
public:
    QQmlVMEMetaObjectEndpoint();
    static void vmecallback(QQmlNotifierEndpoint *);
    void tryConnect();

    QFlagPointer<QQmlVMEMetaObject> metaObject;
};


QQmlVMEVariant::QQmlVMEVariant()
: type(QVariant::Invalid)
{
}

QQmlVMEVariant::~QQmlVMEVariant()
{
    cleanup();
}

void QQmlVMEVariant::cleanup()
{
    if (type == QVariant::Invalid) {
    } else if (type == QMetaType::Int ||
               type == QMetaType::Bool ||
               type == QMetaType::Double) {
        type = QVariant::Invalid;
    } else if (type == QMetaType::QObjectStar) {
        ((QQmlGuard<QObject>*)dataPtr())->~QQmlGuard<QObject>();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QString) {
        ((QString *)dataPtr())->~QString();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QUrl) {
        ((QUrl *)dataPtr())->~QUrl();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QColor) {
        ((QColor *)dataPtr())->~QColor();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QTime) {
        ((QTime *)dataPtr())->~QTime();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QDate) {
        ((QDate *)dataPtr())->~QDate();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QDateTime) {
        ((QDateTime *)dataPtr())->~QDateTime();
        type = QVariant::Invalid;
    } else if (type == qMetaTypeId<QVariant>()) {
        ((QVariant *)dataPtr())->~QVariant();
        type = QVariant::Invalid;
    } else if (type == qMetaTypeId<QJSValue>()) {
        ((QJSValue *)dataPtr())->~QJSValue();
        type = QVariant::Invalid;
    }

}

int QQmlVMEVariant::dataType() const
{
    return type;
}

const void *QQmlVMEVariant::dataPtr() const
{
    return &data;
}

void *QQmlVMEVariant::dataPtr() 
{
    return &data;
}

QObject *QQmlVMEVariant::asQObject() 
{
    if (type != QMetaType::QObjectStar) 
        setValue((QObject *)0);

    return *(QQmlGuard<QObject> *)(dataPtr());
}

const QVariant &QQmlVMEVariant::asQVariant() 
{
    if (type != QMetaType::QVariant)
        setValue(QVariant());

    return *(QVariant *)(dataPtr());
}

int QQmlVMEVariant::asInt() 
{
    if (type != QMetaType::Int)
        setValue(int(0));

    return *(int *)(dataPtr());
}

bool QQmlVMEVariant::asBool() 
{
    if (type != QMetaType::Bool)
        setValue(bool(false));

    return *(bool *)(dataPtr());
}

double QQmlVMEVariant::asDouble() 
{
    if (type != QMetaType::Double)
        setValue(double(0));

    return *(double *)(dataPtr());
}

const QString &QQmlVMEVariant::asQString() 
{
    if (type != QMetaType::QString)
        setValue(QString());

    return *(QString *)(dataPtr());
}

const QUrl &QQmlVMEVariant::asQUrl() 
{
    if (type != QMetaType::QUrl)
        setValue(QUrl());

    return *(QUrl *)(dataPtr());
}

const QColor &QQmlVMEVariant::asQColor() 
{
    if (type != QMetaType::QColor)
        setValue(QColor());

    return *(QColor *)(dataPtr());
}

const QTime &QQmlVMEVariant::asQTime() 
{
    if (type != QMetaType::QTime)
        setValue(QTime());

    return *(QTime *)(dataPtr());
}

const QDate &QQmlVMEVariant::asQDate() 
{
    if (type != QMetaType::QDate)
        setValue(QDate());

    return *(QDate *)(dataPtr());
}

const QDateTime &QQmlVMEVariant::asQDateTime() 
{
    if (type != QMetaType::QDateTime)
        setValue(QDateTime());

    return *(QDateTime *)(dataPtr());
}

const QJSValue &QQmlVMEVariant::asQJSValue()
{
    if (type != qMetaTypeId<QJSValue>())
        setValue(QJSValue());

    return *(QJSValue *)(dataPtr());
}

void QQmlVMEVariant::setValue(QObject *v)
{
    if (type != QMetaType::QObjectStar) {
        cleanup();
        type = QMetaType::QObjectStar;
        new (dataPtr()) QQmlGuard<QObject>();
    }
    *(QQmlGuard<QObject>*)(dataPtr()) = v;
}

void QQmlVMEVariant::setValue(const QVariant &v)
{
    if (type != qMetaTypeId<QVariant>()) {
        cleanup();
        type = qMetaTypeId<QVariant>();
        new (dataPtr()) QVariant(v);
    } else {
        *(QVariant *)(dataPtr()) = v;
    }
}

void QQmlVMEVariant::setValue(int v)
{
    if (type != QMetaType::Int) {
        cleanup();
        type = QMetaType::Int;
    }
    *(int *)(dataPtr()) = v;
}

void QQmlVMEVariant::setValue(bool v)
{
    if (type != QMetaType::Bool) {
        cleanup();
        type = QMetaType::Bool;
    }
    *(bool *)(dataPtr()) = v;
}

void QQmlVMEVariant::setValue(double v)
{
    if (type != QMetaType::Double) {
        cleanup();
        type = QMetaType::Double;
    }
    *(double *)(dataPtr()) = v;
}

void QQmlVMEVariant::setValue(const QString &v)
{
    if (type != QMetaType::QString) {
        cleanup();
        type = QMetaType::QString;
        new (dataPtr()) QString(v);
    } else {
        *(QString *)(dataPtr()) = v;
    }
}

void QQmlVMEVariant::setValue(const QUrl &v)
{
    if (type != QMetaType::QUrl) {
        cleanup();
        type = QMetaType::QUrl;
        new (dataPtr()) QUrl(v);
    } else {
        *(QUrl *)(dataPtr()) = v;
    }
}

void QQmlVMEVariant::setValue(const QColor &v)
{
    if (type != QMetaType::QColor) {
        cleanup();
        type = QMetaType::QColor;
        new (dataPtr()) QColor(v);
    } else {
        *(QColor *)(dataPtr()) = v;
    }
}

void QQmlVMEVariant::setValue(const QTime &v)
{
    if (type != QMetaType::QTime) {
        cleanup();
        type = QMetaType::QTime;
        new (dataPtr()) QTime(v);
    } else {
        *(QTime *)(dataPtr()) = v;
    }
}

void QQmlVMEVariant::setValue(const QDate &v)
{
    if (type != QMetaType::QDate) {
        cleanup();
        type = QMetaType::QDate;
        new (dataPtr()) QDate(v);
    } else {
        *(QDate *)(dataPtr()) = v;
    }
}

void QQmlVMEVariant::setValue(const QDateTime &v)
{
    if (type != QMetaType::QDateTime) {
        cleanup();
        type = QMetaType::QDateTime;
        new (dataPtr()) QDateTime(v);
    } else {
        *(QDateTime *)(dataPtr()) = v;
    }
}

void QQmlVMEVariant::setValue(const QJSValue &v)
{
    if (type != qMetaTypeId<QJSValue>()) {
        cleanup();
        type = qMetaTypeId<QJSValue>();
        new (dataPtr()) QJSValue(v);
    } else {
        *(QJSValue *)(dataPtr()) = v;
    }
}

QQmlVMEMetaObjectEndpoint::QQmlVMEMetaObjectEndpoint()
{
    callback = &vmecallback;
}

void QQmlVMEMetaObjectEndpoint::vmecallback(QQmlNotifierEndpoint *e)
{
    QQmlVMEMetaObjectEndpoint *vmee = static_cast<QQmlVMEMetaObjectEndpoint*>(e);
    vmee->tryConnect();
}

void QQmlVMEMetaObjectEndpoint::tryConnect()
{
    int aliasId = this - metaObject->aliasEndpoints;

    if (metaObject.flag()) {
        // This is actually notify
        int sigIdx = metaObject->methodOffset + aliasId + metaObject->metaData->propertyCount;
        QMetaObject::activate(metaObject->object, sigIdx, 0);
    } else {
        QQmlVMEMetaData::AliasData *d = metaObject->metaData->aliasData() + aliasId;
        if (!d->isObjectAlias()) {
            QQmlContextData *ctxt = metaObject->ctxt;
            QObject *target = ctxt->idValues[d->contextIdx].data();
            if (!target)
                return;

            QMetaProperty prop = target->metaObject()->property(d->propertyIndex());
            if (prop.hasNotifySignal())
                connect(target, prop.notifySignalIndex());
        }

        metaObject.setFlag();
    }
}

QQmlVMEMetaObject::QQmlVMEMetaObject(QObject *obj,
                                                     const QMetaObject *other, 
                                                     const QQmlVMEMetaData *meta,
                                                     QQmlCompiledData *cdata)
: QV8GCCallback::Node(GcPrologueCallback), object(obj), compiledData(cdata),
  ctxt(QQmlData::get(obj, true)->outerContext), metaData(meta), data(0),
  aliasEndpoints(0), firstVarPropertyIndex(-1), varPropertiesInitialized(false),
  v8methods(0), parent(0)
{
    compiledData->addref();

    *static_cast<QMetaObject *>(this) = *other;
    this->d.superdata = obj->metaObject();

    QObjectPrivate *op = QObjectPrivate::get(obj);
    if (op->metaObject)
        parent = static_cast<QAbstractDynamicMetaObject*>(op->metaObject);
    op->metaObject = this;

    propOffset = QAbstractDynamicMetaObject::propertyOffset();
    methodOffset = QAbstractDynamicMetaObject::methodOffset();

    data = new QQmlVMEVariant[metaData->propertyCount - metaData->varPropertyCount];

    aConnected.resize(metaData->aliasCount);
    int list_type = qMetaTypeId<QQmlListProperty<QObject> >();

    // ### Optimize
    for (int ii = 0; ii < metaData->propertyCount - metaData->varPropertyCount; ++ii) {
        int t = (metaData->propertyData() + ii)->propertyType;
        if (t == list_type) {
            listProperties.append(List(methodOffset + ii));
            data[ii].setValue(listProperties.count() - 1);
        } 
    }

    firstVarPropertyIndex = metaData->propertyCount - metaData->varPropertyCount;
    if (metaData->varPropertyCount)
        QV8GCCallback::addGcCallbackNode(this);
}

QQmlVMEMetaObject::~QQmlVMEMetaObject()
{
    compiledData->release();
    delete parent;
    delete [] data;
    delete [] aliasEndpoints;

    for (int ii = 0; v8methods && ii < metaData->methodCount; ++ii) {
        qPersistentDispose(v8methods[ii]);
    }
    delete [] v8methods;

    if (metaData->varPropertyCount)
        qPersistentDispose(varProperties); // if not weak, will not have been cleaned up by the callback.
}

int QQmlVMEMetaObject::metaCall(QMetaObject::Call c, int _id, void **a)
{
    int id = _id;
    if(c == QMetaObject::WriteProperty) {
        int flags = *reinterpret_cast<int*>(a[3]);
        if (!(flags & QQmlPropertyPrivate::BypassInterceptor)
            && !aInterceptors.isEmpty()
            && aInterceptors.testBit(id)) {
            QPair<int, QQmlPropertyValueInterceptor*> pair = interceptors.value(id);
            int valueIndex = pair.first;
            QQmlPropertyValueInterceptor *vi = pair.second;
            int type = property(id).userType();

            if (type != QVariant::Invalid) {
                if (valueIndex != -1) {
                    QQmlEnginePrivate *ep = ctxt?QQmlEnginePrivate::get(ctxt->engine):0;
                    QQmlValueType *valueType = 0;
                    if (ep) valueType = ep->valueTypes[type];
                    else valueType = QQmlValueTypeFactory::valueType(type);
                    Q_ASSERT(valueType);

                    valueType->setValue(QVariant(type, a[0]));
                    QMetaProperty valueProp = valueType->metaObject()->property(valueIndex);
                    vi->write(valueProp.read(valueType));

                    if (!ep) delete valueType;
                    return -1;
                } else {
                    vi->write(QVariant(type, a[0]));
                    return -1;
                }
            }
        }
    }
    if (c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty || c == QMetaObject::ResetProperty) {
        if (id >= propOffset) {
            id -= propOffset;

            if (id < metaData->propertyCount) {
               int t = (metaData->propertyData() + id)->propertyType;
                bool needActivate = false;

                if (id >= firstVarPropertyIndex) {
                    Q_ASSERT(t == QMetaType::QVariant);
                    // the context can be null if accessing var properties from cpp after re-parenting an item.
                    QQmlEnginePrivate *ep = (ctxt == 0 || ctxt->engine == 0) ? 0 : QQmlEnginePrivate::get(ctxt->engine);
                    QV8Engine *v8e = (ep == 0) ? 0 : ep->v8engine();
                    if (v8e) {
                        v8::HandleScope handleScope;
                        v8::Context::Scope contextScope(v8e->context());
                        if (c == QMetaObject::ReadProperty) {
                            *reinterpret_cast<QVariant *>(a[0]) = readPropertyAsVariant(id);
                        } else if (c == QMetaObject::WriteProperty) {
                            writeProperty(id, *reinterpret_cast<QVariant *>(a[0]));
                        }
                    } else if (c == QMetaObject::ReadProperty) {
                        // if the context was disposed, we just return an invalid variant from read.
                        *reinterpret_cast<QVariant *>(a[0]) = QVariant();
                    }

                } else {

                    if (c == QMetaObject::ReadProperty) {
                        switch(t) {
                        case QVariant::Int:
                            *reinterpret_cast<int *>(a[0]) = data[id].asInt();
                            break;
                        case QVariant::Bool:
                            *reinterpret_cast<bool *>(a[0]) = data[id].asBool();
                            break;
                        case QVariant::Double:
                            *reinterpret_cast<double *>(a[0]) = data[id].asDouble();
                            break;
                        case QVariant::String:
                            *reinterpret_cast<QString *>(a[0]) = data[id].asQString();
                            break;
                        case QVariant::Url:
                            *reinterpret_cast<QUrl *>(a[0]) = data[id].asQUrl();
                            break;
                        case QVariant::Color:
                            *reinterpret_cast<QColor *>(a[0]) = data[id].asQColor();
                            break;
                        case QVariant::Date:
                            *reinterpret_cast<QDate *>(a[0]) = data[id].asQDate();
                            break;
                        case QVariant::DateTime:
                            *reinterpret_cast<QDateTime *>(a[0]) = data[id].asQDateTime();
                            break;
                        case QMetaType::QObjectStar:
                            *reinterpret_cast<QObject **>(a[0]) = data[id].asQObject();
                            break;
                        case QMetaType::QVariant:
                            *reinterpret_cast<QVariant *>(a[0]) = readPropertyAsVariant(id);
                            break;
                        default:
                            break;
                        }
                        if (t == qMetaTypeId<QQmlListProperty<QObject> >()) {
                            int listIndex = data[id].asInt();
                            const List *list = &listProperties.at(listIndex);
                            *reinterpret_cast<QQmlListProperty<QObject> *>(a[0]) = 
                                QQmlListProperty<QObject>(object, (void *)list,
                                                                  list_append, list_count, list_at, 
                                                                  list_clear);
                        }

                    } else if (c == QMetaObject::WriteProperty) {

                        switch(t) {
                        case QVariant::Int:
                            needActivate = *reinterpret_cast<int *>(a[0]) != data[id].asInt();
                            data[id].setValue(*reinterpret_cast<int *>(a[0]));
                            break;
                        case QVariant::Bool:
                            needActivate = *reinterpret_cast<bool *>(a[0]) != data[id].asBool();
                            data[id].setValue(*reinterpret_cast<bool *>(a[0]));
                            break;
                        case QVariant::Double:
                            needActivate = *reinterpret_cast<double *>(a[0]) != data[id].asDouble();
                            data[id].setValue(*reinterpret_cast<double *>(a[0]));
                            break;
                        case QVariant::String:
                            needActivate = *reinterpret_cast<QString *>(a[0]) != data[id].asQString();
                            data[id].setValue(*reinterpret_cast<QString *>(a[0]));
                            break;
                        case QVariant::Url:
                            needActivate = *reinterpret_cast<QUrl *>(a[0]) != data[id].asQUrl();
                            data[id].setValue(*reinterpret_cast<QUrl *>(a[0]));
                            break;
                        case QVariant::Color:
                            needActivate = *reinterpret_cast<QColor *>(a[0]) != data[id].asQColor();
                            data[id].setValue(*reinterpret_cast<QColor *>(a[0]));
                            break;
                        case QVariant::Date:
                            needActivate = *reinterpret_cast<QDate *>(a[0]) != data[id].asQDate();
                            data[id].setValue(*reinterpret_cast<QDate *>(a[0]));
                            break;
                        case QVariant::DateTime:
                            needActivate = *reinterpret_cast<QDateTime *>(a[0]) != data[id].asQDateTime();
                            data[id].setValue(*reinterpret_cast<QDateTime *>(a[0]));
                            break;
                        case QMetaType::QObjectStar:
                            needActivate = *reinterpret_cast<QObject **>(a[0]) != data[id].asQObject();
                            data[id].setValue(*reinterpret_cast<QObject **>(a[0]));
                            break;
                        case QMetaType::QVariant:
                            writeProperty(id, *reinterpret_cast<QVariant *>(a[0]));
                            break;
                        default:
                            break;
                        }
                    }

                }

                if (c == QMetaObject::WriteProperty && needActivate) {
                    activate(object, methodOffset + id, 0);
                }

                return -1;
            }

            id -= metaData->propertyCount;

            if (id < metaData->aliasCount) {

                QQmlVMEMetaData::AliasData *d = metaData->aliasData() + id;

                if (d->flags & QML_ALIAS_FLAG_PTR && c == QMetaObject::ReadProperty) 
                        *reinterpret_cast<void **>(a[0]) = 0;

                if (!ctxt) return -1;

                QQmlContext *context = ctxt->asQQmlContext();
                QQmlContextPrivate *ctxtPriv = QQmlContextPrivate::get(context);

                QObject *target = ctxtPriv->data->idValues[d->contextIdx].data();
                if (!target) 
                    return -1;

                connectAlias(id);

                if (d->isObjectAlias()) {
                    *reinterpret_cast<QObject **>(a[0]) = target;
                    return -1;
                } 
                
                // Remove binding (if any) on write
                if(c == QMetaObject::WriteProperty) {
                    int flags = *reinterpret_cast<int*>(a[3]);
                    if (flags & QQmlPropertyPrivate::RemoveBindingOnAliasWrite) {
                        QQmlData *targetData = QQmlData::get(target);
                        if (targetData && targetData->hasBindingBit(d->propertyIndex())) {
                            QQmlAbstractBinding *binding = QQmlPropertyPrivate::setBinding(target, d->propertyIndex(), d->isValueTypeAlias()?d->valueTypeIndex():-1, 0);
                            if (binding) binding->destroy();
                        }
                    }
                }
                
                if (d->isValueTypeAlias()) {
                    // Value type property
                    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(ctxt->engine);

                    QQmlValueType *valueType = ep->valueTypes[d->valueType()];
                    Q_ASSERT(valueType);

                    valueType->read(target, d->propertyIndex());
                    int rv = QMetaObject::metacall(valueType, c, d->valueTypeIndex(), a);
                    
                    if (c == QMetaObject::WriteProperty)
                        valueType->write(target, d->propertyIndex(), 0x00);

                    return rv;

                } else {
                    return QMetaObject::metacall(target, c, d->propertyIndex(), a);
                }

            }
            return -1;

        }

    } else if(c == QMetaObject::InvokeMetaMethod) {

        if (id >= methodOffset) {

            id -= methodOffset;
            int plainSignals = metaData->signalCount + metaData->propertyCount +
                               metaData->aliasCount;
            if (id < plainSignals) {
                QMetaObject::activate(object, _id, a);
                return -1;
            }

            id -= plainSignals;

            if (id < metaData->methodCount) {
                if (!ctxt->engine)
                    return -1; // We can't run the method

                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(ctxt->engine);
                ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.

                v8::Handle<v8::Function> function = method(id);
                if (function.IsEmpty()) {
                    // The function was not compiled.  There are some exceptional cases which the
                    // expression rewriter does not rewrite properly (e.g., \r-terminated lines
                    // are not rewritten correctly but this bug is deemed out-of-scope to fix for
                    // performance reasons; see QTBUG-24064) and thus compilation will have failed.
                    QQmlError e;
                    e.setDescription(QString(QLatin1String("Exception occurred during compilation of function: %1")).
                                     arg(QLatin1String(QMetaObject::method(_id).signature())));
                    ep->warning(e);
                    return -1; // The dynamic method with that id is not available.
                }

                QQmlVMEMetaData::MethodData *data = metaData->methodData() + id;

                v8::HandleScope handle_scope;
                v8::Context::Scope scope(ep->v8engine()->context());
                v8::Handle<v8::Value> *args = 0;

                if (data->parameterCount) {
                    args = new v8::Handle<v8::Value>[data->parameterCount];
                    for (int ii = 0; ii < data->parameterCount; ++ii) 
                        args[ii] = ep->v8engine()->fromVariant(*(QVariant *)a[ii + 1]);
                }

                v8::TryCatch try_catch;

                v8::Local<v8::Value> result = function->Call(ep->v8engine()->global(), data->parameterCount, args);

                QVariant rv;
                if (try_catch.HasCaught()) {
                    QQmlError error;
                    QQmlExpressionPrivate::exceptionToError(try_catch.Message(), error);
                    if (error.isValid())
                        ep->warning(error);
                    if (a[0]) *(QVariant *)a[0] = QVariant();
                } else {
                    if (a[0]) *(QVariant *)a[0] = ep->v8engine()->toVariant(result, 0);
                }

                ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
                return -1;
            }
            return -1;
        }
    }

    if (parent)
        return parent->metaCall(c, _id, a);
    else
        return object->qt_metacall(c, _id, a);
}

v8::Handle<v8::Function> QQmlVMEMetaObject::method(int index)
{
    if (!v8methods) 
        v8methods = new v8::Persistent<v8::Function>[metaData->methodCount];

    if (v8methods[index].IsEmpty()) {
        QQmlVMEMetaData::MethodData *data = metaData->methodData() + index;

        const char *body = ((const char*)metaData) + data->bodyOffset;
        int bodyLength = data->bodyLength;

        // XXX We should evaluate all methods in a single big script block to 
        // improve the call time between dynamic methods defined on the same
        // object
        v8methods[index] = QQmlExpressionPrivate::evalFunction(ctxt, object, body,
                                                                       bodyLength,
                                                                       ctxt->urlString,
                                                                       data->lineNumber);
    }

    return v8methods[index];
}

v8::Handle<v8::Value> QQmlVMEMetaObject::readVarProperty(int id)
{
    Q_ASSERT(id >= firstVarPropertyIndex);

    ensureVarPropertiesAllocated();
    return varProperties->Get(id - firstVarPropertyIndex);
}

QVariant QQmlVMEMetaObject::readPropertyAsVariant(int id)
{
    if (id >= firstVarPropertyIndex) {
        ensureVarPropertiesAllocated();
        return QQmlEnginePrivate::get(ctxt->engine)->v8engine()->toVariant(varProperties->Get(id - firstVarPropertyIndex), -1);
    } else {
        if (data[id].dataType() == QMetaType::QObjectStar) {
            return QVariant::fromValue(data[id].asQObject());
        } else {
            return data[id].asQVariant();
        }
    }
}

void QQmlVMEMetaObject::writeVarProperty(int id, v8::Handle<v8::Value> value)
{
    Q_ASSERT(id >= firstVarPropertyIndex);
    ensureVarPropertiesAllocated();

    // Importantly, if the current value is a scarce resource, we need to ensure that it
    // gets automatically released by the engine if no other references to it exist.
    v8::Local<v8::Value> oldv = varProperties->Get(id - firstVarPropertyIndex);
    if (oldv->IsObject()) {
        QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(oldv));
        if (r) {
            r->removeVmePropertyReference();
        }
    }

    // And, if the new value is a scarce resource, we need to ensure that it does not get
    // automatically released by the engine until no other references to it exist.
    if (value->IsObject()) {
        QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(value));
        if (r) {
            r->addVmePropertyReference();
        }
    }

    // Write the value and emit change signal as appropriate.
    varProperties->Set(id - firstVarPropertyIndex, value);
    activate(object, methodOffset + id, 0);
}

void QQmlVMEMetaObject::writeProperty(int id, const QVariant &value)
{
    if (id >= firstVarPropertyIndex) {
        ensureVarPropertiesAllocated();

        // Importantly, if the current value is a scarce resource, we need to ensure that it
        // gets automatically released by the engine if no other references to it exist.
        v8::Local<v8::Value> oldv = varProperties->Get(id - firstVarPropertyIndex);
        if (oldv->IsObject()) {
            QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(oldv));
            if (r) {
                r->removeVmePropertyReference();
            }
        }

        // And, if the new value is a scarce resource, we need to ensure that it does not get
        // automatically released by the engine until no other references to it exist.
        v8::Handle<v8::Value> newv = QQmlEnginePrivate::get(ctxt->engine)->v8engine()->fromVariant(value);
        if (newv->IsObject()) {
            QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(newv));
            if (r) {
                r->addVmePropertyReference();
            }
        }

        // Write the value and emit change signal as appropriate.
        QVariant currentValue = readPropertyAsVariant(id);
        varProperties->Set(id - firstVarPropertyIndex, newv);
        if ((currentValue.userType() != value.userType() || currentValue != value))
            activate(object, methodOffset + id, 0);
    } else {
        bool needActivate = false;
        if (value.userType() == QMetaType::QObjectStar) {
            QObject *o = qvariant_cast<QObject *>(value);
            needActivate = (data[id].dataType() != QMetaType::QObjectStar || data[id].asQObject() != o);
            data[id].setValue(qvariant_cast<QObject *>(value));
        } else {
            needActivate = (data[id].dataType() != qMetaTypeId<QVariant>() ||
                            data[id].asQVariant().userType() != value.userType() ||
                            data[id].asQVariant() != value);
            data[id].setValue(value);
        }

        if (needActivate)
            activate(object, methodOffset + id, 0);
    }
}

void QQmlVMEMetaObject::listChanged(int id)
{
    activate(object, methodOffset + id, 0);
}

void QQmlVMEMetaObject::list_append(QQmlListProperty<QObject> *prop, QObject *o)
{
    List *list = static_cast<List *>(prop->data);
    list->append(o);
    QMetaObject::activate(prop->object, list->notifyIndex, 0);
}

int QQmlVMEMetaObject::list_count(QQmlListProperty<QObject> *prop)
{
    return static_cast<List *>(prop->data)->count();
}

QObject *QQmlVMEMetaObject::list_at(QQmlListProperty<QObject> *prop, int index)
{
    return static_cast<List *>(prop->data)->at(index);
}

void QQmlVMEMetaObject::list_clear(QQmlListProperty<QObject> *prop)
{
    List *list = static_cast<List *>(prop->data);
    list->clear();
    QMetaObject::activate(prop->object, list->notifyIndex, 0);
}

void QQmlVMEMetaObject::registerInterceptor(int index, int valueIndex, QQmlPropertyValueInterceptor *interceptor)
{
    if (aInterceptors.isEmpty())
        aInterceptors.resize(propertyCount() + metaData->propertyCount);
    aInterceptors.setBit(index);
    interceptors.insert(index, qMakePair(valueIndex, interceptor));
}

int QQmlVMEMetaObject::vmeMethodLineNumber(int index)
{
    if (index < methodOffset) {
        Q_ASSERT(parent);
        return static_cast<QQmlVMEMetaObject *>(parent)->vmeMethodLineNumber(index);
    }

    int plainSignals = metaData->signalCount + metaData->propertyCount + metaData->aliasCount;
    Q_ASSERT(index >= (methodOffset + plainSignals) && index < (methodOffset + plainSignals + metaData->methodCount));

    int rawIndex = index - methodOffset - plainSignals;

    QQmlVMEMetaData::MethodData *data = metaData->methodData() + rawIndex;
    return data->lineNumber;
}

v8::Handle<v8::Function> QQmlVMEMetaObject::vmeMethod(int index)
{
    if (index < methodOffset) {
        Q_ASSERT(parent);
        return static_cast<QQmlVMEMetaObject *>(parent)->vmeMethod(index);
    }
    int plainSignals = metaData->signalCount + metaData->propertyCount + metaData->aliasCount;
    Q_ASSERT(index >= (methodOffset + plainSignals) && index < (methodOffset + plainSignals + metaData->methodCount));
    return method(index - methodOffset - plainSignals);
}

// Used by debugger
void QQmlVMEMetaObject::setVmeMethod(int index, v8::Persistent<v8::Function> value)
{
    if (index < methodOffset) {
        Q_ASSERT(parent);
        return static_cast<QQmlVMEMetaObject *>(parent)->setVmeMethod(index, value);
    }
    int plainSignals = metaData->signalCount + metaData->propertyCount + metaData->aliasCount;
    Q_ASSERT(index >= (methodOffset + plainSignals) && index < (methodOffset + plainSignals + metaData->methodCount));

    if (!v8methods) 
        v8methods = new v8::Persistent<v8::Function>[metaData->methodCount];

    int methodIndex = index - methodOffset - plainSignals;
    if (!v8methods[methodIndex].IsEmpty()) 
        qPersistentDispose(v8methods[methodIndex]);
    v8methods[methodIndex] = value;
}

v8::Handle<v8::Value> QQmlVMEMetaObject::vmeProperty(int index)
{
    if (index < propOffset) {
        Q_ASSERT(parent);
        return static_cast<QQmlVMEMetaObject *>(parent)->vmeProperty(index);
    }
    return readVarProperty(index - propOffset);
}

void QQmlVMEMetaObject::setVMEProperty(int index, v8::Handle<v8::Value> v)
{
    if (index < propOffset) {
        Q_ASSERT(parent);
        static_cast<QQmlVMEMetaObject *>(parent)->setVMEProperty(index, v);
        return;
    }
    return writeVarProperty(index - propOffset, v);
}

void QQmlVMEMetaObject::ensureVarPropertiesAllocated()
{
    if (!varPropertiesInitialized)
        allocateVarPropertiesArray();
}

// see also: QV8GCCallback::garbageCollectorPrologueCallback()
void QQmlVMEMetaObject::allocateVarPropertiesArray()
{
    v8::HandleScope handleScope;
    v8::Context::Scope cs(QQmlEnginePrivate::get(ctxt->engine)->v8engine()->context());
    varProperties = qPersistentNew(v8::Array::New(metaData->varPropertyCount));
    varProperties.MakeWeak(static_cast<void*>(this), VarPropertiesWeakReferenceCallback);
    varPropertiesInitialized = true;
}

/*
   The "var" properties are stored in a v8::Array which will be strong persistent if the object has cpp-ownership
   and the root QObject in the parent chain does not have JS-ownership.  In the weak persistent handle case,
   this callback will dispose the handle when the v8object which owns the lifetime of the var properties array
   is cleared as a result of all other handles to that v8object being released.
   See QV8GCCallback::garbageCollectorPrologueCallback() for more information.
 */
void QQmlVMEMetaObject::VarPropertiesWeakReferenceCallback(v8::Persistent<v8::Value> object, void* parameter)
{
    QQmlVMEMetaObject *vmemo = static_cast<QQmlVMEMetaObject*>(parameter);
    Q_ASSERT(vmemo);
    qPersistentDispose(object);
    vmemo->varProperties.Clear();
}

void QQmlVMEMetaObject::GcPrologueCallback(QV8GCCallback::Node *node)
{
    QQmlVMEMetaObject *vmemo = static_cast<QQmlVMEMetaObject*>(node);
    Q_ASSERT(vmemo);
    if (!vmemo->varPropertiesInitialized || vmemo->varProperties.IsEmpty() || !vmemo->ctxt || !vmemo->ctxt->engine)
        return;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(vmemo->ctxt->engine);
    ep->v8engine()->addRelationshipForGC(vmemo->object, vmemo->varProperties);
}

bool QQmlVMEMetaObject::aliasTarget(int index, QObject **target, int *coreIndex, int *valueTypeIndex) const
{
    Q_ASSERT(index >= propOffset + metaData->propertyCount);

    *target = 0;
    *coreIndex = -1;
    *valueTypeIndex = -1;

    if (!ctxt)
        return false;

    QQmlVMEMetaData::AliasData *d = metaData->aliasData() + (index - propOffset - metaData->propertyCount);
    QQmlContext *context = ctxt->asQQmlContext();
    QQmlContextPrivate *ctxtPriv = QQmlContextPrivate::get(context);

    *target = ctxtPriv->data->idValues[d->contextIdx].data();
    if (!*target)
        return false;

    if (d->isObjectAlias()) {
    } else if (d->isValueTypeAlias()) {
        *coreIndex = d->propertyIndex();
        *valueTypeIndex = d->valueTypeIndex();
    } else {
        *coreIndex = d->propertyIndex();
    }

    return true;
}

void QQmlVMEMetaObject::connectAlias(int aliasId)
{
    if (!aConnected.testBit(aliasId)) {

        if (!aliasEndpoints)
            aliasEndpoints = new QQmlVMEMetaObjectEndpoint[metaData->aliasCount];

        aConnected.setBit(aliasId);

        QQmlVMEMetaData::AliasData *d = metaData->aliasData() + aliasId;

        QQmlVMEMetaObjectEndpoint *endpoint = aliasEndpoints + aliasId;
        endpoint->metaObject = this;

        endpoint->connect(&ctxt->idValues[d->contextIdx].bindings);

        endpoint->tryConnect();
    }
}

void QQmlVMEMetaObject::connectAliasSignal(int index)
{
    int aliasId = (index - methodOffset) - metaData->propertyCount;
    if (aliasId < 0 || aliasId >= metaData->aliasCount)
        return;

    connectAlias(aliasId);
}

QT_END_NAMESPACE
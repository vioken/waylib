// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wglobal.h"

#include <private/qobject_p_p.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

WObject::WObject(WObjectPrivate &dd, WObject *)
    : w_d_ptr(&dd)
{

}

WObject::~WObject()
{
    W_D(WObject);
    d->invalidate();
}

WObjectPrivate *WObjectPrivate::get(WObject *qq)
{
    return qq->d_func();
}

WObjectPrivate::~WObjectPrivate()
{
    // WObject must destroy by safeDelete or safeDeleteLater
    Q_ASSERT(invalidated);
}

WObjectPrivate::WObjectPrivate(WObject *qq)
    : q_ptr(qq)
{

}

void WObjectPrivate::invalidate()
{
    invalidated = true;
    instantRelease();
    for (const auto &connection : std::as_const(connections))
        QObject::disconnect(connection);
    connections.clear();
}

bool WObject::safeDisconnect(const QObject *receiver)
{
    W_D(WObject);
    bool ok = false;
    for (int i = 0; i < d->connections.size(); ++i) {
        const QMetaObject::Connection &connection = d->connections.at(i);

        static_assert(sizeof(connection) == sizeof(void*),
                      "Please check how to use QMetaObject::Connection::d_ptr");
        auto c_d = *reinterpret_cast<QObjectPrivate::Connection**>(const_cast<QMetaObject::Connection*>(&connection));
        if (c_d->receiver == receiver) {
            if (QObject::disconnect(connection))
                ok = true;
            d->connections.removeAt(i);
            // reset index
            --i;
        }
    }

    return ok;
}

bool WObject::safeDisconnect(const QMetaObject::Connection &connection)
{
    W_D(WObject);
    int index = d->connections.indexOf(connection);
    if (index < 0)
        return false;
    d->connections.removeAt(index);
    return QObject::disconnect(connection);
}

void WObject::safeDelete()
{
    W_D(WObject);
    d->invalidate();
    delete this;
}

void WObject::safeDeleteLater()
{
    W_D(WObject);
    d->invalidate();

    auto object = dynamic_cast<QObject*>(this);
    Q_ASSERT(object);
    object->deleteLater();
}

WAYLIB_SERVER_END_NAMESPACE

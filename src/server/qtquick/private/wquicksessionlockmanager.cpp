// Copyright (C) 2024 ssk-wh <fanpengcheng@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquicksessionlockmanager_p.h"

#include <qwsessionlockv1.h>

#include <QQmlInfo>

extern "C" {
#define static
#include <wlr/types/wlr_session_lock_v1.h>
#undef static
}

WAYLIB_SERVER_BEGIN_NAMESPACE

using QW_NAMESPACE::QWSessionLockManagerV1;
using QW_NAMESPACE::QWSessionLockSurfaceV1;
using QW_NAMESPACE::QWSessionLockV1;

class WQuickSessionLockManagerPrivate : public WObjectPrivate
{
public:
    WQuickSessionLockManagerPrivate(WQuickSessionLockManager *qq)
        : WObjectPrivate(qq)
    {
    }

    W_DECLARE_PUBLIC(WQuickSessionLockManager)

    QWSessionLockManagerV1 *manager = nullptr;
    bool locked = false;
};

WQuickSessionLockManager::WQuickSessionLockManager(QObject *parent)
    : Waylib::Server::WQuickWaylandServerInterface(parent)
    , WObject(*new WQuickSessionLockManagerPrivate(this), nullptr)
{
}

bool WQuickSessionLockManager::locked() const
{
    W_DC(WQuickSessionLockManager);
    return d->locked;
}

void WQuickSessionLockManager::create()
{
    W_D(WQuickSessionLockManager);

    d->manager = QWSessionLockManagerV1::create(server()->handle());
    connect(d->manager, &QWSessionLockManagerV1::newLock, this, [d, this](QWSessionLockV1 *lock) {
        connect(lock,
                &QWSessionLockV1::newSurface,
                this,
                [=, this](QWSessionLockSurfaceV1 *surface) {
                    d->locked = true;
                    Q_EMIT newSurface(surface);
                    Q_EMIT lockedChanged(d->locked);
                });
        connect(lock, &QWSessionLockV1::unlock, this, [=, this] {
            d->locked = false;
            Q_EMIT lockedChanged(d->locked);
        });
    });
}

WAYLIB_SERVER_END_NAMESPACE

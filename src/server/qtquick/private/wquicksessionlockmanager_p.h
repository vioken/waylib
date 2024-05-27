// Copyright (C) 2024 Lu YaNing <luyaning@uniontech.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <wquickwaylandserver.h>

#include <QQmlEngine>

WAYLIB_SERVER_BEGIN_NAMESPACE

class QWSessionLockSurfaceV1;
class WQuickSessionLockManagerPrivate;

class WAYLIB_SERVER_EXPORT WQuickSessionLockManager : public WQuickWaylandServerInterface,
                                                      public WObject
{
    Q_OBJECT
    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged);
    W_DECLARE_PRIVATE(WQuickSessionLockManager)
    QML_NAMED_ELEMENT(SessionLockManager)

public:
    explicit WQuickSessionLockManager(QObject *parent = nullptr);

    bool locked() const;

Q_SIGNALS:
    void newSurface(QWSessionLockSurfaceV1 *);
    void lockedChanged(bool);

private:
    void create() override;
};

WAYLIB_SERVER_END_NAMESPACE

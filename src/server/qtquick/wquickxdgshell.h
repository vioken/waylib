// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <WSurfaceLayout>

#include <QQmlEngine>

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WQuickSurface;
class WQuickXdgShellPrivate;
class WAYLIB_SERVER_EXPORT WQuickXdgShell : public QObject, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WQuickXdgShell)
    Q_PROPERTY(WServer *server READ server WRITE setServer NOTIFY serverChanged)
    QML_NAMED_ELEMENT(XdgShell)

public:
    explicit WQuickXdgShell(QObject *parent = nullptr);

    WServer *server() const;
    void setServer(WServer *newServer);

Q_SIGNALS:
    void serverChanged();
    void surfaceAdded(WQuickSurface *surface);
    void surfaceRemoved(WQuickSurface *surface);
    void surfaceMapped(WQuickSurface *surface);
    void surfaceUnmap(WQuickSurface *surface);
};

WAYLIB_SERVER_END_NAMESPACE

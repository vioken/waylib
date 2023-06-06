// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <WOutputLayout>
Q_MOC_INCLUDE("wserver.h")

#include <QQmlEngine>

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WServer;
class WQuickBackendPrivate;
class WAYLIB_SERVER_EXPORT WQuickBackend : public QObject, public WObject
{
    W_DECLARE_PRIVATE(WQuickBackend)
    Q_OBJECT
    Q_PROPERTY(WServer *server READ server WRITE setServer NOTIFY serverChanged)
    Q_PROPERTY(WOutputLayout *layout READ layout NOTIFY layoutChanged)
    QML_NAMED_ELEMENT(WaylandBackend)

public:
    explicit WQuickBackend(QObject *parent = nullptr);

    WServer *server() const;
    void setServer(WServer *newServer);

    WOutputLayout *layout() const;

Q_SIGNALS:
    void serverChanged();
    void layoutChanged();
    void outputAdded(WOutput *output);
    void outputRemoved(WOutput *output);

private:
    W_PRIVATE_SLOT(void onOutputAdded(WOutput *output))
    W_PRIVATE_SLOT(void onOutputRemoved(WOutput *output))
};

WAYLIB_SERVER_END_NAMESPACE

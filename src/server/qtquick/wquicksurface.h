// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <WSurface>

#include <QQmlEngine>

Q_MOC_INCLUDE("wsurfaceitem.h")

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WSurfaceItem;
class WQuickSurfacePrivate;
class WAYLIB_SERVER_EXPORT WQuickSurface : public QObject, public WObject
{
    W_DECLARE_PRIVATE(WQuickSurface)
    Q_OBJECT
    Q_PROPERTY(QQuickItem* shellItem READ shellItem WRITE setShellItem NOTIFY shellItemChanged)
    Q_PROPERTY(WSurfaceItem* surfaceItem READ surfaceItem WRITE setSurfaceItem NOTIFY surfaceItemChanged)
    Q_PROPERTY(QSizeF size READ size WRITE setSize NOTIFY sizeChanged)
    QML_NAMED_ELEMENT(WaylandSurface)
    QML_UNCREATABLE("Using in C++")

public:
    explicit WQuickSurface(WSurface *surface, QObject *parent = nullptr);
    ~WQuickSurface();

    QQuickItem *shellItem() const;
    void setShellItem(QQuickItem *newShellItem);

    WSurfaceItem *surfaceItem() const;
    void setSurfaceItem(WSurfaceItem *newSurfaceItem);

    bool testAttribute(WSurface::Attribute attr) const;
    void notifyBeginState(WSurface::State state);
    void notifyEndState(WSurface::State state);

    WTextureHandle *texture() const;

    QSizeF size() const;
    void setSize(const QSizeF &newSize);

Q_SIGNALS:
    void shellItemChanged();
    void surfaceItemChanged();
    void requestMap();
    void sizeChanged();

private:
    friend class WQuickXdgShellPrivate;
    friend class WSurfaceItem;
    friend class WSurfaceItemPrivate;
    WSurface *surface() const;
    void invalidate();
};

WAYLIB_SERVER_END_NAMESPACE

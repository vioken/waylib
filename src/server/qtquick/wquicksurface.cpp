// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquicksurface.h"
#include "wsurfaceitem.h"
#include "wthreadutils.h"
#include "wserver.h"

WAYLIB_SERVER_BEGIN_NAMESPACE

class WQuickSurfacePrivate : public WObjectPrivate
{
public:
    WQuickSurfacePrivate(WQuickSurface *qq, WSurface *surface)
        : WObjectPrivate(qq)
        , surface(surface)
    {
    }

    W_DECLARE_PUBLIC(WQuickSurface)

    WSurface *surface;
    QPointer<QQuickItem> shellItem;
    QPointer<WSurfaceItem> surfaceItem;
};

QQuickItem *WQuickSurface::shellItem() const
{
    W_DC(WQuickSurface);
    return d->shellItem;
}

void WQuickSurface::setShellItem(QQuickItem *newShellItem)
{
    W_D(WQuickSurface);
    Q_ASSERT(!d->shellItem);

    if (d->shellItem == newShellItem)
        return;
    d->shellItem = newShellItem;
    Q_EMIT shellItemChanged();
}

WSurfaceItem *WQuickSurface::surfaceItem() const
{
    W_DC(WQuickSurface);
    return d->surfaceItem;
}

void WQuickSurface::setSurfaceItem(WSurfaceItem *newSurfaceItem)
{
    W_D(WQuickSurface);
    Q_ASSERT(!d->surfaceItem);

    if (d->surfaceItem == newSurfaceItem)
        return;
    d->surfaceItem = newSurfaceItem;
    Q_EMIT surfaceItemChanged();
}

QSizeF WQuickSurface::size() const
{
    W_DC(WQuickSurface);
    return d->surface->server()->threadUtil()->exec(this, d->surface, &WSurface::size);
}

void WQuickSurface::setSize(const QSizeF &newSize)
{
    W_DC(WQuickSurface);
    d->surface->server()->threadUtil()->run(this, d->surface, &WSurface::resize, newSize.toSize());
}

bool WQuickSurface::testAttribute(WSurface::Attribute attr) const
{
    W_DC(WQuickSurface);
    return d->surface->server()->threadUtil()->exec(this, d->surface, &WSurface::testAttribute, attr);
}

void WQuickSurface::notifyBeginState(WSurface::State state)
{
    W_DC(WQuickSurface);
    d->surface->server()->threadUtil()->run(this, d->surface, &WSurface::notifyBeginState, state);
}

void WQuickSurface::notifyEndState(WSurface::State state)
{
    W_DC(WQuickSurface);
    d->surface->server()->threadUtil()->run(this, d->surface, &WSurface::notifyEndState, state);
}

WTextureHandle *WQuickSurface::texture() const
{
    W_DC(WQuickSurface);
    return d->surface->server()->threadUtil()->exec(this, d->surface, &WSurface::texture);
}

WSurface *WQuickSurface::surface() const
{
    W_DC(WQuickSurface);
    return d->surface;
}

void WQuickSurface::invalidate()
{
    W_D(WQuickSurface);
    d->shellItem.clear();
    d->surfaceItem.clear();
}

WQuickSurface::WQuickSurface(WSurface *surface, QObject *parent)
    : QObject{parent}
    , WObject(*new WQuickSurfacePrivate(this, surface))
{
    connect(surface, &WSurface::sizeChanged, this, &WQuickSurface::sizeChanged);
}

WQuickSurface::~WQuickSurface()
{

}

WAYLIB_SERVER_END_NAMESPACE

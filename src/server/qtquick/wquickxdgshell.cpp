// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquickxdgshell.h"
#include "wquicksurface.h"
#include "wthreadutils.h"
#include "wseat.h"
#include "woutput.h"
#include "wxdgshell.h"
#include "private/wsurfacelayout_p.h"

#include <QCoreApplication>
#include <QThread>
#include <QQuickItem>

WAYLIB_SERVER_BEGIN_NAMESPACE

class WQuickXdgShellPrivate : public WObjectPrivate, public WSurfaceLayout
{
public:
    WQuickXdgShellPrivate(WQuickXdgShell *qq)
        : WObjectPrivate(qq)
        , WSurfaceLayout()
    {
    }
    ~WQuickXdgShellPrivate() {
        qDeleteAll(surface2Bridge);
    }

    inline WQuickSurface *bridge(WSurface *surface) const {
        return surface2Bridge.value(surface);
    }

    void add(WSurface *surface) override;
    void remove(WSurface *surface) override;

    void map(WSurface *surface) override;
    void unmap(WSurface *surface)  override;

    void requestMove(WSurface *surface, WSeat *seat, quint32 serial) override;
    void requestResize(WSurface *surface, WSeat *seat, Qt::Edges edge, quint32 serial) override;
    void requestMaximize(WSurface *surface) override;
    void requestUnmaximize(WSurface *surface) override;
    void requestActivate(WSurface *surface, WSeat *seat) override;

    bool setPosition(WSurface *surface, const QPointF &pos) override;

    W_DECLARE_PUBLIC(WQuickXdgShell)

    WServer *server = nullptr;
    WXdgShell *shell = nullptr;
    QHash<WSurface*, WQuickSurface*> surface2Bridge;
};

void WQuickXdgShellPrivate::add(WSurface *surface)
{
    Q_ASSERT(!surface2Bridge.contains(surface));
    auto bridge = new WQuickSurface(surface, nullptr);
    surface2Bridge[surface] = bridge;
    Q_EMIT q_func()->surfaceAdded(bridge);

    WSurfaceLayout::add(surface);
}

void WQuickXdgShellPrivate::remove(WSurface *surface)
{
    WSurfaceLayout::remove(surface);

    auto bridge = surface2Bridge.take(surface);
    Q_ASSERT(bridge);

    bridge->invalidate();
    bridge->deleteLater();

    Q_EMIT q_func()->surfaceRemoved(bridge);
}

void WQuickXdgShellPrivate::map(WSurface *surface)
{
    WSurfaceLayout::map(surface);

    auto bridge = this->bridge(surface);
    Q_ASSERT(bridge);
    Q_ASSERT(!bridge->shellItem());

    Q_EMIT q_func()->surfaceMapped(bridge);
}

void WQuickXdgShellPrivate::unmap(WSurface *surface)
{
    WSurfaceLayout::unmap(surface);

    auto bridge = this->bridge(surface);
    Q_ASSERT(bridge);
    Q_ASSERT(bridge->shellItem());

    Q_EMIT q_func()->surfaceUnmap(bridge);
}

void WQuickXdgShellPrivate::requestMove(WSurface *surface, WSeat *seat, quint32 serial)
{
    WSurfaceLayout::requestMove(surface, seat, serial);
}

void WQuickXdgShellPrivate::requestResize(WSurface *surface, WSeat *seat, Qt::Edges edge, quint32 serial)
{
    WSurfaceLayout::requestResize(surface, seat, edge, serial);
}

void WQuickXdgShellPrivate::requestMaximize(WSurface *surface)
{
    WSurfaceLayout::requestMaximize(surface);

    auto bridge = this->bridge(surface);
    Q_ASSERT(bridge);
}

void WQuickXdgShellPrivate::requestUnmaximize(WSurface *surface)
{
    WSurfaceLayout::requestUnmaximize(surface);

    auto bridge = this->bridge(surface);
    Q_ASSERT(bridge);
}

void WQuickXdgShellPrivate::requestActivate(WSurface *surface, WSeat *seat)
{
    WSurfaceLayout::requestActivate(surface, seat);

    if (surface->testAttribute(WSurface::Attribute::DoesNotAcceptFocus))
        return;
    seat->setKeyboardFocusTarget(surface);

    auto bridge = this->bridge(surface);
    Q_ASSERT(bridge);
}

bool WQuickXdgShellPrivate::setPosition(WSurface *surface, const QPointF &pos)
{
    if (!WSurfaceLayout::setPosition(surface, pos))
        return false;

    auto bridge = this->bridge(surface);
    Q_ASSERT(bridge);
    WThreadUtil::gui().run(bridge, bridge->shellItem(), &QQuickItem::setPosition, pos);
    return true;
}

WQuickXdgShell::WQuickXdgShell(QObject *parent)
    : QObject(parent)
    , WObject(*new WQuickXdgShellPrivate(this), nullptr)
{

}

WServer *WQuickXdgShell::server() const
{
    W_DC(WQuickXdgShell);
    return d->server;
}

void WQuickXdgShell::setServer(WServer *newServer)
{
    W_D(WQuickXdgShell);
    if (d->server == newServer)
        return;
    if (d->server) {
        Q_ASSERT(d->shell);
        d->server->detach(d->shell);
        d->shell = nullptr;
    }
    d->server = newServer;
    if (d->server)
        d->shell = d->server->attach<WXdgShell>(d);

    Q_EMIT serverChanged();
}

WAYLIB_SERVER_END_NAMESPACE

// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "rootsurfacecontainer.h"
#include "helper.h"
#include "surfacewrapper.h"

RootSurfaceContainer::RootSurfaceContainer(QQuickItem *parent)
    : SurfaceContainer(parent)
{

}

SurfaceWrapper *RootSurfaceContainer::getSurface(WSurface *surface) const
{
    const auto surfaces = this->surfaces();
    for (const auto &wrapper: surfaces) {
        if (wrapper->surface() == surface)
            return wrapper;
    }
    return nullptr;
}

SurfaceWrapper *RootSurfaceContainer::getSurface(WToplevelSurface *surface) const
{
    const auto surfaces = this->surfaces();
    for (const auto &wrapper: surfaces) {
        if (wrapper->shellSurface() == surface)
            return wrapper;
    }
    return nullptr;
}

void RootSurfaceContainer::addSurface(SurfaceWrapper *)
{
    Q_UNREACHABLE_RETURN();
}

void RootSurfaceContainer::removeSurface(SurfaceWrapper *)
{
    Q_UNREACHABLE_RETURN();
}

void RootSurfaceContainer::addBySubContainer(SurfaceContainer *sub, SurfaceWrapper *surface)
{
    SurfaceContainer::addBySubContainer(sub, surface);

    connect(surface, &SurfaceWrapper::requestMove, this, [this] {
        auto surface = qobject_cast<SurfaceWrapper*>(sender());
        Q_ASSERT(surface);
        Helper::instance()->startMove(surface, 0);
    });
    connect(surface, &SurfaceWrapper::requestResize, this, [this] (Qt::Edges edges) {
        auto surface = qobject_cast<SurfaceWrapper*>(sender());
        Q_ASSERT(surface);
        Helper::instance()->startResize(surface, edges, 0);
    });
    connect(surface, &SurfaceWrapper::surfaceStateChanged, this, [surface, this] {
        if (surface->surfaceState() == SurfaceWrapper::State::Minimized
            || surface->surfaceState() == SurfaceWrapper::State::Tiling)
            return;
        Helper::instance()->activeSurface(surface);
    });
    connect(surface, &SurfaceWrapper::geometryChanged, this, [this, surface] {
        Helper::instance()->updateSurfaceOutputs(surface);
    });

    Helper::instance()->updateSurfaceOwnsOutput(surface);
    Helper::instance()->updateSurfaceOutputs(surface);
    Helper::instance()->activeSurface(surface, Qt::OtherFocusReason);
}

void RootSurfaceContainer::removeBySubContainer(SurfaceContainer *sub, SurfaceWrapper *surface)
{
    SurfaceContainer::removeBySubContainer(sub, surface);
}

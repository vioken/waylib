// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "surfacecontainer.h"
#include "surfacewrapper.h"

SurfaceContainer::SurfaceContainer(QQuickItem *parent)
    : QQuickItem(parent)
{

}

SurfaceContainer::SurfaceContainer(SurfaceContainer *parent)
    : SurfaceContainer(static_cast<QQuickItem*>(parent))
{

}

SurfaceContainer::~SurfaceContainer()
{
    if (!m_surfaces.isEmpty()) {
        qWarning() << "SurfaceContainer destroyed with surfaces still attached:" << m_surfaces;
    }
}

SurfaceContainer *SurfaceContainer::parentContainer() const
{
    return qobject_cast<SurfaceContainer*>(parent());
}

QList<SurfaceContainer*> SurfaceContainer::subContainers() const
{
    return findChildren<SurfaceContainer*>(Qt::FindDirectChildrenOnly);
}

void SurfaceContainer::addSurface(SurfaceWrapper *surface)
{
    doAddSurface(surface, true);
}

void SurfaceContainer::removeSurface(SurfaceWrapper *surface)
{
    doRemoveSurface(surface, true);
}

bool SurfaceContainer::doAddSurface(SurfaceWrapper *surface, bool setContainer)
{
    if (m_surfaces.contains(surface))
        return false;

    if (setContainer) {
        Q_ASSERT(!surface->container());
        surface->setContainer(this);
        surface->setParent(this);
    }

    m_surfaces << surface;
    emit surfaceAdded(surface);

    if (auto p = parentContainer())
        p->addBySubContainer(this, surface);

    return true;
}

bool SurfaceContainer::doRemoveSurface(SurfaceWrapper *surface, bool setContainer)
{
    if (!m_surfaces.contains(surface))
        return false;

    if (setContainer) {
        Q_ASSERT(surface->container() == this);
        surface->setContainer(nullptr);
    }

    m_surfaces.removeOne(surface);
    emit surfaceRemoved(surface);

    if (auto p = parentContainer())
        p->removeBySubContainer(this, surface);

    return true;
}

void SurfaceContainer::addBySubContainer(SurfaceContainer *sub, SurfaceWrapper *surface)
{
    Q_UNUSED(sub);
    doAddSurface(surface, false);
}

void SurfaceContainer::removeBySubContainer(SurfaceContainer *sub, SurfaceWrapper *surface)
{
    Q_UNUSED(sub);
    doRemoveSurface(surface, false);
}

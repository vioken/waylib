// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "workspace.h"
#include "surfacewrapper.h"

WorkspaceContainer::WorkspaceContainer(Workspace *parent)
    : SurfaceContainer(parent)
{

}

Workspace::Workspace(SurfaceContainer *parent)
    : SurfaceContainer(parent)
{
    createContainer(true);
}

void Workspace::addSurface(SurfaceWrapper *surface, int workspaceIndex)
{
    doAddSurface(surface, false);
    auto container = workspaceIndex >= 0
                         ? m_containers.at(workspaceIndex)
                         : m_containers.at(m_currentIndex);

    if (container->m_surfaces.contains(surface))
        return;

    for (auto c : std::as_const(m_containers)) {
        if (c == container)
            continue;
        if (c->surfaces().contains(surface)) {
            c->removeSurface(surface);
            break;
        }
    }

    container->addSurface(surface);
}

void Workspace::removeSurface(SurfaceWrapper *surface)
{
    if (!doRemoveSurface(surface, false))
        return;

    for (auto container : std::as_const(m_containers)) {
        if (container->surfaces().contains(surface)) {
            container->removeSurface(surface);
            break;
        }
    }
}

int Workspace::containerIndexOfSurface(SurfaceWrapper *surface) const
{
    for (int i = 0; i < m_containers.size(); ++i) {
        if (m_containers.at(i)->m_surfaces.contains(surface))
            return i;
    }

    return -1;
}

int Workspace::createContainer(bool visible)
{
    m_containers.append(new WorkspaceContainer(this));
    m_containers.last()->setVisible(visible);
    return m_containers.size() - 1;
}

void Workspace::removeContainer(int index)
{
    if (m_containers.size() == 1)
        return;
    if (index < 0 || index >= m_containers.size())
        return;

    auto container = m_containers.at(index);
    m_containers.removeAt(index);
    m_currentIndex = qMin(m_currentIndex, m_containers.size() - 1);
    auto current = m_containers.at(m_currentIndex);

    const auto tmp = container->surfaces();
    for (auto s : tmp) {
        container->removeSurface(s);
        current->addSurface(s);
    }

    container->deleteLater();
    emit currentChanged();
}

WorkspaceContainer *Workspace::container(int index) const
{
    if (index < 0 || index >= m_containers.size())
        return nullptr;
    return m_containers.at(index);
}

int Workspace::currentIndex() const
{
    return m_currentIndex;
}

void Workspace::setCurrentIndex(int newCurrentIndex)
{
    if (newCurrentIndex < 0 || newCurrentIndex >= m_containers.size())
        return;

    if (m_currentIndex == newCurrentIndex)
        return;
    m_currentIndex = newCurrentIndex;

    for (int i = 0; i < m_containers.size(); ++i) {
        m_containers.at(i)->setVisible(i == m_currentIndex);
    }

    emit currentChanged();
}

void Workspace::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    for (auto container : std::as_const(m_containers)) {
        container->setSize(newGeometry.size());
    }

    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

WorkspaceContainer *Workspace::currentworkspace() const
{
    return m_containers.at(m_currentIndex);
}

// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "workspace.h"
#include "surfacewrapper.h"
#include "output.h"
#include "helper.h"
#include "rootsurfacecontainer.h"

Workspace::Workspace(SurfaceContainer *parent)
    : SurfaceContainer(parent)
{
    // TODO: save and restore from local storage
    static int workspaceGlobalIndex = 0;

    // TODO: save and restore workpsace's name from local storage
    createContainer(QStringLiteral("show-on-all-workspace"), true);
    createContainer(QStringLiteral("workspace-%1").arg(++workspaceGlobalIndex), true);
    createContainer(QStringLiteral("workspace-%1").arg(++workspaceGlobalIndex));
}

void Workspace::addSurface(SurfaceWrapper *surface, int workspaceIndex)
{
    qDebug() << "Move to surface!!!!!!!!!" << surface << workspaceIndex;
    doAddSurface(surface, true);

    if (workspaceIndex < 0)
        workspaceIndex = m_currentIndex;

    auto container = m_containers.at(workspaceIndex);

    if (container->m_model->hasSurface(surface))
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
    if (!surface->ownsOutput())
        surface->setOwnsOutput(rootContainer()->primaryOutput());
}

void Workspace::removeSurface(SurfaceWrapper *surface)
{
    if (!doRemoveSurface(surface, false))
        return;

    for (auto container : std::as_const(m_containers)) {
        if (container->surfaces().contains(surface)) {
            container->removeSurface(surface);
            surface->setWorkspaceId(-1);
            break;
        }
    }
}

int Workspace::containerIndexOfSurface(SurfaceWrapper *surface) const
{
    for (int i = 0; i < m_containers.size(); ++i) {
        if (m_containers.at(i)->m_model->hasSurface(surface))
            return i;
    }

    //if (m_showOnAllWorkspaceModel->m_model->hasSurface(surface))
    //    return -2;

    return -1;
}

int Workspace::createContainer(const QString &name, bool visible)
{
    m_containers.append(new WorkspaceModel(this, m_containers.size()));
    auto newContainer = m_containers.last();
    newContainer->setName(name);
    newContainer->setVisible(visible);
    return newContainer->index();
}

void Workspace::removeContainer(int index)
{
    if (m_containers.size() == 1)
        return;
    if (index < 0 || index >= m_containers.size())
        return;

    auto container = m_containers.at(index);
    m_containers.removeAt(index);

    // reset index
    for (int i = index; i < m_containers.size(); ++i) {
        m_containers.at(i)->setIndex(i);
    }

    auto oldCurrent = this->current();
    m_currentIndex = qMin(m_currentIndex, m_containers.size() - 1);
    auto current = this->current();

    const auto tmp = container->surfaces();
    for (auto s : tmp) {
        container->removeSurface(s);
        if (current)
            current->addSurface(s);
    }

    container->deleteLater();

    if (oldCurrent != current)
        emit currentChanged();
}

WorkspaceModel *Workspace::container(int index) const
{
    //if (index == -2)
    //    return m_showOnAllWorkspaceModel;
    if (index < 0 || index >= m_containers.size())
        return nullptr;
    return m_containers.at(index);
}

int Workspace::count() const
{
    return m_containers.size();
}

int Workspace::currentIndex() const
{
    return m_currentIndex;
}

WorkspaceModel *Workspace::showOnAllWorkspaceModel() const
{
    return m_containers.at(0);
}

void Workspace::setCurrentIndex(int newCurrentIndex)
{
    if (newCurrentIndex < 0 || newCurrentIndex >= m_containers.size())
        return;

    if (m_currentIndex == newCurrentIndex)
        return;
    m_currentIndex = newCurrentIndex;

    if (m_switcher) {
        m_switcher->deleteLater();
    }

    for (int i = 1; i < m_containers.size(); ++i) {
        m_containers.at(i)->setVisible(i == m_currentIndex);
    }

    emit currentChanged();
}

void Workspace::switchToNext()
{
    if (m_currentIndex < m_containers.size() - 1)
        switchTo(m_currentIndex + 1);
}

void Workspace::switchToPrev()
{
    if (m_currentIndex > 1)
        switchTo(m_currentIndex - 1);
}

void Workspace::switchTo(int index)
{
    if (m_switcher)
        return;

    Q_ASSERT(index != m_currentIndex);
    Q_ASSERT(index > 0 && index < m_containers.size());
    auto from = current();
    auto to = m_containers.at(index);
    auto engine = Helper::instance()->qmlEngine();
    from->setVisible(false);
    to->setVisible(false);
    showOnAllWorkspaceModel()->setVisible(false);
    m_switcher = engine->createWorkspaceSwitcher(this, from, to);
}

WorkspaceModel *Workspace::current() const
{
    if (m_currentIndex < 1 || m_currentIndex >= m_containers.size())
        return nullptr;

    return m_containers.at(m_currentIndex);
}

void Workspace::setCurrent(WorkspaceModel *container)
{
    int index = m_containers.indexOf(container);
    if (index < 1)
        return;
    setCurrentIndex(index);
}

void Workspace::updateSurfaceOwnsOutput(SurfaceWrapper *surface)
{
    auto outputs = surface->surface()->outputs();
    if (surface->ownsOutput() && outputs.contains(surface->ownsOutput()->output()))
        return;

    Output *output = nullptr;
    if (!outputs.isEmpty())
        output = Helper::instance()->getOutput(outputs.first());
    if (!output)
        output = rootContainer()->cursorOutput();
    if (!output)
        output = rootContainer()->primaryOutput();
    if (output)
        surface->setOwnsOutput(output);
}

void Workspace::updateSurfacesOwnsOutput()
{
    const auto surfaces = this->surfaces();
    for (auto surface : surfaces) {
        updateSurfaceOwnsOutput(surface);
    }
}


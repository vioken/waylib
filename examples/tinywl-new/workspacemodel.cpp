// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "workspacemodel.h"
#include "surfacewrapper.h"
#include "helper.h"

WorkspaceModel::WorkspaceModel(QObject *parent, int index)
    : QObject(parent)
    , m_model(new SurfaceListModel(this))
    , m_index(index)
{

}

QString WorkspaceModel::name() const
{
    return m_name;
}

void WorkspaceModel::setName(const QString &newName)
{
    if (m_name == newName)
        return;
    m_name = newName;
    Q_EMIT nameChanged();
}

int WorkspaceModel::index() const
{
    return m_index;
}

void WorkspaceModel::setIndex(int newIndex)
{
    if (m_index == newIndex)
        return;
    m_index = newIndex;
    Q_EMIT indexChanged();
}

bool WorkspaceModel::visable() const
{
    return m_visable;
}

void WorkspaceModel::setVisible(bool visible)
{
    if (m_visable == visible)
        return;
    m_visable = visible;
    for (auto surface : m_model->surfaces())
        surface->setVisible(visible);
    Q_EMIT visableChanged();
}

void WorkspaceModel::addSurface(SurfaceWrapper *surface)
{
    m_model->addSurface(surface);
    surface->setVisible(m_visable);
    surface->setWorkspaceId(m_index);
}

void WorkspaceModel::removeSurface(SurfaceWrapper *surface)
{
    m_model->removeSurface(surface);
}
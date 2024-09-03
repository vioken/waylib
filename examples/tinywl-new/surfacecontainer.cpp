// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "surfacecontainer.h"
#include "surfacewrapper.h"

SurfaceListModel::SurfaceListModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

int SurfaceListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_surfaces.count();
}

QVariant SurfaceListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_surfaces.count())
        return {};

    if (role == Qt::DisplayRole)
        return QVariant::fromValue(m_surfaces.at(index.row()));

    return {};
}

QMap<int, QVariant> SurfaceListModel::itemData(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_surfaces.count())
        return {};

    QMap<int, QVariant> data;
    data.insert(Qt::DisplayRole, QVariant::fromValue(m_surfaces.at(index.row())));
    return data;
}

Qt::ItemFlags SurfaceListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}

void SurfaceListModel::addSurface(SurfaceWrapper *surface)
{
    beginInsertRows(QModelIndex(), m_surfaces.count(), m_surfaces.count());
    m_surfaces.append(surface);
    endInsertRows();
}

void SurfaceListModel::removeSurface(SurfaceWrapper *surface)
{
    int index = m_surfaces.indexOf(surface);
    if (index <= 0)
        return;
    beginRemoveRows({}, index, index);
    m_surfaces.removeAt(index);
    endRemoveRows();
}

bool SurfaceListModel::hasSurface(SurfaceWrapper *surface) const
{
    return m_surfaces.contains(surface);
}

SurfaceContainer::SurfaceContainer(QQuickItem *parent)
    : QQuickItem(parent)
    , m_model(new SurfaceListModel(this))
{

}

SurfaceContainer::SurfaceContainer(SurfaceContainer *parent)
    : SurfaceContainer(static_cast<QQuickItem*>(parent))
{

}

SurfaceContainer::~SurfaceContainer()
{
    if (!m_model->surfaces().isEmpty()) {
        qWarning() << "SurfaceContainer destroyed with surfaces still attached:" << m_model->surfaces();
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
    if (m_model->hasSurface(surface))
        return false;

    if (setContainer) {
        Q_ASSERT(!surface->container());
        surface->setContainer(this);
        surface->setParent(this);
    }

    m_model->addSurface(surface);
    emit surfaceAdded(surface);

    if (auto p = parentContainer())
        p->addBySubContainer(this, surface);

    return true;
}

bool SurfaceContainer::doRemoveSurface(SurfaceWrapper *surface, bool setContainer)
{
    if (!m_model->hasSurface(surface))
        return false;

    if (setContainer) {
        Q_ASSERT(surface->container() == this);
        surface->setContainer(nullptr);
    }

    m_model->removeSurface(surface);
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

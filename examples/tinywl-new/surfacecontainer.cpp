// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "surfacecontainer.h"
#include "surfacewrapper.h"
#include "output.h"

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

QHash<int, QByteArray> SurfaceListModel::roleNames() const
{
    return {{Qt::DisplayRole, "surface"}};
}

void SurfaceListModel::addSurface(SurfaceWrapper *surface)
{
    if (m_surfaces.contains(surface))
        return;

    beginInsertRows(QModelIndex(), m_surfaces.count(), m_surfaces.count());
    m_surfaces.append(surface);
    endInsertRows();

    emit surfaceAdded(surface);
}

void SurfaceListModel::removeSurface(SurfaceWrapper *surface)
{
    int index = m_surfaces.indexOf(surface);
    if (index < 0)
        return;
    beginRemoveRows({}, index, index);
    m_surfaces.removeAt(index);
    endRemoveRows();

    emit surfaceRemoved(surface);
}

bool SurfaceListModel::hasSurface(SurfaceWrapper *surface) const
{
    return m_surfaces.contains(surface);
}

SurfaceFilterModel::SurfaceFilterModel(SurfaceListModel *parent)
    : SurfaceListModel(parent)
{
    connect(parent, &SurfaceListModel::surfaceAdded,
            this, &SurfaceFilterModel::initForSourceSurface);
    connect(parent, &SurfaceListModel::surfaceRemoved,
            this, [this] (SurfaceWrapper *surface) {
        removeSurface(surface);
        m_properties.erase(surface);
    });
}

void SurfaceFilterModel::setFilter(std::function<bool(SurfaceWrapper*)> filter)
{
    m_filter = filter;
    m_properties.clear();

    for (auto surface : parent()->surfaces()) {
        initForSourceSurface(surface);
    }
}

void SurfaceFilterModel::initForSourceSurface(SurfaceWrapper *surface)
{
    if (m_filter) {
        makeBindingForSourceSurface(surface);
    }
    updateSurfaceVisibility(surface);
}

void SurfaceFilterModel::makeBindingForSourceSurface(SurfaceWrapper *surface)
{
    Property &p = m_properties[surface];

    if (!p.init) {
        p.init = true;
        p.notifier = p.prop.addNotifier([this, surface] {
            updateSurfaceVisibility(surface);
        });
    }

    p.setBinding([this, surface] {
        return m_filter(surface);
    });
}

void SurfaceFilterModel::updateSurfaceVisibility(SurfaceWrapper *surface)
{
    if (!m_filter) {
        if (hasSurface(surface))
            return;
        addSurface(surface);
        return;
    }

    Q_ASSERT(m_properties.contains(surface));
    Q_ASSERT(parent()->hasSurface(surface));
    const Property &p = m_properties.at(surface);
    Q_ASSERT(p.init);
    if (p.prop.value()) {
        addSurface(surface);
    } else {
        removeSurface(surface);
    }
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

SurfaceContainer *SurfaceContainer::rootContainer() const
{
    SurfaceContainer *root = const_cast<SurfaceContainer*>(this);
    while (auto p = root->parentContainer()) {
        root = p;
    }
    return root;
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

void SurfaceContainer::addOutput(Output *output)
{
    Q_ASSERT(output->isPrimary());
    const auto subContainers = this->subContainers();
    for (auto sub : subContainers) {
        sub->addOutput(output);
    }
}

void SurfaceContainer::removeOutput(Output *output)
{
    const auto subContainers = this->subContainers();
    for (auto sub : subContainers) {
        sub->removeOutput(output);
    }
}

void SurfaceContainer::geometryChange(const QRectF &newGeo, const QRectF &oldGeo)
{
    const auto subContainers = this->subContainers();
    for (SurfaceContainer *c : subContainers) {
        c->setPosition(newGeo.topLeft());
        c->setSize(newGeo.size());
    }

    QQuickItem::geometryChange(newGeo, oldGeo);
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

bool SurfaceContainer::filterSurfaceGeometryChanged(SurfaceWrapper *surface, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (auto p = parentContainer())
        return p->filterSurfaceGeometryChanged(surface, newGeometry, oldGeometry);
    return false;
}

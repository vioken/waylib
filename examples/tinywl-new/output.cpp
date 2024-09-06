// Copyright (C) 2024 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "output.h"
#include "surfacewrapper.h"
#include "helper.h"
#include "rootsurfacecontainer.h"

#include <woutputitem.h>
#include <wlayersurface.h>
#include <wsurfaceitem.h>
#include <wlayersurface.h>
#include <woutputrenderwindow.h>

#include <QQmlEngine>

Output *Output::createPrimary(WOutput *output, QQmlEngine *engine, QObject *parent)
{
    QQmlComponent delegate(engine, "Tinywl", "PrimaryOutput");
    QObject *obj = delegate.create(engine->rootContext());
    WOutputItem *outputItem = qobject_cast<WOutputItem *>(obj);
    Q_ASSERT(outputItem);
    QQmlEngine::setObjectOwnership(outputItem, QQmlEngine::CppOwnership);
    outputItem->setOutput(output);

    auto o = new Output(outputItem, parent);
    o->m_type = Type::Primary;
    obj->setParent(o);

    return o;
}

Output *Output::createCopy(WOutput *output, Output *proxy, QQmlEngine *engine, QObject *parent)
{
    QQmlComponent delegate(engine, "Tinywl", "CopyOutput");
    QObject *obj = delegate.create(engine->rootContext());
    WOutputItem *outputItem = qobject_cast<WOutputItem *>(obj);
    Q_ASSERT(outputItem);
    QQmlEngine::setObjectOwnership(outputItem, QQmlEngine::CppOwnership);
    outputItem->setOutput(output);

    auto o = new Output(outputItem, parent);
    o->m_type = Type::Proxy;
    o->m_proxy = proxy;
    obj->setParent(o);

    return o;
}

Output::Output(WOutputItem *output, QObject *parent)
    : SurfaceListModel(parent)
    , m_item(output)
    , minimizedSurfaces(new SurfaceFilterModel(this))
{
    minimizedSurfaces->setFilter([] (SurfaceWrapper *s) {
        return s->isMinimized();
    });

    connect(output, &WOutputItem::geometryChanged, this, &Output::layoutAllSurfaces);

    auto contentItem = Helper::instance()->window()->contentItem();
    m_taskBar = Helper::instance()->qmlEngine()->createTaskBar(this, contentItem);
    m_taskBar->setZ(RootSurfaceContainer::TaskBarZOrder);
}

Output::~Output()
{
    if (m_taskBar) {
        delete m_taskBar;
        m_taskBar = nullptr;
    }
}

bool Output::isPrimary() const
{
    return m_type == Type::Primary;
}

void Output::addSurface(SurfaceWrapper *surface)
{
    Q_ASSERT(!hasSurface(surface));
    SurfaceListModel::addSurface(surface);

    if (surface->type() == SurfaceWrapper::Type::Layer) {
        auto layer = qobject_cast<WLayerSurface*>(surface->shellSurface());
        layer->safeConnect(&WLayerSurface::ancherChanged, this, &Output::layoutLayerSurfaces);
        layer->safeConnect(&WLayerSurface::leftMarginChanged, this, &Output::layoutLayerSurfaces);
        layer->safeConnect(&WLayerSurface::rightMarginChanged, this, &Output::layoutLayerSurfaces);
        layer->safeConnect(&WLayerSurface::topMarginChanged, this, &Output::layoutLayerSurfaces);
        layer->safeConnect(&WLayerSurface::bottomMarginChanged, this, &Output::layoutLayerSurfaces);
        layer->safeConnect(&WLayerSurface::exclusiveZoneChanged, this, &Output::layoutLayerSurfaces);
        connect(surface, &SurfaceWrapper::widthChanged, this, &Output::layoutLayerSurfaces);
        connect(surface, &SurfaceWrapper::heightChanged, this, &Output::layoutLayerSurfaces);

        layoutLayerSurfaces();
    } else {
        auto layoutSurface = [surface, this] {
            layoutNonLayerSurface(surface, {});
        };

        connect(surface, &SurfaceWrapper::widthChanged, this, layoutSurface);
        connect(surface, &SurfaceWrapper::heightChanged, this, layoutSurface);
        layoutSurface();
    }
}

void Output::removeSurface(SurfaceWrapper *surface)
{
    Q_ASSERT(hasSurface(surface));
    SurfaceListModel::removeSurface(surface);
    surface->disconnect(this);

    if (surface->type() == SurfaceWrapper::Type::Layer) {
        if (auto ss = surface->shellSurface())
            ss->safeDisconnect(this);

        layoutLayerSurfaces();
    }
}

WOutput *Output::output() const
{
    auto o = m_item->output();
    Q_ASSERT(o);
    return o;
}

WOutputItem *Output::outputItem() const
{
    return m_item;
}

void Output::addExclusiveZone(Qt::Edge edge, QObject *object, int value)
{
    removeExclusiveZone(object);

    switch (edge) {
    case Qt::TopEdge:
        m_topExclusiveZones.append(std::make_pair(object, value));
        m_exclusiveZone.setTop(m_exclusiveZone.top() + value);
        break;
    case Qt::BottomEdge:
        m_bottomExclusiveZones.append(std::make_pair(object, value));
        m_exclusiveZone.setBottom(m_exclusiveZone.bottom() + value);
        break;
    case Qt::LeftEdge:
        m_leftExclusiveZones.append(std::make_pair(object, value));
        m_exclusiveZone.setLeft(m_exclusiveZone.left() + value);
        break;
    case Qt::RightEdge:
        m_rightExclusiveZones.append(std::make_pair(object, value));
        m_exclusiveZone.setRight(m_exclusiveZone.right() + value);
        break;
    default:
        Q_UNREACHABLE_RETURN();
    }
}

bool Output::removeExclusiveZone(QObject *object)
{
    auto finder = [object](const auto &pair) { return pair.first == object; };
    auto tmp = std::find_if(m_topExclusiveZones.begin(), m_topExclusiveZones.end(), finder);
    if (tmp != m_topExclusiveZones.end()) {
        m_topExclusiveZones.erase(tmp);
        m_exclusiveZone.setTop(m_exclusiveZone.top() - tmp->second);
        Q_ASSERT(m_exclusiveZone.top() >= 0);
        return true;
    }

    tmp = std::find_if(m_bottomExclusiveZones.begin(), m_bottomExclusiveZones.end(), finder);
    if (tmp != m_bottomExclusiveZones.end()) {
        m_bottomExclusiveZones.erase(tmp);
        m_exclusiveZone.setBottom(m_exclusiveZone.bottom() - tmp->second);
        Q_ASSERT(m_exclusiveZone.bottom() >= 0);
        return true;
    }

    tmp = std::find_if(m_leftExclusiveZones.begin(), m_leftExclusiveZones.end(), finder);
    if (tmp != m_leftExclusiveZones.end()) {
        m_leftExclusiveZones.erase(tmp);
        m_exclusiveZone.setLeft(m_exclusiveZone.left() - tmp->second);
        Q_ASSERT(m_exclusiveZone.left() >= 0);
        return true;
    }

    tmp = std::find_if(m_rightExclusiveZones.begin(), m_rightExclusiveZones.end(), finder);
    if (tmp != m_rightExclusiveZones.end()) {
        m_rightExclusiveZones.erase(tmp);
        m_exclusiveZone.setRight(m_exclusiveZone.right() - tmp->second);
        Q_ASSERT(m_exclusiveZone.right() >= 0);
        return true;
    }

    return false;
}

void Output::layoutLayerSurface(SurfaceWrapper *surface)
{
    WLayerSurface* layer = qobject_cast<WLayerSurface*>(surface->shellSurface());
    Q_ASSERT(layer);

    auto validGeo = layer->exclusiveZone() == -1 ? this->rect() : validRect();
    validGeo = validGeo.marginsRemoved(QMargins(layer->leftMargin(),
                                                layer->topMargin(),
                                                layer->rightMargin(),
                                                layer->bottomMargin()));
    auto anchor = layer->ancher();

    if (anchor == WLayerSurface::AnchorType::None) {
        surface->resetWidth();
        surface->resetHeight();
        QRectF surfaceGeo = surface->normalGeometry();
        surfaceGeo.moveCenter(validGeo.center());
        surface->moveNormalGeometryInOutput(surfaceGeo.topLeft());
        return;
    }

    // update surface size
    if (anchor.testFlags(WLayerSurface::AnchorType::Left | WLayerSurface::AnchorType::Right)) {
        surface->setWidth(validGeo.width());
    } else {
        surface->resetWidth();
    }
    if (anchor.testFlags(WLayerSurface::AnchorType::Top | WLayerSurface::AnchorType::Bottom)) {
        surface->setHeight(validGeo.height());
    } else {
        surface->resetHeight();
    }

    // update surface position
    QRectF surfaceGeo(QPointF(0, 0), surface->size());
    if (anchor & WLayerSurface::AnchorType::Top) {
        surfaceGeo.moveTop(validGeo.top());
        if (!(anchor & WLayerSurface::AnchorType::Bottom)) {
            if (layer->exclusiveZone() > 0)
                addExclusiveZone(Qt::TopEdge, layer, layer->exclusiveZone());
        }
    } else if (anchor & WLayerSurface::AnchorType::Bottom) {
        surfaceGeo.moveBottom(validGeo.bottom());
        if (layer->exclusiveZone() > 0)
            addExclusiveZone(Qt::BottomEdge, layer, layer->exclusiveZone());
    }

    if (anchor & WLayerSurface::AnchorType::Left) {
        surfaceGeo.moveLeft(validGeo.left());
        if (!(anchor & WLayerSurface::AnchorType::Right)) {
            if (layer->exclusiveZone() > 0)
                addExclusiveZone(Qt::LeftEdge, layer, layer->exclusiveZone());
        }
    } else if (anchor & WLayerSurface::AnchorType::Right) {
        surfaceGeo.moveRight(validGeo.right());
        if (layer->exclusiveZone() > 0)
            addExclusiveZone(Qt::RightEdge, layer, layer->exclusiveZone());
    }

    surface->moveNormalGeometryInOutput(surfaceGeo.topLeft());
}

void Output::layoutLayerSurfaces()
{
    auto oldExclusiveZone = m_exclusiveZone;
    m_exclusiveZone = QMargins();
    m_topExclusiveZones.clear();
    m_bottomExclusiveZones.clear();
    m_leftExclusiveZones.clear();
    m_rightExclusiveZones.clear();

    for (auto *s : surfaces()) {
        if (s->type() != SurfaceWrapper::Type::Layer)
            continue;
        layoutLayerSurface(s);
    }

    if (oldExclusiveZone != m_exclusiveZone) {
        layoutNonLayerSurfaces();
        emit exclusiveZoneChanged();
    }
}

void Output::layoutNonLayerSurface(SurfaceWrapper *surface, const QSizeF &sizeDiff)
{
    Q_ASSERT(surface->type() != SurfaceWrapper::Type::Layer);
    surface->setFullscreenGeometry(geometry());
    const auto validGeo = this->validGeometry();
    surface->setMaximizedGeometry(validGeo);

    QRectF normalGeo = surface->normalGeometry();
    do {
        if (surface->positionAutomatic()) {
            if (normalGeo.size().isEmpty())
                return;

            normalGeo.moveCenter(validGeo.center());
            normalGeo.moveTop(qMax(normalGeo.top(), validGeo.top()));
            normalGeo.moveLeft(qMax(normalGeo.left(), validGeo.left()));
            surface->moveNormalGeometryInOutput(normalGeo.topLeft());
        } else if (!sizeDiff.isNull() && sizeDiff.isValid()) {
            const QSizeF outputSize = m_item->size();
            const auto xScale = outputSize.width() / (outputSize.width() - sizeDiff.width());
            const auto yScale = outputSize.height() / (outputSize.height() - sizeDiff.height());
            normalGeo.moveLeft(normalGeo.x() * xScale);
            normalGeo.moveTop(normalGeo.y() * yScale);
            surface->moveNormalGeometryInOutput(normalGeo.topLeft());
        } else {
            break;
        }
    } while (false);
}

void Output::layoutNonLayerSurfaces()
{
    const auto currentSize = validRect().size();
    const auto sizeDiff = m_lastSizeOnLayoutNonLayerSurfaces.isValid()
                              ? currentSize - m_lastSizeOnLayoutNonLayerSurfaces
                              : QSizeF(0, 0);
    m_lastSizeOnLayoutNonLayerSurfaces = currentSize;

    for (SurfaceWrapper *surface : surfaces()) {
        if (surface->type() == SurfaceWrapper::Type::Layer)
            continue;
        layoutNonLayerSurface(surface, sizeDiff);
    }
}

void Output::layoutAllSurfaces()
{
    layoutLayerSurfaces();
    layoutNonLayerSurfaces();
}

QMargins Output::exclusiveZone() const
{
    return m_exclusiveZone;
}

QRectF Output::rect() const
{
    return QRectF(QPointF(0, 0), m_item->size());
}

QRectF Output::geometry() const
{
    return QRectF(m_item->position(), m_item->size());
}

QRectF Output::validRect() const
{
    return rect().marginsRemoved(m_exclusiveZone);
}

QRectF Output::validGeometry() const
{
    return geometry().marginsRemoved(m_exclusiveZone);
}

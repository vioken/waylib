// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "surfacewrapper.h"
#include "qmlengine.h"
#include "output.h"

#include <woutput.h>
#include <wxdgsurfaceitem.h>
#include <wlayersurfaceitem.h>
#include <wxwaylandsurfaceitem.h>
#include <wxwaylandsurface.h>
#include <woutputitem.h>
#include <woutputrenderwindow.h>

SurfaceWrapper::SurfaceWrapper(QmlEngine *qmlEngine, WToplevelSurface *shellSurface, Type type, QQuickItem *parent)
    : QQuickItem(parent)
    , m_engine(qmlEngine)
    , m_shellSurface(shellSurface)
    , m_type(type)
{
    QQmlEngine::setContextForObject(this, qmlEngine->rootContext());

    if (type == Type::XWayland) {
        m_surfaceItem = new WXWaylandSurfaceItem(this);
    } else if (type == Type::Layer) {
        m_surfaceItem = new WLayerSurfaceItem(this);
    } else {
        m_surfaceItem = new WXdgSurfaceItem(this);
    }

    QQmlEngine::setContextForObject(m_surfaceItem, qmlEngine->rootContext());
    m_surfaceItem->setResizeMode(WSurfaceItem::ManualResize);
    m_surfaceItem->setShellSurface(shellSurface);

    connect(m_surfaceItem, &WSurfaceItem::implicitWidthChanged, this, [this] {
        setImplicitWidth(m_surfaceItem->implicitWidth());
        if (m_titleBar)
            m_titleBar->setWidth(m_surfaceItem->width());
    });
    connect(m_surfaceItem, &WSurfaceItem::heightChanged, this, &SurfaceWrapper::updateImplicitHeight);
    setImplicitSize(m_surfaceItem->implicitWidth(), m_surfaceItem->implicitHeight());

    if (shellSurface->doesNotAcceptFocus()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        m_surfaceItem->setFocusPolicy(Qt::NoFocus);
#endif
    }
}

SurfaceWrapper::~SurfaceWrapper()
{
    if (m_titleBar)
        delete m_titleBar;
    if (m_decoration)
        delete m_decoration;

    if (m_ownsOutput)
        m_ownsOutput->removeSurface(this);

    if (m_container)
        m_container->removeSurface(this);

    for (auto subS : std::as_const(m_subSurfaces)) {
        subS->m_parentSurface = nullptr;
    }

    if (m_parentSurface)
        m_parentSurface->removeSubSurface(this);
}

void SurfaceWrapper::setParent(QQuickItem *item)
{
    QObject::setParent(item);
    setParentItem(item);
}

void SurfaceWrapper::setFocus(bool focus, Qt::FocusReason reason)
{
    if (focus)
        m_surfaceItem->forceActiveFocus(reason);
    else
        m_surfaceItem->setFocus(false, reason);
}

WSurface *SurfaceWrapper::surface() const
{
    return m_shellSurface->surface();
}

WToplevelSurface *SurfaceWrapper::shellSurface() const
{
    return m_shellSurface;
}

WSurfaceItem *SurfaceWrapper::surfaceItem() const
{
    return m_surfaceItem;
}

bool SurfaceWrapper::resize(const QSizeF &size)
{
    QSizeF surfaceSize = m_titleBar ? QSizeF(size.width(), size.height() - m_titleBar->height()) : size;
    return m_surfaceItem->resizeSurface(surfaceSize);
}

QRectF SurfaceWrapper::titlebarGeometry() const
{
    return m_titleBar ? QRectF({0, 0}, m_titleBar->size()) : QRectF();
}

QRectF SurfaceWrapper::boundedRect() const
{
    return m_boundedRect;
}

QRectF SurfaceWrapper::normalGeometry() const
{
    return m_normalGeometry;
}

void SurfaceWrapper::moveNormalGeometryInOutput(const QPointF &position)
{
    if (isNormal())
        setPosition(position);
    setNormalGeometry(QRectF(position, m_normalGeometry.size()));
}

void SurfaceWrapper::resizeNormalGeometryInOutput(const QSizeF &size)
{
    Q_ASSERT(m_type == Type::Layer);
    if (size.isValid()) {
        setSize(size);
    }
}

void SurfaceWrapper::setNormalGeometry(const QRectF &newNormalGeometry)
{
    if (m_normalGeometry == newNormalGeometry)
        return;
    m_normalGeometry = newNormalGeometry;
    emit normalGeometryChanged();
}

QRectF SurfaceWrapper::maximizedGeometry() const
{
    return m_maximizedGeometry;
}

void SurfaceWrapper::setMaximizedGeometry(const QRectF &newMaximizedGeometry)
{
    if (m_maximizedGeometry == newMaximizedGeometry)
        return;
    m_maximizedGeometry = newMaximizedGeometry;
    if (m_surfaceState == State::Maximized) {
        setPosition(newMaximizedGeometry.topLeft());
        setSize(newMaximizedGeometry.size());
    }

    emit maximizedGeometryChanged();
}

QRectF SurfaceWrapper::fullscreenGeometry() const
{
    return m_fullscreenGeometry;
}

void SurfaceWrapper::setFullscreenGeometry(const QRectF &newFullscreenGeometry)
{
    if (m_fullscreenGeometry == newFullscreenGeometry)
        return;
    m_fullscreenGeometry = newFullscreenGeometry;
    if (m_surfaceState == State::Fullscreen) {
        setPosition(newFullscreenGeometry.topLeft());
        setSize(newFullscreenGeometry.size());
    }

    emit fullscreenGeometryChanged();

    updateClipRect();
}

QRectF SurfaceWrapper::tilingGeometry() const
{
    return m_tilingGeometry;
}

void SurfaceWrapper::setTilingGeometry(const QRectF &newTilingGeometry)
{
    if (m_tilingGeometry == newTilingGeometry)
        return;
    m_tilingGeometry = newTilingGeometry;
    if (m_surfaceState == State::Tiling) {
        setPosition(newTilingGeometry.topLeft());
        setSize(newTilingGeometry.size());
    }

    emit tilingGeometryChanged();
}

bool SurfaceWrapper::positionAutomatic() const
{
    return m_positionAutomatic;
}

void SurfaceWrapper::setPositionAutomatic(bool newPositionAutomatic)
{
    if (m_positionAutomatic == newPositionAutomatic)
        return;
    m_positionAutomatic = newPositionAutomatic;
    emit positionAutomaticChanged();
}

void SurfaceWrapper::resetWidth()
{
    m_surfaceItem->resetWidth();
    QQuickItem::resetWidth();
}

void SurfaceWrapper::resetHeight()
{
    m_surfaceItem->resetHeight();
    QQuickItem::resetHeight();
}

SurfaceWrapper::Type SurfaceWrapper::type() const
{
    return m_type;
}

Output *SurfaceWrapper::ownsOutput() const
{
    return m_ownsOutput;
}

void SurfaceWrapper::setOwnsOutput(Output *newOwnsOutput)
{
    if (m_ownsOutput == newOwnsOutput)
        return;

    if (m_ownsOutput) {
        m_ownsOutput->removeSurface(this);
    }

    m_ownsOutput = newOwnsOutput;

    if (m_ownsOutput) {
        m_ownsOutput->addSurface(this);
    }

    emit ownsOutputChanged();
}

void SurfaceWrapper::setOutputs(const QList<WOutput*> &outputs)
{
    auto oldOutputs = surface()->outputs();
    for (auto output : oldOutputs) {
        if (outputs.contains(output)) {
            continue;
        }
        surface()->leaveOutput(output);
    }

    for (auto output : outputs) {
        if (oldOutputs.contains(output))
            continue;
        surface()->enterOutput(output);
    }
}

QRectF SurfaceWrapper::geometry() const
{
    return QRectF(position(), size());
}

SurfaceWrapper::State SurfaceWrapper::previousSurfaceState() const
{
    return m_previousSurfaceState;
}

SurfaceWrapper::State SurfaceWrapper::surfaceState() const
{
    return m_surfaceState;
}

void SurfaceWrapper::setSurfaceState(State newSurfaceState)
{
    if (m_surfaceState == newSurfaceState
        || m_pendingSurfaceState == newSurfaceState) {
        return;
    }
    m_pendingSurfaceState.setValueBypassingBindings(newSurfaceState);
    updateVisible();
    setVisibleDecoration(newSurfaceState == State::Minimized
                         || newSurfaceState == State::Normal);

    if (newSurfaceState == State::Maximized) {
        setPosition(m_maximizedGeometry.topLeft());
        setSize(m_maximizedGeometry.size());
    } else if (newSurfaceState == State::Fullscreen) {
        setPosition(m_fullscreenGeometry.topLeft());
        setSize(m_fullscreenGeometry.size());
    } else if (newSurfaceState == State::Normal) {
        setPosition(m_normalGeometry.topLeft());
        setSize(m_normalGeometry.size());
    } else if (newSurfaceState == State::Tiling) {
        setPosition(m_tilingGeometry.topLeft());
        setSize(m_tilingGeometry.size());
    }

    if (m_previousSurfaceState != newSurfaceState) {
        m_previousSurfaceState.setValueBypassingBindings(m_surfaceState);
    }

    m_surfaceState.setValueBypassingBindings(newSurfaceState);
    m_previousSurfaceState.notify();
    m_surfaceState.notify();

    updateVisible();
}

QBindable<SurfaceWrapper::State> SurfaceWrapper::bindableSurfaceState()
{
    return &m_surfaceState;
}

bool SurfaceWrapper::isNormal() const
{
    return m_surfaceState == State::Normal
           && m_pendingSurfaceState == State::Normal;
}

bool SurfaceWrapper::isMaximized() const
{
    return m_surfaceState == State::Maximized
           && m_pendingSurfaceState == State::Maximized;
}

bool SurfaceWrapper::isMinimized() const
{
    return m_surfaceState == State::Minimized
           && m_pendingSurfaceState == State::Minimized;
}

bool SurfaceWrapper::isTiling() const
{
    return m_surfaceState == State::Tiling
           && m_pendingSurfaceState == State::Tiling;
}

bool SurfaceWrapper::noDecoration() const
{
    return m_noDecoration;
}

void SurfaceWrapper::setNoDecoration(bool newNoDecoration)
{
    if (m_noDecoration == newNoDecoration)
        return;
    m_noDecoration = newNoDecoration;

    if (newNoDecoration) {
        Q_ASSERT(m_titleBar);
        m_titleBar->deleteLater();
        m_titleBar = nullptr;
        Q_ASSERT(m_decoration);
        m_decoration->deleteLater();
        m_decoration = nullptr;
        m_surfaceItem->setY(0);
        Q_EMIT boundedRectChanged();
    } else {
        Q_ASSERT(!m_titleBar);
        m_titleBar = m_engine->createTitleBar(this, m_surfaceItem);
        m_titleBar->setZ(static_cast<int>(WSurfaceItem::ZOrder::ContentItem));
        m_titleBar->setWidth(m_surfaceItem->width());
        m_surfaceItem->setY(m_titleBar->height());
        m_titleBar->setY(-m_titleBar->height());
        connect(m_titleBar, &QQuickItem::heightChanged, this, [this] {
            m_surfaceItem->setY(m_titleBar->height());
            m_titleBar->setY(-m_titleBar->height());
            updateImplicitHeight();
        });

        Q_ASSERT(!m_decoration);
        m_decoration = m_engine->createDecoration(this, this);
        m_decoration->stackBefore(m_surfaceItem);
        connect(m_decoration, &QQuickItem::xChanged, this, &SurfaceWrapper::updateBoundedRect);
        connect(m_decoration, &QQuickItem::yChanged, this, &SurfaceWrapper::updateBoundedRect);
        connect(m_decoration, &QQuickItem::widthChanged, this, &SurfaceWrapper::updateBoundedRect);
        connect(m_decoration, &QQuickItem::heightChanged, this, &SurfaceWrapper::updateBoundedRect);
        updateBoundedRect();
    }

    updateImplicitHeight();
    emit noDecorationChanged();
}

void SurfaceWrapper::setBoundedRect(const QRectF &newBoundedRect)
{
    if (m_boundedRect == newBoundedRect)
        return;
    m_boundedRect = newBoundedRect;
    emit boundedRectChanged();
}

void SurfaceWrapper::updateBoundedRect()
{
    const QRectF rect(QRectF(QPointF(0, 0), size()));
    if (!m_decoration) {
        setBoundedRect(rect);
        return;
    }

    const QRectF dr(m_decoration->position(), m_decoration->size());
    setBoundedRect(dr | rect);
}

void SurfaceWrapper::updateVisible()
{
    setVisible(!isMinimized() && surface()->mapped());
}

void SurfaceWrapper::updateImplicitHeight()
{
    if (m_titleBar) {
        setImplicitHeight(m_surfaceItem->implicitHeight() + m_titleBar->height());
    } else {
        setImplicitHeight(m_surfaceItem->implicitHeight());
    }
}

void SurfaceWrapper::updateSubSurfaceStacking()
{
    SurfaceWrapper *lastSurface = this;
    for (auto surface : std::as_const(m_subSurfaces)) {
        surface->stackAfter(lastSurface);
        lastSurface = surface->stackLastSurface();
    }
}

void SurfaceWrapper::updateClipRect()
{
    if (!clip() || !window())
        return;
    auto rw = qobject_cast<WOutputRenderWindow*>(window());
    Q_ASSERT(rw);
    rw->markItemClipRectDirty(this);
}

void SurfaceWrapper::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (m_container && m_container->filterSurfaceGeometryChanged(this, newGeometry, oldGeometry))
        return;

    if (isNormal()) {
        setNormalGeometry(newGeometry);
    }

    const qreal contentHeight = m_titleBar ? newGeometry.height() - m_titleBar->height() : newGeometry.height();
    if (widthValid() && heightValid()) {
        m_surfaceItem->resizeSurface({newGeometry.width(), contentHeight});
    } else if (widthValid()) {
        m_surfaceItem->resizeSurface({newGeometry.width(), m_surfaceItem->implicitHeight()});
    } else if (heightValid()) {
        m_surfaceItem->resizeSurface({m_surfaceItem->implicitWidth(), contentHeight});
    }

    Q_EMIT geometryChanged();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    updateBoundedRect();
    updateClipRect();
}

qreal SurfaceWrapper::radius() const
{
    return m_radius;
}

void SurfaceWrapper::setRadius(qreal newRadius)
{
    if (qFuzzyCompare(m_radius, newRadius))
        return;
    m_radius = newRadius;
    emit radiusChanged();
}

void SurfaceWrapper::requestMinimize()
{
    setSurfaceState(State::Minimized);
}

void SurfaceWrapper::requestUnminimize()
{
    if (m_surfaceState == State::Minimized)
        setSurfaceState(m_previousSurfaceState);
}

void SurfaceWrapper::requestToggleMaximize()
{
    if (m_surfaceState == State::Maximized)
        setSurfaceState(State::Normal);
    else
        setSurfaceState(State::Maximized);
}

void SurfaceWrapper::requestClose()
{
    m_shellSurface->close();
    updateVisible();
}

SurfaceWrapper *SurfaceWrapper::stackFirstSurface() const
{
    return m_subSurfaces.isEmpty() ? const_cast<SurfaceWrapper*>(this) : m_subSurfaces.first()->stackFirstSurface();
}

SurfaceWrapper *SurfaceWrapper::stackLastSurface() const
{
    return m_subSurfaces.isEmpty() ? const_cast<SurfaceWrapper*>(this) : m_subSurfaces.last()->stackLastSurface();
}

bool SurfaceWrapper::hasChild(SurfaceWrapper *child) const
{
    for (auto s : std::as_const(m_subSurfaces)) {
        if (s == child || s->hasChild(child))
            return true;
    }

    return false;
}

bool SurfaceWrapper::stackBefore(QQuickItem *item)
{
    if (!parentItem() || item->parentItem() != parentItem())
        return false;
    if (this == item)
        return false;

    do {
        auto s = qobject_cast<SurfaceWrapper*>(item);
        if (s) {
            if (hasChild(s) || s->hasChild(this))
                return false;
            item = s->stackFirstSurface();

            if (m_parentSurface && m_parentSurface == s->m_parentSurface) {
                QQuickItem::stackBefore(item);
                m_parentSurface->m_subSurfaces.removeOne(this);
                m_parentSurface->m_subSurfaces.prepend(this);
                break;
            }
        }

        if (m_parentSurface) {
            if (!m_parentSurface->stackBefore(item))
                return false;
        } else {
            QQuickItem::stackBefore(item);
        }
    } while (false);

    updateSubSurfaceStacking();
    return true;
}

bool SurfaceWrapper::stackAfter(QQuickItem *item)
{
    if (!parentItem() || item->parentItem() != parentItem())
        return false;
    if (this == item)
        return false;

    do {
        auto s = qobject_cast<SurfaceWrapper*>(item);
        if (s) {
            if (hasChild(s) || s->hasChild(this))
                return false;
            item = s->stackLastSurface();

            if (m_parentSurface && m_parentSurface == s->m_parentSurface) {
                QQuickItem::stackAfter(item);
                m_parentSurface->m_subSurfaces.removeOne(this);
                m_parentSurface->m_subSurfaces.append(this);
                break;
            }
        }

        if (m_parentSurface) {
            if (!m_parentSurface->stackAfter(item))
                return false;
        } else {
            QQuickItem::stackAfter(item);
        }
    } while (false);

    updateSubSurfaceStacking();
    return true;
}

void SurfaceWrapper::stackToLast()
{
    if (!parentItem())
        return;

    if (m_parentSurface) {
        m_parentSurface->stackToLast();
        stackAfter(m_parentSurface->stackLastSurface());
    } else {
        auto last = parentItem()->childItems().last();
        stackAfter(last);
    }
}

void SurfaceWrapper::addSubSurface(SurfaceWrapper *surface)
{
    Q_ASSERT(!surface->m_parentSurface);
    surface->m_parentSurface = this;
    m_subSurfaces.append(surface);
}

void SurfaceWrapper::removeSubSurface(SurfaceWrapper *surface)
{
    Q_ASSERT(surface->m_parentSurface == this);
    surface->m_parentSurface = nullptr;
    m_subSurfaces.removeOne(surface);
}

const QList<SurfaceWrapper *> &SurfaceWrapper::subSurfaces() const
{
    return m_subSurfaces;
}

SurfaceContainer *SurfaceWrapper::container() const
{
    return m_container;
}

void SurfaceWrapper::setContainer(SurfaceContainer *newContainer)
{
    if (m_container == newContainer)
        return;
    m_container = newContainer;
    emit containerChanged();
}

QQuickItem *SurfaceWrapper::titleBar() const
{
    return m_titleBar;
}

QQuickItem *SurfaceWrapper::decoration() const
{
    return m_decoration;
}

bool SurfaceWrapper::visibleDecoration() const
{
    return m_visibleDecoration;
}

void SurfaceWrapper::setVisibleDecoration(bool newVisibleDecoration)
{
    if (m_visibleDecoration == newVisibleDecoration)
        return;
    m_visibleDecoration = newVisibleDecoration;
    emit visibleDecorationChanged();
}

bool SurfaceWrapper::clipInOutput() const
{
    return m_clipInOutput;
}

void SurfaceWrapper::setClipInOutput(bool newClipInOutput)
{
    if (m_clipInOutput == newClipInOutput)
        return;
    m_clipInOutput = newClipInOutput;
    updateClipRect();
    emit clipInOutputChanged();
}

QRectF SurfaceWrapper::clipRect() const
{
    if (m_clipInOutput) {
        return m_fullscreenGeometry & geometry();
    }

    return QQuickItem::clipRect();
}

// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "rootsurfacecontainer.h"
#include "helper.h"
#include "surfacewrapper.h"
#include "output.h"

#include <woutputlayout.h>
#include <wcursor.h>
#include <woutputitem.h>
#include <woutput.h>
#include <wxdgsurface.h>

#include <QQuickWindow>

WAYLIB_SERVER_USE_NAMESPACE

RootSurfaceContainer::RootSurfaceContainer(QQuickItem *parent)
    : SurfaceContainer(parent)
    , m_outputLayout(new WOutputLayout(this))
    , m_cursor(new WCursor(this))
{
    m_cursor->setLayout(m_outputLayout);
    m_cursor->setEventWindow(window());

    connect(m_outputLayout, &WOutputLayout::implicitWidthChanged, this, [this] {
        const auto width = m_outputLayout->implicitWidth();
        window()->setWidth(width);
        setWidth(width);
    });

    connect(m_outputLayout, &WOutputLayout::implicitHeightChanged, this, [this] {
        const auto height = m_outputLayout->implicitHeight();
        window()->setHeight(height);
        setHeight(height);
    });

    connect(m_outputLayout, &WOutputLayout::notify_change, this, [this] {
        ensureCursorVisible();

        // for (auto s : m_surfaceContainer->surfaces()) {
        //     ensureSurfaceNormalPositionValid(s);
        //     updateSurfaceOutputs(s);
        // }
    });
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

void RootSurfaceContainer::destroyForSurface(WSurface *surface)
{
    auto wrapper = getSurface(surface);
    if (wrapper == moveResizeState.surface)
        endMoveResize();

    delete wrapper;
}

void RootSurfaceContainer::addOutput(Output *output)
{
    m_outputList << output;
    m_outputLayout->autoAdd(output->output());
    if (!m_primaryOutput)
        setPrimaryOutput(output);

    SurfaceContainer::addOutput(output);
}

void RootSurfaceContainer::removeOutput(Output *output)
{
    m_outputList.removeOne(output);
    SurfaceContainer::removeOutput(output);

    if (moveResizeState.surface && moveResizeState.surface->ownsOutput() == output) {
        endMoveResize();
    }

    if (m_primaryOutput == output) {
        const auto outputs = m_outputLayout->outputs();
        if (!outputs.isEmpty()) {
            auto newPrimaryOutput = Helper::instance()->getOutput(outputs.first());
            setPrimaryOutput(newPrimaryOutput);
        }
    }

    // ensure cursor within output
    const auto outputPos = output->outputItem()->position();
    if (output->geometry().contains(m_cursor->position()) && m_primaryOutput) {
        const auto posInOutput = m_cursor->position() - outputPos;
        const auto newCursorPos = m_primaryOutput->outputItem()->position() + posInOutput;

        if (m_primaryOutput->geometry().contains(newCursorPos))
            Helper::instance()->setCursorPosition(newCursorPos);
        else
            Helper::instance()->setCursorPosition(m_primaryOutput->geometry().center());
    }

    m_outputLayout->remove(output->output());
}

void RootSurfaceContainer::beginMoveResize(SurfaceWrapper *surface, Qt::Edges edges)
{
    Q_ASSERT(!moveResizeState.surface);
    moveResizeState.surface = surface;
    moveResizeState.startGeometry = surface->geometry();
    moveResizeState.resizeEdges = edges;
    surface->setPositionAutomatic(false);
}

void RootSurfaceContainer::doMoveResize(const QPointF &incrementPos)
{
    Q_ASSERT(moveResizeState.surface);

    if (moveResizeState.resizeEdges) {
        QRectF geo = moveResizeState.startGeometry;

        if (moveResizeState.resizeEdges & Qt::LeftEdge)
            geo.setLeft(geo.left() + incrementPos.x());
        if (moveResizeState.resizeEdges & Qt::TopEdge)
            geo.setTop(geo.top() + incrementPos.y());

        if (moveResizeState.resizeEdges & Qt::RightEdge)
            geo.setRight(geo.right() + incrementPos.x());
        if (moveResizeState.resizeEdges & Qt::BottomEdge)
            geo.setBottom(geo.bottom() + incrementPos.y());

        moveResizeState.surface->resize(geo.size());
    } else {
        auto new_pos = moveResizeState.startGeometry.topLeft() + incrementPos;
        moveResizeState.surface->setPosition(new_pos);
    }
}

void RootSurfaceContainer::cancelMoveResize(SurfaceWrapper *surface)
{
    if (moveResizeState.surface != surface)
        return;
    endMoveResize();
}

void RootSurfaceContainer::endMoveResize()
{
    if (!moveResizeState.surface)
        return;

    auto o = moveResizeState.surface->ownsOutput();
    moveResizeState.surface->shellSurface()->setResizeing(false);

    if (!o || !moveResizeState.surface->surface()->outputs().contains(o->output())) {
        o = cursorOutput();
        Q_ASSERT(o);
        moveResizeState.surface->setOwnsOutput(o);
    }

    ensureSurfaceNormalPositionValid(moveResizeState.surface);

    moveResizeState.surface = nullptr;
    Q_EMIT moveResizeFinised();
}

SurfaceWrapper *RootSurfaceContainer::moveResizeSurface() const
{
    return moveResizeState.surface;
}

void RootSurfaceContainer::startMove(SurfaceWrapper *surface)
{
    endMoveResize();
    beginMoveResize(surface, Qt::Edges{0});

    Helper::instance()->activeSurface(surface);
}

void RootSurfaceContainer::startResize(SurfaceWrapper *surface, Qt::Edges edges)
{
    endMoveResize();
    Q_ASSERT(edges != 0);

    beginMoveResize(surface, edges);
    surface->shellSurface()->setResizeing(true);
    Helper::instance()->activeSurface(surface);
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
        startMove(surface);
    });
    connect(surface, &SurfaceWrapper::requestResize, this, [this] (Qt::Edges edges) {
        auto surface = qobject_cast<SurfaceWrapper*>(sender());
        Q_ASSERT(surface);
        startResize(surface, edges);
    });
    connect(surface, &SurfaceWrapper::surfaceStateChanged, this, [surface, this] {
        if (surface->surfaceState() == SurfaceWrapper::State::Minimized
            || surface->surfaceState() == SurfaceWrapper::State::Tiling)
            return;
        Helper::instance()->activeSurface(surface);
    });

    if (!surface->ownsOutput()) {
        auto parentSurface = surface->parentSurface();
        auto output = parentSurface ? parentSurface->ownsOutput() :
            Helper::instance()->rootContainer()->primaryOutput();

        if (auto xdgSurface = qobject_cast<WXdgSurface*>(surface->shellSurface())) {
            if (xdgSurface->isPopup() && parentSurface->type() != SurfaceWrapper::Type::Layer) {
                auto pos = parentSurface->position() + parentSurface->surfaceItem()->position() + xdgSurface->getPopupPosition();
                if (auto op = m_outputLayout->output_at(pos.x(), pos.y()))
                    output = Helper::instance()->getOutput(WOutput::fromHandle(qw_output::from(op)));
            }
        }
        surface->setOwnsOutput(output);
    }

    connect(surface, &SurfaceWrapper::geometryChanged, this, [this, surface] {
        updateSurfaceOutputs(surface);
    });

    updateSurfaceOutputs(surface);
    Helper::instance()->activeSurface(surface, Qt::OtherFocusReason);
}

void RootSurfaceContainer::removeBySubContainer(SurfaceContainer *sub, SurfaceWrapper *surface)
{
    if (moveResizeState.surface == surface)
        endMoveResize();

    SurfaceContainer::removeBySubContainer(sub, surface);
}

bool RootSurfaceContainer::filterSurfaceGeometryChanged(SurfaceWrapper *surface, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (surface != moveResizeState.surface)
        return false;

    if (moveResizeState.resizeEdges != 0
        && !moveResizeState.setSurfacePositionForAnchorEdgets) {
        QRectF geometry = newGeometry;
        if (moveResizeState.resizeEdges & Qt::RightEdge)
            geometry.moveLeft(oldGeometry.left());
        if (moveResizeState.resizeEdges & Qt::BottomEdge)
            geometry.moveTop(oldGeometry.top());
        if (moveResizeState.resizeEdges & Qt::LeftEdge)
            geometry.moveRight(oldGeometry.right());
        if (moveResizeState.resizeEdges & Qt::TopEdge)
            geometry.moveBottom(oldGeometry.bottom());

        if (geometry.topLeft() != newGeometry.topLeft()) {
            moveResizeState.setSurfacePositionForAnchorEdgets = true;
            surface->setPosition(geometry.topLeft());
            moveResizeState.setSurfacePositionForAnchorEdgets = false;
            return true;
        }
    }

    return false;
}

WOutputLayout *RootSurfaceContainer::outputLayout() const
{
    return m_outputLayout;
}

WCursor *RootSurfaceContainer::cursor() const
{
    return m_cursor;
}

Output *RootSurfaceContainer::cursorOutput() const
{
    Q_ASSERT(m_cursor->layout() == m_outputLayout);
    const auto &pos = m_cursor->position();
    auto o = m_outputLayout->output_at(pos.x(), pos.y());
    if (!o)
        return nullptr;

    return Helper::instance()->getOutput(WOutput::fromHandle(qw_output::from(o)));
}

Output *RootSurfaceContainer::primaryOutput() const
{
    return m_primaryOutput;
}

void RootSurfaceContainer::setPrimaryOutput(Output *newPrimaryOutput)
{
    if (m_primaryOutput == newPrimaryOutput)
        return;
    m_primaryOutput = newPrimaryOutput;
    emit primaryOutputChanged();
}

void RootSurfaceContainer::ensureCursorVisible()
{
    const auto cursorPos = m_cursor->position();
    if (m_outputLayout->output_at(cursorPos.x(), cursorPos.y()))
        return;

    if (m_primaryOutput) {
        Helper::instance()->setCursorPosition(m_primaryOutput->geometry().center());
    }
}

void RootSurfaceContainer::updateSurfaceOutputs(SurfaceWrapper *surface)
{
    const QRectF geometry = surface->geometry();
    auto outputs = m_outputLayout->getIntersectedOutputs(geometry.toRect());
    surface->setOutputs(outputs);
}

static qreal pointToRectMinDistance(const QPointF &pos, const QRectF &rect) {
    if (rect.contains(pos))
        return 0;
    return std::min({std::abs(rect.x() - pos.x()), std::abs(rect.y() - pos.y()),
                     std::abs(rect.right() - pos.x()), std::abs(rect.bottom() - pos.y())});
}

static QRectF adjustRectToMakePointVisible(const QRectF& inputRect, const QPointF& absolutePoint, const QList<QRectF>& visibleAreas)
{
    Q_ASSERT(inputRect.contains(absolutePoint));
    QRectF adjustedRect = inputRect;

    QRectF targetRect;
    qreal distanceToTargetRect = std::numeric_limits<qreal>::max();
    for (const QRectF& area : visibleAreas) {
        Q_ASSERT(!area.isEmpty());
        if (area.contains(absolutePoint))
            return adjustedRect;
        const auto distance = pointToRectMinDistance(absolutePoint, area);
        if (distance < distanceToTargetRect) {
            distanceToTargetRect = distance;
            targetRect = area;
        }
    }
    Q_ASSERT(!targetRect.isEmpty());

    if (absolutePoint.x() < targetRect.x())
        adjustedRect.moveLeft(adjustedRect.x() + targetRect.x() - absolutePoint.x());
    else if (absolutePoint.x() > targetRect.right())
        adjustedRect.moveRight(adjustedRect.right() + targetRect.right() - absolutePoint.x());

    if (absolutePoint.y() < targetRect.y())
        adjustedRect.moveTop(adjustedRect.y() + targetRect.y() - absolutePoint.y());
    else if (absolutePoint.y() > targetRect.bottom())
        adjustedRect.moveBottom(adjustedRect.bottom() + targetRect.bottom() - absolutePoint.y());

    return adjustedRect;
}

void RootSurfaceContainer::ensureSurfaceNormalPositionValid(SurfaceWrapper *surface)
{
    if (surface->type() == SurfaceWrapper::Type::Layer)
        return;

    auto normalGeo = surface->normalGeometry();
    if (normalGeo.size().isEmpty())
        return;

    auto output = surface->ownsOutput();
    if (!output)
        return;

    QList<QRectF> outputRects;
    outputRects.reserve(m_outputList.size());
    for (auto o : std::as_const(m_outputList))
        outputRects << o->validGeometry();

    // Ensure window is not outside the screen
    const QPointF mustVisiblePosOfSurface(qMin(normalGeo.right(), normalGeo.x() + 20),
                                          qMin(normalGeo.bottom(), normalGeo.y() + 20));
    normalGeo = adjustRectToMakePointVisible(normalGeo, mustVisiblePosOfSurface, outputRects);

    // Ensure titlebar is not outside the screen
    const auto titlebarGeometry = surface->titlebarGeometry().translated(normalGeo.topLeft());
    if (titlebarGeometry.isValid()) {
        bool titlebarGeometryAdjusted = false;
        for (auto r : std::as_const(outputRects)) {
            if ((r & titlebarGeometry).isEmpty())
                continue;
            if (titlebarGeometry.top() < r.top()) {
                normalGeo.moveTop(normalGeo.top() + r.top() - titlebarGeometry.top());
                titlebarGeometryAdjusted = true;
            }
        }

        if (!titlebarGeometryAdjusted) {
            normalGeo = adjustRectToMakePointVisible(normalGeo, titlebarGeometry.topLeft(), outputRects);
        }
    }

    surface->moveNormalGeometryInOutput(normalGeo.topLeft());
}
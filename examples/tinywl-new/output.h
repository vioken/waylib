// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include <wglobal.h>
#include <QMargins>
#include <QObject>
#include <QQmlComponent>

WAYLIB_SERVER_BEGIN_NAMESPACE
class WOutput;
class WOutputItem;
WAYLIB_SERVER_END_NAMESPACE

WAYLIB_SERVER_USE_NAMESPACE

class SurfaceWrapper;
class Output : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QMargins exclusiveZone READ exclusiveZone NOTIFY exclusiveZoneChanged FINAL)
    Q_PROPERTY(QRectF validRect READ validRect NOTIFY exclusiveZoneChanged FINAL)

public:
    enum class Type {
        Primary,
        Proxy
    };

    static Output *createPrimary(WOutput *output, QQmlEngine *engine, QObject *parent = nullptr);
    static Output *createCopy(WOutput *output, Output *proxy, QQmlEngine *engine, QObject *parent = nullptr);

    explicit Output(WOutputItem *output, QObject *parent = nullptr);
    ~Output();

    bool isPrimary() const;

    void addSurface(SurfaceWrapper *surface);
    void removeSurface(SurfaceWrapper *surface);
    const QList<SurfaceWrapper *> &surfaceList() const;

    void moveSurface(SurfaceWrapper *surface, const QPointF &startPos, const QPointF &incrementPos);
    void resizeSurface(SurfaceWrapper *surface, const QRectF &startGeo, Qt::Edges edges, const QPointF &incrementPos);

    WOutput *output() const;
    WOutputItem *outputItem() const;

    QMargins exclusiveZone() const;
    QRectF rect() const;
    QRectF geometry() const;
    QRectF validRect() const;
    QRectF validGeometry() const;

signals:
    void exclusiveZoneChanged();

private:
    void addExclusiveZone(Qt::Edge edge, QObject *object, int value);
    bool removeExclusiveZone(QObject *object);
    void layoutLayerSurface(SurfaceWrapper *surface);
    void layoutLayerSurfaces();
    void layoutNonLayerSurface(SurfaceWrapper *surface, const QSizeF &sizeDiff);
    void layoutNonLayerSurfaces();
    void layoutAllSurfaces();

    Type m_type;
    WOutputItem *m_item;
    Output *m_proxy = nullptr;

    QList<SurfaceWrapper*> m_surfaces;

    QMargins m_exclusiveZone;
    QList<std::pair<QObject*, int>> m_topExclusiveZones;
    QList<std::pair<QObject*, int>> m_bottomExclusiveZones;
    QList<std::pair<QObject*, int>> m_leftExclusiveZones;
    QList<std::pair<QObject*, int>> m_rightExclusiveZones;

    QSizeF m_lastSizeOnLayoutNonLayerSurfaces;
};

// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include <wsurfaceitem.h>
#include <wtoplevelsurface.h>

#include <QQuickItem>

Q_MOC_INCLUDE(<woutput.h>)

WAYLIB_SERVER_USE_NAMESPACE

class QmlEngine;
class Output;
class SurfaceContainer;
class SurfaceWrapper : public QQuickItem
{
    friend class Helper;
    friend class SurfaceContainer;
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("SurfaceWrapper objects are created by c++")
    Q_PROPERTY(Type type READ type CONSTANT)
    // make to readonly
    Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged FINAL)
    Q_PROPERTY(WAYLIB_SERVER_NAMESPACE::WSurface* surface READ surface CONSTANT)
    Q_PROPERTY(WAYLIB_SERVER_NAMESPACE::WToplevelSurface* shellSurface READ shellSurface CONSTANT)
    Q_PROPERTY(WAYLIB_SERVER_NAMESPACE::WSurfaceItem* surfaceItem READ surfaceItem CONSTANT)
    Q_PROPERTY(QRectF boundedRect READ boundedRect NOTIFY boundedRectChanged)
    Q_PROPERTY(QRectF normalGeometry READ normalGeometry NOTIFY normalGeometryChanged FINAL)
    Q_PROPERTY(QRectF maximizedGeometry READ maximizedGeometry NOTIFY maximizedGeometryChanged FINAL)
    Q_PROPERTY(QRectF fullscreenGeometry READ fullscreenGeometry NOTIFY fullscreenGeometryChanged FINAL)
    Q_PROPERTY(QRectF tilingGeometry READ tilingGeometry NOTIFY tilingGeometryChanged FINAL)
    Q_PROPERTY(Output* ownsOutput READ ownsOutput NOTIFY ownsOutputChanged FINAL)
    Q_PROPERTY(bool positionAutomatic READ positionAutomatic WRITE setPositionAutomatic NOTIFY positionAutomaticChanged FINAL)
    Q_PROPERTY(State previousSurfaceState READ previousSurfaceState NOTIFY previousSurfaceStateChanged FINAL)
    Q_PROPERTY(State surfaceState READ surfaceState NOTIFY surfaceStateChanged BINDABLE bindableSurfaceState FINAL)
    Q_PROPERTY(bool noDecoration READ noDecoration NOTIFY noDecorationChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(SurfaceContainer* container READ container NOTIFY containerChanged FINAL)
    Q_PROPERTY(QQuickItem* titleBar READ titleBar NOTIFY noDecorationChanged FINAL)
    Q_PROPERTY(QQuickItem* decoration READ decoration NOTIFY noDecorationChanged FINAL)
    Q_PROPERTY(bool visibleDecoration READ visibleDecoration NOTIFY visibleDecorationChanged FINAL)

public:
    enum class Type {
        XdgToplevel,
        XdgPopup,
        XWayland,
        Layer,
    };
    Q_ENUM(Type)

    enum class State {
        Normal,
        Maximized,
        Minimized,
        Fullscreen,
        Tiling,
    };
    Q_ENUM(State)

    explicit SurfaceWrapper(QmlEngine *qmlEngine,
                            WToplevelSurface *shellSurface, Type type,
                            QQuickItem *parent = nullptr);
    ~SurfaceWrapper();

    void setFocus(bool focus, Qt::FocusReason reason);

    WSurface *surface() const;
    WToplevelSurface *shellSurface() const;
    WSurfaceItem *surfaceItem() const;
    bool resize(const QSizeF &size);

    QRectF titlebarGeometry() const;
    QRectF boundedRect() const;

    Type type() const;

    Output *ownsOutput() const;
    void setOwnsOutput(Output *newOwnsOutput);
    void setOutputs(const QList<WOutput*> &outputs);

    QRectF geometry() const;
    QRectF normalGeometry() const;
    void moveNormalGeometryInOutput(const QPointF &position);
    void resizeNormalGeometryInOutput(const QSizeF &size);

    QRectF maximizedGeometry() const;
    void setMaximizedGeometry(const QRectF &newMaximizedGeometry);

    QRectF fullscreenGeometry() const;
    void setFullscreenGeometry(const QRectF &newFullscreenGeometry);

    QRectF tilingGeometry() const;
    void setTilingGeometry(const QRectF &newTilingGeometry);

    bool positionAutomatic() const;
    void setPositionAutomatic(bool newPositionAutomatic);

    void resetWidth();
    void resetHeight();

    State previousSurfaceState() const;
    State surfaceState() const;
    void setSurfaceState(State newSurfaceState);
    QBindable<State> bindableSurfaceState();
    bool isNormal() const;
    bool isMaximized() const;
    bool isMinimized() const;
    bool isTiling() const;

    bool noDecoration() const;

    qreal radius() const;
    void setRadius(qreal newRadius);

    SurfaceContainer *container() const;

    void addSubSurface(SurfaceWrapper *surface);
    void removeSubSurface(SurfaceWrapper *surface);
    const QList<SurfaceWrapper*> &subSurfaces() const;
    SurfaceWrapper *stackFirstSurface() const;
    SurfaceWrapper *stackLastSurface() const;
    bool hasChild(SurfaceWrapper *child) const;

    QQuickItem *titleBar() const;
    QQuickItem *decoration() const;

    bool visibleDecoration() const;

public Q_SLOTS:
    // for titlebar
    void requestMinimize();
    void requestUnminimize();
    void requestToggleMaximize();
    void requestClose();

    bool stackBefore(QQuickItem *item);
    bool stackAfter(QQuickItem *item);
    void stackToLast();

signals:
    void boundedRectChanged();
    void ownsOutputChanged();
    void normalGeometryChanged();
    void maximizedGeometryChanged();
    void fullscreenGeometryChanged();
    void tilingGeometryChanged();
    void positionAutomaticChanged();
    void previousSurfaceStateChanged();
    void surfaceStateChanged();
    void noDecorationChanged();
    void radiusChanged();
    void requestMove(); // for titlebar
    void requestResize(Qt::Edges edges);
    void geometryChanged();
    void containerChanged();
    void visibleDecorationChanged();

private:
    using QQuickItem::setParentItem;
    using QQuickItem::stackBefore;
    using QQuickItem::stackAfter;
    void setParent(QQuickItem *item);
    void setNormalGeometry(const QRectF &newNormalGeometry);
    void setNoDecoration(bool newNoDecoration);
    void setBoundedRect(const QRectF &newBoundedRect);
    void setContainer(SurfaceContainer *newContainer);
    void setVisibleDecoration(bool newVisibleDecoration);
    void updateBoundedRect();
    void updateVisible();
    void updateImplicitHeight();
    void updateSubSurfaceStacking();
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QmlEngine *m_engine;
    QPointer<SurfaceContainer> m_container;
    QList<SurfaceWrapper*> m_subSurfaces;
    SurfaceWrapper *m_parentSurface = nullptr;

    WToplevelSurface *m_shellSurface = nullptr;
    WSurfaceItem *m_surfaceItem = nullptr;
    QPointer<QQuickItem> m_titleBar;
    QPointer<QQuickItem> m_decoration;
    QRectF m_boundedRect;
    QRectF m_normalGeometry;
    QRectF m_maximizedGeometry;
    QRectF m_fullscreenGeometry;
    QRectF m_tilingGeometry;
    Type m_type;
    QPointer<Output> m_ownsOutput;
    QPointF m_positionInOwnsOutput;
    bool m_positionAutomatic = true;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SurfaceWrapper, SurfaceWrapper::State, m_pendingSurfaceState, State::Normal)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SurfaceWrapper, SurfaceWrapper::State, m_previousSurfaceState, State::Normal, &SurfaceWrapper::previousSurfaceStateChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SurfaceWrapper, SurfaceWrapper::State, m_surfaceState, State::Normal, &SurfaceWrapper::surfaceStateChanged)
    bool m_noDecoration = true;
    qreal m_radius = 18.0;
    bool m_visibleDecoration = true;
};

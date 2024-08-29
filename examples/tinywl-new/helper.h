// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "qmlengine.h"

#include <wglobal.h>
#include <wqmlcreator.h>
#include <wseat.h>

#include <QObject>
#include <QQmlApplicationEngine>

Q_MOC_INCLUDE(<wtoplevelsurface.h>)
Q_MOC_INCLUDE("surfacewrapper.h")

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE
class WServer;
class WOutputRenderWindow;
class WOutputLayout;
class WCursor;
class WBackend;
class WOutputItem;
class WOutputViewport;
class WOutputLayer;
class WOutput;
class WXWayland;
class WInputMethodHelper;
class WXdgDecorationManager;
class WSocket;
class WSurface;
class WToplevelSurface;
class WSurfaceItem;
class WForeignToplevel;
WAYLIB_SERVER_END_NAMESPACE

QW_BEGIN_NAMESPACE
class qw_renderer;
class qw_allocator;
class qw_compositor;
QW_END_NAMESPACE

WAYLIB_SERVER_USE_NAMESPACE
QW_USE_NAMESPACE

class Output;
class SurfaceWrapper;
class Workspace;
class SurfaceContainer;
class Helper : public WSeatEventFilter
{
    Q_OBJECT
    Q_PROPERTY(bool socketEnabled READ socketEnabled WRITE setSocketEnabled NOTIFY socketEnabledChanged FINAL)
    Q_PROPERTY(SurfaceWrapper* activatedSurface READ activatedSurface NOTIFY activatedSurfaceChanged FINAL)
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Helper(QObject *parent = nullptr);
    ~Helper();

    QmlEngine *qmlEngine() const;
    void init();

    bool socketEnabled() const;
    void setSocketEnabled(bool newSocketEnabled);

    void activeSurface(SurfaceWrapper *wrapper, Qt::FocusReason reason);

public Q_SLOTS:
    void activeSurface(SurfaceWrapper *wrapper);

signals:
    void socketEnabledChanged();
    void keyboardFocusSurfaceChanged();
    void activatedSurfaceChanged();

private:
    void allowNonDrmOutputAutoChangeMode(WOutput *output);
    void enableOutput(WOutput *output);

    int indexOfOutput(WOutput *output) const;
    Output *getOutput(WOutput *output) const;
    Output *outputAt(WCursor *cursor) const;
    void setPrimaryOutput(Output *output);

    void setOutputProxy(Output *output);

    void addSurface(SurfaceWrapper *surface);
    int indexOfSurface(WSurface *surface) const;
    SurfaceWrapper *getSurface(WSurface *surface) const;
    int indexOfSurface(WToplevelSurface *surface) const;
    SurfaceWrapper *getSurface(WToplevelSurface *surface) const;
    void destroySurface(WSurface *surface);
    void updateLayerSurfaceContainer(SurfaceWrapper *surface);
    void updateSurfaceOwnsOutput(SurfaceWrapper *surface);
    void updateSurfacesOwnsOutput();
    void updateSurfaceOutputs(SurfaceWrapper *surface);

    SurfaceWrapper *keyboardFocusSurface() const;
    void setKeyboardFocusSurface(SurfaceWrapper *newActivateSurface, Qt::FocusReason reason);
    SurfaceWrapper *activatedSurface() const;
    void setActivatedSurface(SurfaceWrapper *newActivateSurface);

    void setCursorPosition(const QPointF &position);
    void ensureCursorPositionValid();
    void ensureSurfaceNormalPositionValid(SurfaceWrapper *surface);

    void stopMoveResize();
    void startMove(SurfaceWrapper *surface, WSeat *seat, int serial);
    void startResize(SurfaceWrapper *surface, WSeat *seat, Qt::Edges edges, int serial);
    void cancelMoveResize(SurfaceWrapper *surface);

    bool startDemoClient();

    bool beforeDisposeEvent(WSeat *seat, QWindow *watched, QInputEvent *event) override;
    bool afterHandleEvent(WSeat *seat, WSurface *watched, QObject *surfaceItem, QObject *, QInputEvent *event) override;
    bool unacceptedEvent(WSeat *, QWindow *, QInputEvent *event) override;

    // qtquick helper
    WOutputRenderWindow *m_renderWindow = nullptr;
    WOutputLayout *m_outputLayout = nullptr;

    // wayland helper
    WServer *m_server = nullptr;
    WSocket *m_socket = nullptr;
    WCursor *m_cursor = nullptr;
    WSeat *m_seat = nullptr;
    WBackend *m_backend = nullptr;
    qw_renderer *m_renderer = nullptr;
    qw_allocator *m_allocator = nullptr;

    // protocols
    qw_compositor *m_compositor = nullptr;
    WXWayland *m_xwayland = nullptr;
    WInputMethodHelper *m_inputMethodHelper = nullptr;
    WXdgDecorationManager *m_xdgDecorationManager = nullptr;
    WForeignToplevel *m_foreignToplevel = nullptr;

    // privaet data
    QList<Output*> m_outputList;
    QPointer<Output> m_primaryOutput;

    enum ContainerZOrder {
        BackgroundZOrder = -2,
        BottomZOrder = -1,
        NormalZOrder = 0,
        TopZOrder = 1,
        OverlayZOrder = 2,
    };

    QList<SurfaceWrapper*> m_surfaceList;
    QPointer<SurfaceWrapper> m_keyboardFocusSurface;
    QPointer<SurfaceWrapper> m_activatedSurface;

    SurfaceContainer *m_surfaceContainer = nullptr;
    SurfaceContainer *m_backgroundContainer = nullptr;
    SurfaceContainer *m_bottomContainer = nullptr;
    Workspace *m_workspace = nullptr;
    SurfaceContainer *m_topContainer = nullptr;
    SurfaceContainer *m_overlayContainer = nullptr;

    // for move resize
    struct {
        SurfaceWrapper *surface = nullptr;
        WSeat *seat = nullptr;
        QPointF surfacePosOfStartMoveResize;
        QSizeF surfaceSizeOfStartMoveResize;
        Qt::Edges resizeEdgets;
    } moveReiszeState;
};

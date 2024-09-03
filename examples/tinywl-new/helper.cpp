// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "helper.h"
#include "surfacewrapper.h"
#include "output.h"
#include "workspace.h"
#include "qmlengine.h"
#include "surfacecontainer.h"

#include <WServer>
#include <WOutput>
#include <WSurfaceItem>
#include <qqmlengine.h>
#include <wxdgsurface.h>
#include <winputpopupsurface.h>
#include <wrenderhelper.h>
#include <WBackend>
#include <wxdgshell.h>
#include <wlayershell.h>
#include <wxwayland.h>
#include <woutputitem.h>
#include <wquickcursor.h>
#include <woutputrenderwindow.h>
#include <wqmlcreator.h>
#include <winputmethodhelper.h>
#include <WForeignToplevel>
#include <WXdgOutput>
#include <wxwaylandsurface.h>
#include <woutputmanagerv1.h>
#include <wcursorshapemanagerv1.h>
#include <woutputitem.h>
#include <woutputlayout.h>
#include <woutputviewport.h>
#include <wseat.h>
#include <wsocket.h>
#include <wtoplevelsurface.h>
#include <wlayersurface.h>
#include <wxdgdecorationmanager.h>

#include <qwbackend.h>
#include <qwdisplay.h>
#include <qwoutput.h>
#include <qwlogging.h>
#include <qwallocator.h>
#include <qwrenderer.h>
#include <qwcompositor.h>
#include <qwsubcompositor.h>
#include <qwxwaylandsurface.h>
#include <qwlayershellv1.h>
#include <qwscreencopyv1.h>
#include <qwfractionalscalemanagerv1.h>
#include <qwgammacontorlv1.h>
#include <qwbuffer.h>
#include <qwoutput.h>
#include <qwoutput.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QProcess>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QLoggingCategory>
#include <QKeySequence>
#include <QQmlComponent>
#include <QVariant>

#define WLR_FRACTIONAL_SCALE_V1_VERSION 1

Helper::Helper(QObject *parent)
    : WSeatEventFilter(parent)
    , m_renderWindow(new WOutputRenderWindow(this))
    , m_outputLayout(new WOutputLayout(this))
    , m_server(new WServer(this))
    , m_cursor(new WCursor(this))
    , m_surfaceContainer(new SurfaceContainer(m_renderWindow->contentItem()))
    , m_backgroundContainer(new SurfaceContainer(m_surfaceContainer))
    , m_bottomContainer(new SurfaceContainer(m_surfaceContainer))
    , m_workspace(new Workspace(m_surfaceContainer))
    , m_topContainer(new SurfaceContainer(m_surfaceContainer))
    , m_overlayContainer(new SurfaceContainer(m_surfaceContainer))
{
    m_cursor->setLayout(m_outputLayout);
    m_cursor->setEventWindow(m_renderWindow);
    m_surfaceContainer->setFlag(QQuickItem::ItemIsFocusScope, true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    m_surfaceContainer->setFocusPolicy(Qt::StrongFocus);
#endif
    m_backgroundContainer->setZ(ContainerZOrder::BackgroundZOrder);
    m_bottomContainer->setZ(ContainerZOrder::BottomZOrder);
    m_workspace->setZ(ContainerZOrder::NormalZOrder);
    m_topContainer->setZ(ContainerZOrder::TopZOrder);
    m_overlayContainer->setZ(ContainerZOrder::OverlayZOrder);

    connect(m_outputLayout, &WOutputLayout::implicitWidthChanged, this, [this] {
        const auto width = m_outputLayout->implicitWidth();
        m_renderWindow->setWidth(width);
        m_surfaceContainer->setWidth(width);
        m_backgroundContainer->setWidth(width);
        m_bottomContainer->setWidth(width);
        m_topContainer->setWidth(width);
        m_overlayContainer->setWidth(width);
        m_workspace->setWidth(width);
    });

    connect(m_outputLayout, &WOutputLayout::implicitHeightChanged, this, [this] {
        const auto height = m_outputLayout->implicitHeight();
        m_renderWindow->setHeight(height);
        m_surfaceContainer->setHeight(height);
        m_backgroundContainer->setHeight(height);
        m_bottomContainer->setHeight(height);
        m_topContainer->setHeight(height);
        m_overlayContainer->setHeight(height);
        m_workspace->setHeight(height);
    });

    connect(m_outputLayout, &WOutputLayout::notify_change, this, [this] {
        ensureCursorPositionValid();

        for (auto s : std::as_const(m_surfaceList)) {
            ensureSurfaceNormalPositionValid(s);
            updateSurfaceOutputs(s);
        }
    });
}

Helper::~Helper()
{

}

QmlEngine *Helper::qmlEngine() const
{
    return qobject_cast<QmlEngine*>(::qmlEngine(this));
}

void Helper::init()
{
    m_seat = m_server->attach<WSeat>();
    m_seat->setEventFilter(this);
    m_seat->setCursor(m_cursor);
    m_seat->setKeyboardFocusWindow(m_renderWindow);

    m_backend = m_server->attach<WBackend>();
    connect(m_backend, &WBackend::inputAdded, this, [this] (WInputDevice *device) {
        m_seat->attachInputDevice(device);
    });

    connect(m_backend, &WBackend::inputRemoved, this, [this] (WInputDevice *device) {
        m_seat->detachInputDevice(device);
    });

    connect(m_backend, &WBackend::outputAdded, this, [this] (WOutput *output) {
        allowNonDrmOutputAutoChangeMode(output);
        auto o = Output::createPrimary(output, qmlEngine(), this);
        o->outputItem()->setParentItem(m_renderWindow->contentItem());
        o->outputItem()->stackBefore(m_surfaceContainer);
        m_outputList.append(o);
        m_outputLayout->autoAdd(output);

        if (!m_primaryOutput) {
            setPrimaryOutput(o);

            for (auto s : std::as_const(m_surfaceList)) {
                updateSurfaceOwnsOutput(s);
                ensureSurfaceNormalPositionValid(s);
            }
        }

        enableOutput(output);
    });

    connect(m_backend, &WBackend::outputRemoved, this, [this] (WOutput *output) {
        auto index = indexOfOutput(output);
        Q_ASSERT(index >= 0);
        const auto o = m_outputList.takeAt(index);
        auto primaryOutput = m_primaryOutput;

        if (primaryOutput == o) {
            primaryOutput = nullptr;
            for (auto output : std::as_const(m_outputList)) {
                if (output->isPrimary()) {
                    primaryOutput = output;
                    break;
                }
            }
        }

        if (primaryOutput) {
            const auto outputPos = o->outputItem()->position();
            if (o->geometry().contains(m_cursor->position())) {
                const auto posInOutput = m_cursor->position() - outputPos;
                setCursorPosition(primaryOutput->outputItem()->position() + posInOutput);
            }

            for (auto s : std::as_const(m_surfaceList)) {
                if (s->type() == SurfaceWrapper::Type::Layer)
                    continue;

                if (s->ownsOutput() == o) {
                    s->setOwnsOutput(m_primaryOutput);
                    const auto posInOutput = s->position() - outputPos;
                    s->setPosition(m_primaryOutput->outputItem()->position() + posInOutput);
                }
            }
        }

        if (m_moveResizeSurface && m_moveResizeSurface->ownsOutput() == o) {
            stopMoveResize();
        }

        m_outputLayout->remove(o->output());
        delete o;
    });

    auto *xdgShell = m_server->attach<WXdgShell>();
    m_foreignToplevel = m_server->attach<WForeignToplevel>(xdgShell);
    auto *layerShell = m_server->attach<WLayerShell>();
    auto *xdgOutputManager = m_server->attach<WXdgOutputManager>(m_outputLayout);

    connect(xdgShell, &WXdgShell::surfaceAdded, this, [this] (WXdgSurface *surface) {
        SurfaceWrapper *wrapper = nullptr;

        if (surface->isToplevel()) {
            wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::XdgToplevel);
            m_foreignToplevel->addSurface(surface);
        } else {
            wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::XdgPopup);
        }

        wrapper->setNoDecoration(m_xdgDecorationManager->modeBySurface(surface->surface())
                                 != WXdgDecorationManager::Server);
        addSurface(wrapper);

        if (auto parent = surface->parentSurface()) {
            auto parentWrapper = getSurface(parent);
            auto container = parentWrapper->container();
            Q_ASSERT(container);
            container->addSurface(wrapper);
        } else {
            m_workspace->addSurface(wrapper);
        }

        Q_ASSERT(wrapper->parentItem());
    });
    connect(xdgShell, &WXdgShell::surfaceRemoved, this, [this] (WXdgSurface *surface) {
        if (surface->isToplevel()) {
            m_foreignToplevel->removeSurface(surface);
        }

        destroySurface(surface->surface());
    });

    connect(layerShell, &WLayerShell::surfaceAdded, this, [this] (WLayerSurface *surface) {
        auto wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::Layer);
        wrapper->setNoDecoration(true);
        addSurface(wrapper);

        updateLayerSurfaceContainer(wrapper);

        connect(surface, &WLayerSurface::layerChanged, this, [this, wrapper] {
            updateLayerSurfaceContainer(wrapper);
        });
        Q_ASSERT(wrapper->parentItem());
    });

    connect(layerShell, &WLayerShell::surfaceRemoved, this, [this] (WLayerSurface *surface) {
        destroySurface(surface->surface());
    });

    m_server->start();
    m_renderer = WRenderHelper::createRenderer(m_backend->handle());
    if (!m_renderer) {
        qFatal("Failed to create renderer");
    }

    m_allocator = qw_allocator::autocreate(*m_backend->handle(), *m_renderer);
    m_renderer->init_wl_display(*m_server->handle());

    // free follow display
    m_compositor = qw_compositor::create(*m_server->handle(), 6, *m_renderer);
    qw_subcompositor::create(*m_server->handle());
    qw_screencopy_manager_v1::create(*m_server->handle());
    m_renderWindow->init(m_renderer, m_allocator);

    // for xwayland
    auto *xwaylandOutputManager = m_server->attach<WXdgOutputManager>(m_outputLayout);
    xwaylandOutputManager->setScaleOverride(1.0);

    auto xwayland_lazy = true;
    m_xwayland = m_server->attach<WXWayland>(m_compositor, xwayland_lazy);
    m_xwayland->setSeat(m_seat);

    xdgOutputManager->setFilter([this] (WClient *client) {
        return client != m_xwayland->waylandClient();
    });
    xwaylandOutputManager->setFilter([this] (WClient *client) {
        return client == m_xwayland->waylandClient();
    });

    connect(m_xwayland, &WXWayland::surfaceAdded, this, [this] (WXWaylandSurface *surface) {
        surface->safeConnect(&qw_xwayland_surface::notify_associate, this, [this, surface] {
            auto wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::XWayland);
            wrapper->setNoDecoration(false);
            m_foreignToplevel->addSurface(surface);
            addSurface(wrapper);
            m_workspace->addSurface(wrapper);
            Q_ASSERT(wrapper->parentItem());
        });
        surface->safeConnect(&qw_xwayland_surface::notify_dissociate, this, [this, surface] {
            m_foreignToplevel->removeSurface(surface);
            destroySurface(surface->surface());
        });
    });

    m_inputMethodHelper = new WInputMethodHelper(m_server, m_seat);

    // connect(m_inputMethodHelper, &WInputMethodHelper::inputPopupSurfaceV2Added, this, [this](WInputPopupSurface *inputPopup) {
    //     auto initProperties = m_qmlEngine->newObject();
    //     initProperties.setProperty("popupSurface", m_qmlEngine->toScriptValue(inputPopup));
    //     m_inputPopupCreator->add(inputPopup, initProperties);
    // });

    // connect(m_inputMethodHelper, &WInputMethodHelper::inputPopupSurfaceV2Removed, m_inputPopupCreator, &WQmlCreator::removeByOwner);

    m_xdgDecorationManager = m_server->attach<WXdgDecorationManager>();
    connect(m_xdgDecorationManager, &WXdgDecorationManager::surfaceModeChanged,
            this, [this] (WSurface *surface, WXdgDecorationManager::DecorationMode mode) {
        auto s = getSurface(surface);
        if (!s)
            return;
        s->setNoDecoration(mode != WXdgDecorationManager::Server);
    });

    bool freezeClientWhenDisable = false;
    m_socket = new WSocket(freezeClientWhenDisable);
    if (m_socket->autoCreate()) {
        m_server->addSocket(m_socket);
    } else {
        delete m_socket;
        qCritical("Failed to create socket");
        return;
    }

    auto gammaControlManager = qw_gamma_control_manager_v1::create(*m_server->handle());
    connect(gammaControlManager, &qw_gamma_control_manager_v1::notify_set_gamma, this, [this]
            (wlr_gamma_control_manager_v1_set_gamma_event *event) {
        auto *qwOutput = qw_output::from(event->output);
        auto *wOutput = WOutput::fromHandle(qwOutput);
        size_t ramp_size = 0;
        uint16_t *r = nullptr, *g = nullptr, *b = nullptr;
        wlr_gamma_control_v1 *gamma_control = event->control;
        if (gamma_control) {
            ramp_size = gamma_control->ramp_size;
            r = gamma_control->table;
            g = gamma_control->table + gamma_control->ramp_size;
            b = gamma_control->table + 2 * gamma_control->ramp_size;
            if (!wOutput->setGammaLut(ramp_size, r, g, b)) {
                qw_gamma_control_v1::from(gamma_control)->send_failed_and_destroy();
            }
        }
    });

    auto wOutputManager = m_server->attach<WOutputManagerV1>();
    connect(wOutputManager, &WOutputManagerV1::requestTestOrApply, this, [this, wOutputManager]
            (qw_output_configuration_v1 *config, bool onlyTest) {
        QList<WOutputState> states = wOutputManager->stateListPending();
        bool ok = true;
        for (auto state : std::as_const(states)) {
            WOutput *output = state.output;
            output->enable(state.enabled);
            if (state.enabled) {
                if (state.mode)
                    output->setMode(state.mode);
                else
                    output->setCustomMode(state.customModeSize, state.customModeRefresh);

                output->enableAdaptiveSync(state.adaptiveSyncEnabled);
                if (!onlyTest) {
                    WOutputItem *item = getOutput(output)->outputItem();
                    if (item) {
                        WOutputViewport *viewport = item->property("onscreenViewport").value<WOutputViewport *>();
                        if (viewport) {
                                    viewport->rotateOutput(state.transform);
                                    viewport->setOutputScale(state.scale);
                                    viewport->setX(state.x);
                                    viewport->setY(state.y);
                        }
                    }
                }
            }

            if (onlyTest)
                ok &= output->test();
            else
                ok &= output->commit();
        }
        wOutputManager->sendResult(config, ok);
    });

    m_server->attach<WCursorShapeManagerV1>();
    qw_fractional_scale_manager_v1::create(*m_server->handle(), WLR_FRACTIONAL_SCALE_V1_VERSION);

    m_backend->handle()->start();

    qInfo() << "Listing on:" << m_socket->fullServerName();
    startDemoClient();
}

bool Helper::socketEnabled() const
{
    return m_socket->isEnabled();
}

void Helper::setSocketEnabled(bool newEnabled)
{
    if (m_socket)
        m_socket->setEnabled(newEnabled);
    else
        qWarning() << "Can't set enabled for empty socket!";
}

void Helper::activeSurface(SurfaceWrapper *wrapper, Qt::FocusReason reason)
{
    setActivatedSurface(wrapper);
    setKeyboardFocusSurface(wrapper, reason);
}

void Helper::activeSurface(SurfaceWrapper *wrapper)
{
    activeSurface(wrapper, Qt::OtherFocusReason);
}

void Helper::stopMoveResize()
{
    if (!m_moveResizeSurface)
        return;
    auto o = m_moveResizeSurface->ownsOutput();
    if (o && o->moveResizeSurface()) {
        Q_ASSERT(o->moveResizeSurface() == m_moveResizeSurface);
        o->endMoveResize();
        Q_ASSERT(!m_moveResizeSurface);
        return;
    }
    m_moveResizeSurface->shellSurface()->setResizeing(false);

    if (!o || !m_moveResizeSurface->surface()->outputs().contains(o->output())) {
        o = outputAt(m_cursor);
        Q_ASSERT(o);
        m_moveResizeSurface->setOwnsOutput(o);
    }

    ensureSurfaceNormalPositionValid(m_moveResizeSurface);
    m_moveResizeSurface = nullptr;
}

void Helper::startMove(SurfaceWrapper *surface, WSeat *seat, int serial)
{
    Q_ASSERT(seat == m_seat);
    auto o = surface->ownsOutput();
    if (!o)
        return;

    stopMoveResize();

    Q_UNUSED(serial)

    m_moveResizeSurface = surface;
    connect(o, &Output::moveResizeFinised, this, &Helper::stopMoveResize, Qt::SingleShotConnection);
    o->beginMoveResize(surface, Qt::Edges{0});

    activeSurface(surface);
}

void Helper::startResize(SurfaceWrapper *surface, WSeat *seat, Qt::Edges edge, int serial)
{
    Q_ASSERT(seat == m_seat);
    auto o = surface->ownsOutput();
    if (!o)
        return;

    stopMoveResize();

    Q_UNUSED(serial)
    Q_ASSERT(edge != 0);

    m_moveResizeSurface = surface;
    connect(o, &Output::moveResizeFinised, this, &Helper::stopMoveResize, Qt::SingleShotConnection);
    o->beginMoveResize(surface, edge);
    surface->shellSurface()->setResizeing(true);
    activeSurface(surface);
}

void Helper::cancelMoveResize(SurfaceWrapper *surface)
{
    if (m_moveResizeSurface != surface)
        return;
    stopMoveResize();
}

bool Helper::startDemoClient()
{
#ifdef START_DEMO
    QProcess waylandClientDemo;

    waylandClientDemo.setProgram(PROJECT_BINARY_DIR"/examples/animationclient/animationclient");
    waylandClientDemo.setArguments({"-platform", "wayland"});
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("WAYLAND_DISPLAY", m_socket->fullServerName());

    waylandClientDemo.setProcessEnvironment(env);
    return waylandClientDemo.startDetached();
#else
    return false;
#endif
}

bool Helper::beforeDisposeEvent(WSeat *seat, QWindow *, QInputEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto kevent = static_cast<QKeyEvent*>(event);
        if (QKeySequence(kevent->keyCombination()) == QKeySequence::Quit) {
            qApp->quit();
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
        seat->cursor()->setVisible(true);
    } else if (event->type() == QEvent::TouchBegin) {
        seat->cursor()->setVisible(false);
    }

    if (m_moveResizeSurface) {
        // for move resize
        if (Q_LIKELY(event->type() == QEvent::MouseMove || event->type() == QEvent::TouchUpdate)) {
            auto cursor = seat->cursor();
            Q_ASSERT(cursor);
            QMouseEvent *ev = static_cast<QMouseEvent*>(event);

            auto ownsOutput = m_moveResizeSurface->ownsOutput();
            if (!ownsOutput) {
                stopMoveResize();
                return false;
            }

            auto increment_pos = ev->globalPosition() - cursor->lastPressedOrTouchDownPosition();
            ownsOutput->doMoveResize(increment_pos);

            return true;
        } else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd) {
            stopMoveResize();
        }
    }

    return false;
}

bool Helper::afterHandleEvent(WSeat *seat, WSurface *watched, QObject *surfaceItem, QObject *, QInputEvent *event)
{
    Q_UNUSED(seat)

    if (event->isSinglePointEvent() && static_cast<QSinglePointEvent*>(event)->isBeginEvent()) {
        // surfaceItem is qml type: XdgSurfaceItem or LayerSurfaceItem
        auto toplevelSurface = qobject_cast<WSurfaceItem*>(surfaceItem)->shellSurface();
        if (!toplevelSurface)
            return false;
        Q_ASSERT(toplevelSurface->surface() == watched);
        if (auto *xdgSurface = qobject_cast<WXdgSurface *>(toplevelSurface)) {
            // TODO: popupSurface should not inherit WToplevelSurface
            if (xdgSurface->isPopup()) {
                return false;
            }
        }

        auto surface = getSurface(watched);
        activeSurface(surface, Qt::MouseFocusReason);
    }

    return false;
}

bool Helper::unacceptedEvent(WSeat *, QWindow *, QInputEvent *event)
{
    if (event->isSinglePointEvent()) {
        if (static_cast<QSinglePointEvent*>(event)->isBeginEvent()) {
            activeSurface(nullptr, Qt::OtherFocusReason);
        }
    }

    return false;
}

SurfaceWrapper *Helper::keyboardFocusSurface() const
{
    return m_keyboardFocusSurface;
}

void Helper::setKeyboardFocusSurface(SurfaceWrapper *newActivate, Qt::FocusReason reason)
{
    if (m_keyboardFocusSurface == newActivate)
        return;

    if (newActivate && newActivate->shellSurface()->doesNotAcceptFocus())
        return;

    if (m_keyboardFocusSurface) {
        if (newActivate) {
            if (m_keyboardFocusSurface->shellSurface()->keyboardFocusPriority()
                > newActivate->shellSurface()->keyboardFocusPriority())
                return;
        } else {
            if (m_keyboardFocusSurface->shellSurface()->keyboardFocusPriority() > 0)
                return;
        }
    }

    if (newActivate) {
        newActivate->setFocus(true, reason);
        m_seat->setKeyboardFocusSurface(newActivate->surface());
    } else if (m_keyboardFocusSurface) {
        m_keyboardFocusSurface->setFocus(false, reason);
        m_seat->setKeyboardFocusSurface(nullptr);
    }

    m_keyboardFocusSurface = newActivate;

    Q_EMIT keyboardFocusSurfaceChanged();
}

SurfaceWrapper *Helper::activatedSurface() const
{
    return m_activatedSurface;
}

void Helper::setActivatedSurface(SurfaceWrapper *newActivateSurface)
{
    if (m_activatedSurface == newActivateSurface)
        return;

    if (newActivateSurface) {
        newActivateSurface->stackToLast();
    }

    if (m_activatedSurface)
        m_activatedSurface->shellSurface()->setActivate(false);
    if (newActivateSurface)
        newActivateSurface->shellSurface()->setActivate(true);
    m_activatedSurface = newActivateSurface;
    Q_EMIT activatedSurfaceChanged();
}

void Helper::setCursorPosition(const QPointF &position)
{
    stopMoveResize();
    m_seat->setCursorPosition(position);
}

void Helper::ensureCursorPositionValid()
{
    const auto cursorPos = m_cursor->position();
    if (m_outputLayout->output_at(cursorPos.x(), cursorPos.y()))
        return;

    if (m_primaryOutput) {
        setCursorPosition(m_primaryOutput->geometry().center());
    } else {
        setCursorPosition(QPointF(0, 0));
    }
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

void Helper::ensureSurfaceNormalPositionValid(SurfaceWrapper *surface)
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

void Helper::allowNonDrmOutputAutoChangeMode(WOutput *output)
{
    output->safeConnect(&qw_output::notify_request_state,
        this, [this] (wlr_output_event_request_state *newState) {
        if (newState->state->committed & WLR_OUTPUT_STATE_MODE) {
            auto output = qobject_cast<qw_output*>(sender());

            if (newState->state->mode_type == WLR_OUTPUT_STATE_MODE_CUSTOM) {
                output->set_custom_mode(newState->state->custom_mode.width,
                                        newState->state->custom_mode.height,
                                        newState->state->custom_mode.refresh);
            } else {
                output->set_mode(newState->state->mode);
            }

            output->commit();
        }
    });
}

void Helper::enableOutput(WOutput *output)
{
    // Enable on default
    auto qwoutput = output->handle();
    // Don't care for WOutput::isEnabled, must do WOutput::commit here,
    // In order to ensure trigger QWOutput::frame signal, WOutputRenderWindow
    // needs this signal to render next frmae. Because QWOutput::frame signal
    // maybe emit before WOutputRenderWindow::attach, if no commit here,
    // WOutputRenderWindow will ignore this ouptut on render.
    if (!qwoutput->property("_Enabled").toBool()) {
        qwoutput->setProperty("_Enabled", true);

        if (!qwoutput->handle()->current_mode) {
            auto mode = qwoutput->preferred_mode();
            if (mode)
                output->setMode(mode);
        }
        output->enable(true);
        bool ok = output->commit();
        Q_ASSERT(ok);
    }
}

int Helper::indexOfOutput(WOutput *output) const
{
    for (int i = 0; i < m_outputList.size(); i++) {
        if (m_outputList.at(i)->output() == output)
            return i;
    }
    return -1;
}

Output *Helper::getOutput(WOutput *output) const
{
    for (auto o : std::as_const(m_outputList)) {
        if (o->output() == output)
            return o;
    }
    return nullptr;
}

Output *Helper::outputAt(WCursor *cursor) const
{
    Q_ASSERT(cursor->layout() == m_outputLayout);
    const auto &pos = cursor->position();
    auto o = m_outputLayout->output_at(pos.x(), pos.y());
    if (!o)
        return nullptr;

    return getOutput(WOutput::fromHandle(qw_output::from(o)));
}

void Helper::setPrimaryOutput(Output *output)
{
    if (m_primaryOutput == output)
        return;
    m_primaryOutput = output;
    updateSurfacesOwnsOutput();
}

void Helper::setOutputProxy(Output *output)
{

}

void Helper::addSurface(SurfaceWrapper *surface)
{
    m_surfaceList << surface;

    connect(surface, &SurfaceWrapper::requestMove, this, [this] {
        auto surface = qobject_cast<SurfaceWrapper*>(sender());
        Q_ASSERT(surface);
        startMove(surface, m_seat, 0);
    });
    connect(surface, &SurfaceWrapper::requestResize, this, [this] (Qt::Edges edges) {
        auto surface = qobject_cast<SurfaceWrapper*>(sender());
        Q_ASSERT(surface);
        startResize(surface, m_seat, edges, 0);
    });
    connect(surface, &SurfaceWrapper::surfaceStateChanged, this, [surface, this] {
        if (surface->surfaceState() == SurfaceWrapper::State::Minimized
            || surface->surfaceState() == SurfaceWrapper::State::Tiling)
            return;
        activeSurface(surface);
    });
    connect(surface, &SurfaceWrapper::geometryChanged, this, [this, surface] {
        updateSurfaceOutputs(surface);
    });

    updateSurfaceOwnsOutput(surface);
    updateSurfaceOutputs(surface);
    activeSurface(surface, Qt::OtherFocusReason);
}

int Helper::indexOfSurface(WSurface *surface) const
{
    for (int i = 0; i < m_surfaceList.size(); i++) {
        if (m_surfaceList.at(i)->surface() == surface)
            return i;
    }
    return -1;
}

SurfaceWrapper *Helper::getSurface(WSurface *surface) const
{
    for (const auto &wrapper: std::as_const(m_surfaceList)) {
        if (wrapper->surface() == surface)
            return wrapper;
    }
    return nullptr;
}

int Helper::indexOfSurface(WToplevelSurface *surface) const
{
    for (int i = 0; i < m_surfaceList.size(); i++) {
        if (m_surfaceList.at(i)->shellSurface() == surface)
            return i;
    }
    return -1;
}

SurfaceWrapper *Helper::getSurface(WToplevelSurface *surface) const
{
    for (const auto &wrapper: m_surfaceList) {
        if (wrapper->shellSurface() == surface)
            return wrapper;
    }
    return nullptr;
}

void Helper::destroySurface(WSurface *surface)
{
    auto index = indexOfSurface(surface);
    Q_ASSERT(index >= 0);
    auto wrapper = m_surfaceList.takeAt(index);
    if (wrapper == m_moveResizeSurface)
        stopMoveResize();

    if (wrapper->type() != SurfaceWrapper::Type::Layer)
        m_workspace->removeSurface(wrapper);

    delete wrapper;
}

void Helper::updateLayerSurfaceContainer(SurfaceWrapper *surface)
{
    auto layer = qobject_cast<WLayerSurface*>(surface->shellSurface());
    Q_ASSERT(layer);

    if (auto oldContainer = surface->container())
        oldContainer->removeSurface(surface);

    switch (layer->layer()) {
    case WLayerSurface::LayerType::Background:
        m_backgroundContainer->addSurface(surface);
        break;
    case WLayerSurface::LayerType::Bottom:
        m_bottomContainer->addSurface(surface);
        break;
    case WLayerSurface::LayerType::Top:
        m_topContainer->addSurface(surface);
        break;
    case WLayerSurface::LayerType::Overlay:
        m_overlayContainer->addSurface(surface);
        break;
    default:
        Q_UNREACHABLE_RETURN();
    }
}

void Helper::updateSurfaceOwnsOutput(SurfaceWrapper *surface)
{
    if (surface->type() == SurfaceWrapper::Type::Layer) {
        auto layer = qobject_cast<WLayerSurface*>(surface->shellSurface());
        if (auto output = layer->output()) {
            auto o = getOutput(output);
            Q_ASSERT(o);
            surface->setOwnsOutput(o);
        } else if (m_primaryOutput) {
            surface->setOwnsOutput(m_primaryOutput);
        } else {
            surface->setOwnsOutput(nullptr);
        }
    } else {
        auto outputs = surface->surface()->outputs();
        if (surface->ownsOutput() && outputs.contains(surface->ownsOutput()->output()))
            return;

        Output *output = nullptr;
        if (!outputs.isEmpty())
            output = getOutput(outputs.first());
        if (!output)
            output = outputAt(m_cursor);
        if (!output)
            output = m_primaryOutput;
        if (output)
            surface->setOwnsOutput(output);
    }
}

void Helper::updateSurfacesOwnsOutput()
{
    for (auto surface : std::as_const(m_surfaceList)) {
        updateSurfaceOwnsOutput(surface);
    }
}

void Helper::updateSurfaceOutputs(SurfaceWrapper *surface)
{
    const QRectF geometry(surface->position(), surface->size());
    auto outputs = m_outputLayout->getIntersectedOutputs(geometry.toRect());
    surface->setOutputs(outputs);
}

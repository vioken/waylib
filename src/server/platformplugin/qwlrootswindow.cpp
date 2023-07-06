// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwlrootswindow.h"
#include "qwlrootscreen.h"
#include "qwlrootsintegration.h"
#include "woutput.h"

#include <qwbuffer.h>
#include <qwrenderer.h>
#include <qwoutput.h>

#include <QCoreApplication>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <private/qguiapplication_p.h>

extern "C" {
#define static
#include <wlr/types/wlr_output.h>
#undef static
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE

QWlrootsOutputWindow::QWlrootsOutputWindow(QWindow *window)
    : QPlatformWindow(window)
{

}

QWlrootsOutputWindow::~QWlrootsOutputWindow()
{
    setBuffer(nullptr);
}

void QWlrootsOutputWindow::initialize()
{
    auto onGeometryChanged = [this] {
        const QRect newGeo = geometry();
        QWindowSystemInterface::handleGeometryChange(window(), newGeo);
    };

    QObject::connect(window(), &QWindow::screenChanged,  window(),[this, onGeometryChanged] (QScreen *newScreen) {
        if (onScreenGeometryConnection)
            QObject::disconnect(onScreenGeometryConnection);
        onScreenGeometryConnection = QObject::connect(newScreen, &QScreen::geometryChanged, window(), onGeometryChanged);
    });

    onScreenGeometryConnection = QObject::connect(screen()->screen(), &QScreen::geometryChanged, window(), onGeometryChanged);
    QMetaObject::invokeMethod(window(), onGeometryChanged, Qt::QueuedConnection);
}

QWlrootsScreen *QWlrootsOutputWindow::qwScreen() const
{
    return dynamic_cast<QWlrootsScreen*>(this->screen());
}

QPlatformScreen *QWlrootsOutputWindow::screen() const
{
    return QPlatformWindow::screen();
}

void QWlrootsOutputWindow::setGeometry(const QRect &rect)
{
    auto screen = qwScreen();
    Q_ASSERT(screen);
    screen->move(rect.topLeft());
}

QRect QWlrootsOutputWindow::geometry() const
{
    return screen()->geometry();
}

WId QWlrootsOutputWindow::winId() const
{
    return reinterpret_cast<WId>(this);
}

qreal QWlrootsOutputWindow::devicePixelRatio() const
{
    return 1.0;
}

void QWlrootsOutputWindow::setBuffer(QWBuffer *buffer)
{
    Q_ASSERT(!bufferAttached);
    if (renderBuffer)
        renderBuffer->unlock();

    renderBuffer = buffer;
}

QWBuffer *QWlrootsOutputWindow::buffer() const
{
    return renderBuffer.get();
}

bool QWlrootsOutputWindow::attachRenderer()
{
    if (!renderBuffer)
        return false;

    if (bufferAttached)
        return true;

    if (!qwScreen())
        return false;

    QWRenderer *renderer = qwScreen()->output()->renderer();
    bool ok = wlr_renderer_begin_with_buffer(renderer->handle(), renderBuffer->handle());
    if (!ok) {
        // Drop the error buffer
        renderBuffer->unlock();
        return false;
    }

    auto qwOutput = qwScreen()->output()->nativeInterface<QWOutput>();
    qwOutput->attachBuffer(renderBuffer);
    bufferAttached = true;

    return true;
}

void QWlrootsOutputWindow::detachRenderer()
{
    Q_ASSERT(renderBuffer && bufferAttached);
    QWRenderer *renderer = qwScreen()->output()->renderer();
    auto qwOutput = qwScreen()->output()->nativeInterface<QWOutput>();
    renderer->end();
    qwOutput->rollback();
    bufferAttached = false;
}

WAYLIB_SERVER_END_NAMESPACE

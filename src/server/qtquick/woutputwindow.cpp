// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "woutputwindow.h"
#include "woutput.h"
#include "wserver.h"
#include "wthreadutils.h"
#include "platformplugin/qwlrootsintegration.h"
#include "platformplugin/qwlrootscreen.h"

#include <QQuickRenderControl>
#include <private/qquickwindow_p.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

class WOutputWindowPrivate : public WObjectPrivate
{
public:
    WOutputWindowPrivate(WOutputWindow *qq)
        : WObjectPrivate(qq) {}

    W_DECLARE_PUBLIC(WOutputWindow)

    WOutput *output = nullptr;
};

WOutput *WOutputWindow::output() const
{
    W_DC(WOutputWindow);
    return d->output;
}

void WOutputWindow::setOutput(WOutput *newOutput)
{
    W_D(WOutputWindow);
    if (d->output == newOutput)
        return;

    Q_ASSERT(!d->output);
    d->output = newOutput;

    setScreen(QWlrootsIntegration::instance()->getScreenFrom(d->output)->screen());
    create();

    d->output->attach(renderControl());

    Q_EMIT outputChanged();
}

WOutputWindow::WOutputWindow(QObject *parent)
    : QQuickWindow(new QQuickRenderControl())
{
    if (parent)
        QObject::setParent(parent);
    setObjectName(QT_STRINGIFY(WAYLIB_SERVER_NAMESPACE));
}

WOutputWindow::~WOutputWindow()
{
    delete renderControl();
}

QQuickRenderControl *WOutputWindow::renderControl() const
{
    auto wd = QQuickWindowPrivate::get(this);
    return wd->renderControl;
}

WAYLIB_SERVER_END_NAMESPACE

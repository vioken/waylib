// Copyright (C) 2024 Lu YaNing <luyaning@uniontech.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquickxdgoutput_p.h"
#include "woutputlayout.h"
#include "private/wglobal_p.h"

#include <qwxdgoutputv1.h>
#include <QQmlInfo>

extern "C" {
#define static
#include <wlr/types/wlr_xdg_output_v1.h>
#undef static
}

WAYLIB_SERVER_BEGIN_NAMESPACE

using QW_NAMESPACE::QWXdgOutputManagerV1;

class WQuickXdgOutputManagerPrivate : public WObjectPrivate
{
public:
    WQuickXdgOutputManagerPrivate(WQuickXdgOutputManager *qq)
        : WObjectPrivate(qq)
    {

    }

    W_DECLARE_PUBLIC(WQuickXdgOutputManager)

    WOutputLayout *layout = nullptr;
};

WQuickXdgOutputManager::WQuickXdgOutputManager(QObject *parent)
    : WQuickWaylandServerInterface(parent)
    , WObject(*new WQuickXdgOutputManagerPrivate(this), nullptr)
{

}

void WQuickXdgOutputManager::setLayout(WOutputLayout *layout)
{
    W_D(WQuickXdgOutputManager);

    if (d->layout) {
        qmlWarning(this) << "Trying to set output layout for xdg output manager twice. Ignore this request";
        return;
    }

    d->layout = layout;
}

WOutputLayout *WQuickXdgOutputManager::layout() const
{
    W_DC(WQuickXdgOutputManager);

    return d->layout;
}

WServerInterface *WQuickXdgOutputManager::create()
{
    W_D(WQuickXdgOutputManager);

    if (!d->layout)
        qFatal() << "Output layout not set, xdg output manager will never be created!";

    auto handle = QWXdgOutputManagerV1::create(server()->handle(), d->layout);

    return new WServerInterface(handle, handle->handle()->global);
}

WAYLIB_SERVER_END_NAMESPACE

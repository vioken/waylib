// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquickbackend.h"
#include "wbackend.h"
#include "woutput.h"
#include "private/woutputlayout_p.h"

WAYLIB_SERVER_BEGIN_NAMESPACE

class WQuickBackendPrivate : public WObjectPrivate
{
public:
    WQuickBackendPrivate(WQuickBackend *qq)
        : WObjectPrivate(qq)
    {
    }

    void onOutputAdded(WOutput *output);
    void onOutputRemoved(WOutput *output);

    W_DECLARE_PUBLIC(WQuickBackend)

    WServer *server = nullptr;
    WBackend *backend = nullptr;
    WOutputLayout *layout = nullptr;
};

void WQuickBackendPrivate::onOutputAdded(WOutput *output)
{
    W_Q(WQuickBackend);
    Q_EMIT q->outputAdded(output);
}

void WQuickBackendPrivate::onOutputRemoved(WOutput *output)
{
    W_Q(WQuickBackend);
    Q_EMIT q->outputRemoved(output);
}

WQuickBackend::WQuickBackend(QObject *parent)
    : QObject(parent)
    , WObject(*new WQuickBackendPrivate(this))
{

}

WServer *WQuickBackend::server() const
{
    W_DC(WQuickBackend);
    return d->server;
}

void WQuickBackend::setServer(WServer *newServer)
{
    W_D(WQuickBackend);
    if (d->server == newServer)
        return;
    if (d->server) {
        Q_ASSERT(d->backend);
        d->server->detach(d->backend);
        d->backend = nullptr;
    }
    d->server = newServer;
    if (d->server) {
        if (!d->layout) {
            d->layout = new WOutputLayout(this);
            connect(d->layout, SIGNAL(outputAdded(WOutput*)), this, SLOT(onOutputAdded(WOutput*)));
            connect(d->layout, SIGNAL(outputRemoved(WOutput*)), this, SLOT(onOutputRemoved(WOutput*)));
            Q_EMIT layoutChanged();
        }
        d->backend = d->server->attach<WBackend>(d->layout);
    }

    Q_EMIT serverChanged();
}

WOutputLayout *WQuickBackend::layout() const
{
    W_DC(WQuickBackend);
    return d->layout;
}

WAYLIB_SERVER_END_NAMESPACE

#include "moc_wquickbackend.cpp"

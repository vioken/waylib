// Copyright (C) 2024 YaoBing Xiao <xiaoyaobing@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquicksecuritycontextmanagerv1_p.h"
#include "wsocket.h"
#include "private/wglobal_p.h"

#include <qwsecuritycontextmanagerv1.h>

extern "C" {
#define class className
#include <wlr/types/wlr_security_context_v1.h>
#undef class
}

#define WLR_FRACTIONAL_SCALE_V1_VERSION 1

WAYLIB_SERVER_BEGIN_NAMESPACE

static WQuickSecurityContextManagerV1 *SECURITY_CONTEXT_MANAGER = nullptr;

class WQuickSecurityContextManagerV1Private : public WObjectPrivate {
public:
    WQuickSecurityContextManagerV1Private(WQuickSecurityContextManagerV1 *qq)
        : WObjectPrivate(qq) {}
    ~WQuickSecurityContextManagerV1Private() = default;

    W_DECLARE_PUBLIC(WQuickSecurityContextManagerV1)

    QWLRoots::QWSecurityContextManagerV1 *manager = nullptr;
};

WQuickSecurityContextManagerV1::WQuickSecurityContextManagerV1(QObject *parent)
    : WQuickWaylandServerInterface(parent)
    , WObject(*new WQuickSecurityContextManagerV1Private(this), nullptr)
{
    if (SECURITY_CONTEXT_MANAGER) {
        qFatal("There are multiple instances of WQuickSecurityContextManagerV1");
    }

    SECURITY_CONTEXT_MANAGER = this;
}

SecurityContextV1State WQuickSecurityContextManagerV1::lookSecurityContextState(const WObject *object)
{
    return lookSecurityContextState(object->waylandClient()->handle());
}

SecurityContextV1State WQuickSecurityContextManagerV1::lookSecurityContextState(const WClient *client)
{
    return lookSecurityContextState(client->handle());
}

SecurityContextV1State WQuickSecurityContextManagerV1::lookSecurityContextState(wl_client *client)
{
    W_D(WQuickSecurityContextManagerV1);
    const wlr_security_context_v1_state * wlr_state = QWLRoots::QWSecurityContextManagerV1::lookupClient(d->manager, client);
    struct SecurityContextV1State w_state;
    if (!wlr_state)
        return w_state;

    w_state = {
        .sandboxEngine = wlr_state->sandbox_engine,
        .appId = wlr_state->app_id,
        .instanceId = wlr_state->instance_id,
    };

    return w_state;
}

WServerInterface *WQuickSecurityContextManagerV1::create()
{
    W_D(WQuickSecurityContextManagerV1);

    d->manager = QWLRoots::QWSecurityContextManagerV1::create(server()->handle());
    return new WServerInterface(d->manager, d->manager->handle()->global);
}

WAYLIB_SERVER_END_NAMESPACE

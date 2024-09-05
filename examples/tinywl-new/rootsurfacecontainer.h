// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include "surfacecontainer.h"

#include <wglobal.h>

WAYLIB_SERVER_BEGIN_NAMESPACE
class WSurface;
class WToplevelSurface;
WAYLIB_SERVER_END_NAMESPACE

WAYLIB_SERVER_USE_NAMESPACE

class RootSurfaceContainer : public SurfaceContainer
{
    Q_OBJECT
public:
    explicit RootSurfaceContainer(QQuickItem *parent);

    enum ContainerZOrder {
        BackgroundZOrder = -2,
        BottomZOrder = -1,
        NormalZOrder = 0,
        TopZOrder = 1,
        OverlayZOrder = 2,
        TaskBarZOrder = 3,
    };

    SurfaceWrapper *getSurface(WSurface *surface) const;
    SurfaceWrapper *getSurface(WToplevelSurface *surface) const;

private:
    void addSurface(SurfaceWrapper *surface) override;
    void removeSurface(SurfaceWrapper *surface) override;

    void addBySubContainer(SurfaceContainer *, SurfaceWrapper *surface) override;
    void removeBySubContainer(SurfaceContainer *, SurfaceWrapper *surface) override;
};

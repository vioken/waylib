// Copyright (C) 2024 Yicheng Zhong <zhongyicheng@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "surfaceitemfactory.h"

#include <winputpopupsurfaceitem.h>
#include <wlayersurfaceitem.h>
#include <wxdgsurfaceitem.h>
#include <wxwaylandsurface.h>
#include <wxwaylandsurfaceitem.h>

#include <QQmlContext>
#include <qqmlproperty.h>

SurfaceItemFactory::SurfaceItemFactory(QQuickItem *parent)
    : QQuickItem(parent)
{
}

constexpr auto typePropName = "surfType";

void SurfaceItemFactory::classBegin()
{
    Q_ASSERT(qmlContext(this));
    Q_ASSERT(qmlContext(this)->contextProperty(typePropName).isValid());
    auto c = qmlContext(this);
    auto typeName = c->contextProperty(typePropName).toString();
    if (typeName == "toplevel" || typeName == "popup") {
        m_surfaceItem = new Waylib::Server::WXdgSurfaceItem(this);
    }
    if (typeName == "xwayland") {
        m_surfaceItem = new Waylib::Server::WXWaylandSurfaceItem(this);
    }
    if (typeName == "inputPopup") {
        m_surfaceItem = new Waylib::Server::WInputPopupSurfaceItem(this);
    }
    if (typeName == "layerShell") {
        m_surfaceItem = new Waylib::Server::WLayerSurfaceItem(this);
    }
}


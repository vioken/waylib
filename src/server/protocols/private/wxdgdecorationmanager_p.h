// Copyright (C) 2023 rewine <luhongxu@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <WServer>

#include <QObject>

WAYLIB_SERVER_BEGIN_NAMESPACE

class WSurface;
class WXdgDecorationManager;
class WXdgDecorationManagerPrivate;
class WAYLIB_SERVER_EXPORT WXdgDecorationManagerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool serverDecorationEnabled READ serverDecorationEnabled NOTIFY serverDecorationEnabledChanged FINAL)

public:
    WXdgDecorationManagerAttached(WSurface *target, WXdgDecorationManager *manager);

    bool serverDecorationEnabled() const;

Q_SIGNALS:
    void serverDecorationEnabledChanged();

private:
    WSurface *m_target;
    WXdgDecorationManager *m_manager;
};

WAYLIB_SERVER_END_NAMESPACE

// Copyright (C) 2024 YaoBing Xiao <xiaoyaobing@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wquickwaylandserver.h>

#include <QObject>
#include <QString>
#include <QQmlEngine>

WAYLIB_SERVER_BEGIN_NAMESPACE

struct SecurityContextV1State {
    Q_GADGET
    Q_PROPERTY(QString sandboxEngine MEMBER sandboxEngine)
    Q_PROPERTY(QString appId MEMBER appId)
    Q_PROPERTY(QString instanceId MEMBER instanceId)

public:
    QString sandboxEngine;
    QString appId;
    QString instanceId;
};

class WClient;

class WQuickSecurityContextManagerV1Private;
class WQuickSecurityContextManagerV1 : public WQuickWaylandServerInterface, public WObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SecurityContextManagerV1)
    W_DECLARE_PRIVATE(WQuickSecurityContextManagerV1)
public:

    explicit WQuickSecurityContextManagerV1(QObject *parent = nullptr);
    Q_INVOKABLE SecurityContextV1State lookSecurityContextState(const WObject *object);
    Q_INVOKABLE SecurityContextV1State lookSecurityContextState(const WClient *client);
    Q_INVOKABLE SecurityContextV1State lookSecurityContextState(wl_client *client);

private:
    WServerInterface *create() override;
};

WAYLIB_SERVER_END_NAMESPACE

Q_DECLARE_METATYPE(WAYLIB_SERVER_NAMESPACE::SecurityContextV1State)

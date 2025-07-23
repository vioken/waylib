// Copyright (C) 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include <WServer>
#include <WOutput>
#include <WSeat>
#include <QObject>
#include <QMap>
#include <QRegularExpression>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WInputDevice;

class WAYLIB_SERVER_EXPORT WSeatManager : public QObject, public WServerInterface
{
    Q_OBJECT

public:
    explicit WSeatManager(QObject *parent = nullptr);
    ~WSeatManager();

    // Seat management
    WSeat *createSeat(const QString &name, bool isFallback = false);
    void removeSeat(const QString &name);
    void removeSeat(WSeat *seat);
    WSeat *getSeat(const QString &name) const;
    QList<WSeat*> seats() const;
    WSeat *fallbackSeat() const;
    
    // Device assignment
    void assignDeviceToSeat(WInputDevice *device, const QString &seatName);
    bool autoAssignDevice(WInputDevice *device);
    
    // Output device association
    void assignOutputToSeat(WOutput *output, const QString &seatName);
    
    // Configuration management
    void loadConfig(const QJsonObject &config);
    QJsonObject saveConfig() const;
    
    // Device matching rules
    void addDeviceRule(const QString &seatName, const QString &rule);
    void removeDeviceRule(const QString &seatName, const QString &rule);
    QStringList deviceRules(const QString &seatName) const;

    // Device auto-assignment
    bool deviceMatchesSeat(WInputDevice *device, WSeat *seat) const;

    // Get seat matching device
    WSeat *findSeatForDevice(WInputDevice *device) const;
    
    // Get seat matching output device
    WSeat *findSeatForOutput(WOutput *output) const;

protected:
    void create(WServer *server) override;
    void destroy(WServer *server) override;
    wl_global *global() const override;
    QByteArrayView interfaceName() const override;
    
private:
    QMap<QString, WSeat*> m_seats;
    QMap<QString, QList<QRegularExpression>> m_deviceRules;
    QString m_fallbackSeatName;
};

WAYLIB_SERVER_END_NAMESPACE

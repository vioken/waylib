// Copyright (C) 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "wseatmanager.h"
#include "wseat.h"
#include "winputdevice.h"
#include "woutput.h"

#include <qwinputdevice.h>

#include <QJsonObject>
#include <QJsonArray>

WAYLIB_SERVER_BEGIN_NAMESPACE

WSeatManager::WSeatManager(QObject *parent)
    : QObject(parent)
{
}

WSeatManager::~WSeatManager()
{
    QList<WSeat*> seatsToDelete = m_seats.values();
    m_seats.clear();
    m_deviceRules.clear();
    
    for (WSeat* seat : seatsToDelete) {
        delete seat;
    }
}

WSeat *WSeatManager::createSeat(const QString &name, bool isFallback)
{
    if (m_seats.contains(name))
        return m_seats[name];
        
    WSeat *seat = new WSeat(name);
    seat->setIsFallback(isFallback);
    m_seats[name] = seat;
    
    if (isFallback) {
        m_fallbackSeatName = name;
        for (auto it = m_seats.begin(); it != m_seats.end(); ++it) {
            if (it.key() != name && it.value()->isFallback()) {
                it.value()->setIsFallback(false);
            }
        }
    }
    
    return seat;
}

void WSeatManager::removeSeat(const QString &name)
{
    if (m_seats.contains(name)) {
        WSeat *seat = m_seats.take(name);
        
        if (seat->isFallback() && !m_seats.isEmpty()) {
            auto it = m_seats.begin();
            it.value()->setIsFallback(true);
            m_fallbackSeatName = it.key();
        }
        
        QList<WInputDevice*> devices = seat->deviceList();
        for (auto device : devices) {
            seat->detachInputDevice(device);
            
            if (!m_seats.isEmpty()) {
                autoAssignDevice(device);
            }
        }
        
        QList<WOutput*> outputs = seat->outputs();
        for (auto output : outputs) {
            seat->detachOutput(output);
            
            if (!m_seats.isEmpty() && fallbackSeat()) {
                fallbackSeat()->attachOutput(output);
            }
        }
        
        delete seat;
    }
}

void WSeatManager::removeSeat(WSeat *seat)
{
    if (!seat)
        return;
        
    QString seatName;
    for (auto it = m_seats.begin(); it != m_seats.end(); ++it) {
        if (it.value() == seat) {
            seatName = it.key();
            break;
        }
    }
    
    if (!seatName.isEmpty()) {
        removeSeat(seatName);
    } else {
        qWarning() << "Attempted to remove a seat that is not managed by WSeatManager";
    }
}

WSeat *WSeatManager::getSeat(const QString &name) const
{
    return m_seats.value(name);
}

QList<WSeat*> WSeatManager::seats() const
{
    return m_seats.values();
}

WSeat *WSeatManager::fallbackSeat() const
{
    return m_seats.value(m_fallbackSeatName);
}

void WSeatManager::assignDeviceToSeat(WInputDevice *device, const QString &seatName)
{
    if (!device) {
        qWarning() << "Cannot assign null device to seat";
        return;
    }
    
    for (auto seat : m_seats) {
        if (seat->deviceList().contains(device)) {
            if (seat->name() == seatName) {
                return;
            }
            
            seat->detachInputDevice(device);
            break;
        }
    }
    
    if (m_seats.contains(seatName)) {
        m_seats[seatName]->attachInputDevice(device);
    } else if (fallbackSeat()) {
        fallbackSeat()->attachInputDevice(device);
    }
}

bool WSeatManager::autoAssignDevice(WInputDevice *device)
{
    if (!device) {
        qWarning() << "Cannot auto-assign null device";
        return false;
    }
    
    for (auto seat : m_seats) {
        if (seat->deviceList().contains(device)) {
            return true;
        }
    }
    
    WSeat *targetSeat = findSeatForDevice(device);
    
    if (targetSeat) {
        targetSeat->attachInputDevice(device);
        return true;
    } else if (fallbackSeat()) {
        fallbackSeat()->attachInputDevice(device);
        return true;
    }
    
    return false;
}

void WSeatManager::assignOutputToSeat(WOutput *output, const QString &seatName)
{
    if (!output) {
        qWarning() << "Cannot assign null output to seat";
        return;
    }
    
    for (auto seat : m_seats) {
        if (seat->outputs().contains(output)) {
            if (seat->name() == seatName) {
                return;
            }
            
            seat->detachOutput(output);
            break;
        }
    }
    
    if (m_seats.contains(seatName)) {
        m_seats[seatName]->attachOutput(output);
    } else if (fallbackSeat()) {
        fallbackSeat()->attachOutput(output);
    }
}

void WSeatManager::addDeviceRule(const QString &seatName, const QString &rule)
{
    if (seatName.isEmpty()) {
        qWarning() << "Cannot add device rule for seat with empty name";
        return;
    }
    
    if (rule.isEmpty()) {
        qWarning() << "Cannot add empty device rule";
        return;
    }
    
    if (!m_seats.contains(seatName)) {
        qWarning() << "Cannot add device rule for non-existent seat:" << seatName;
        return;
    }
    
    QRegularExpression regex(rule);
    if (!regex.isValid()) {
        qWarning() << "Invalid regex pattern for device rule:" << rule << "Error:" << regex.errorString();
        return;
    }
    
    if (!m_deviceRules.contains(seatName)) {
        m_deviceRules[seatName] = QList<QRegularExpression>();
    }
    
        m_deviceRules[seatName].append(regex);
}

void WSeatManager::removeDeviceRule(const QString &seatName, const QString &rule)
{
    if (!m_deviceRules.contains(seatName))
        return;
        
    QRegularExpression regex(rule);
    if (!regex.isValid())
        return;
        
        auto &rules = m_deviceRules[seatName];
    for (int i = 0; i < rules.size(); i++) {
        if (rules[i].pattern() == regex.pattern()) {
                rules.removeAt(i);
                break;
        }
    }
    
    if (rules.isEmpty())
        m_deviceRules.remove(seatName);
}

QStringList WSeatManager::deviceRules(const QString &seatName) const
{
    if (!m_deviceRules.contains(seatName))
        return QStringList();
        
    QStringList result;
    for (const auto &regex : m_deviceRules[seatName]) {
        result.append(regex.pattern());
    }
    
    return result;
}

void WSeatManager::loadConfig(const QJsonObject &config)
{
    QList<WSeat*> seatsToDelete = m_seats.values();
    m_seats.clear();
    m_deviceRules.clear();
    
    for (WSeat* seat : seatsToDelete) {
        delete seat;
    }
    
    QJsonArray seatsArray = config["seats"].toArray();
    for (const auto &seatValue : seatsArray) {
        QJsonObject seatObj = seatValue.toObject();
        QString name = seatObj["name"].toString();
        bool isFallback = seatObj["fallback"].toBool();
        
        WSeat *seat = createSeat(name, isFallback);
        
        QJsonArray rulesArray = seatObj["deviceRules"].toArray();
        for (const auto &ruleValue : rulesArray) {
            QString rule = ruleValue.toString();
            addDeviceRule(name, rule);
        }
    }
    
    if (m_seats.isEmpty()) {
        createSeat("seat0", true);
    }
    
    if (!fallbackSeat() && !m_seats.isEmpty()) {
        auto it = m_seats.begin();
        it.value()->setIsFallback(true);
        m_fallbackSeatName = it.key();
    }
}

QJsonObject WSeatManager::saveConfig() const
{
    QJsonObject config;
    QJsonArray seatsArray;
    
    for (auto it = m_seats.begin(); it != m_seats.end(); ++it) {
        QString name = it.key();
        WSeat *seat = it.value();
        
        QJsonObject seatObj;
        seatObj["name"] = name;
        seatObj["fallback"] = seat->isFallback();
        
        QJsonArray rulesArray;
        for (const auto &rule : deviceRules(name)) {
            rulesArray.append(rule);
        }
        seatObj["deviceRules"] = rulesArray;
        
        QJsonArray outputsArray;
        for (auto output : seat->outputs()) {
            outputsArray.append(output->name());
        }
        seatObj["outputs"] = outputsArray;
        
        seatsArray.append(seatObj);
    }
    
    config["seats"] = seatsArray;
    return config;
}

void WSeatManager::create(WServer *server)
{
    if (!server) {
        qCritical() << "Cannot create seats with null server";
        return;
    }

    for (auto it = m_seats.begin(); it != m_seats.end(); ++it) {
        WSeat *seat = it.value();

        server->attach(seat);

        if (!seat->nativeHandle()) {
            qCritical() << "Failed to create native handle for seat" << seat->name();
        }
    }

    if (!fallbackSeat() && !m_seats.isEmpty()) {
        auto it = m_seats.begin();
        it.value()->setIsFallback(true);
        m_fallbackSeatName = it.key();
    }
}

void WSeatManager::destroy(WServer *server)
{
    Q_UNUSED(server);
    m_seats.clear();
    m_deviceRules.clear();
    m_fallbackSeatName.clear();
}

bool WSeatManager::deviceMatchesSeat(WInputDevice *device, WSeat *seat) const
{
    if (!device || !seat)
        return false;
        
    QString seatName = seat->name();
    
    if (!m_deviceRules.contains(seatName)) {
        return false;
    }

    bool matches = seat->matchesDevice(device, m_deviceRules[seatName]);
    return matches;
}

WSeat *WSeatManager::findSeatForDevice(WInputDevice *device) const
{
    if (!device)
        return nullptr;
        
    for (auto seat : m_seats) {
        if (seat->deviceList().contains(device)) {
            return seat;
        }
    }

    for (auto it = m_seats.begin(); it != m_seats.end(); ++it) {
        WSeat *seat = it.value();
        if (seat->isFallback()) {
            continue;
        }
        if (deviceMatchesSeat(device, seat)) {
            return seat;
        }
    }
    
    WSeat *fallbackSeat = this->fallbackSeat();
    if (fallbackSeat) {
        bool hasRules = m_deviceRules.contains(fallbackSeat->name()) &&
                       !m_deviceRules[fallbackSeat->name()].isEmpty();

        if (hasRules) {
            if (deviceMatchesSeat(device, fallbackSeat)) {
                return fallbackSeat;
            }
        } else {
            return fallbackSeat;
        }
    }
    return fallbackSeat;
}

WSeat *WSeatManager::findSeatForOutput(WOutput *output) const
{
    if (!output)
        return nullptr;
        
    for (auto seat : m_seats) {
        if (seat->outputs().contains(output)) {
            return seat;
        }
    }
    
    return fallbackSeat();
}

wl_global *WSeatManager::global() const
{
    return nullptr;
}

QByteArrayView WSeatManager::interfaceName() const
{
    return "wseatmanager";
}

WAYLIB_SERVER_END_NAMESPACE

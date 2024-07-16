// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <QObject>
#include <qwglobal.h>

#include <libinput.h>

QW_BEGIN_NAMESPACE
class QWInputDevice;
QW_END_NAMESPACE

QT_BEGIN_NAMESPACE
class QInputDevice;
QT_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WSeat;
class WInputDevicePrivate;
class WInputDevice : public WWrapObject
{
    W_DECLARE_PRIVATE(WInputDevice)
public:
    enum class Type {
        Unknow,
        Keyboard,
        Pointer,
        Touch,
        Tablet,
        TabletPad,
        Switch
    };
    Q_ENUM(Type)

    WInputDevice(QW_NAMESPACE::QWInputDevice *handle);

    QW_NAMESPACE::QWInputDevice *handle() const;

    static WInputDevice *fromHandle(const QW_NAMESPACE::QWInputDevice *handle);

    template<class QInputDevice>
    inline QInputDevice *qtDevice() const {
        return qobject_cast<QInputDevice*>(qtDevice());
    }
    QInputDevice *qtDevice() const;
    static WInputDevice *from(const QInputDevice *device);

    Type type() const;
    void setSeat(WSeat *seat);
    WSeat *seat() const;

    libinput_device *libinput_device_handle();

    bool setSendEventsMode(uint32_t mode);
    bool setTapEnabled(enum libinput_config_tap_state tap);
    bool setTapButtonMap(enum libinput_config_tap_button_map map);
    bool setTapDragEnabled(enum libinput_config_drag_state drag);
    bool setTapDragLock(enum libinput_config_drag_lock_state lock);
    bool setAccelSpeed(qreal speed);
    bool setRotationAngle(qreal angle);
    bool setAccelProfile(enum libinput_config_accel_profile profile);
    bool setNaturalScroll(bool natural);
    bool setLeftHanded(bool left);
    bool setClickMethod(enum libinput_config_click_method method);
    bool setMiddleEmulation(enum libinput_config_middle_emulation_state mid);
    bool setScrollMethod(enum libinput_config_scroll_method method);
    bool setScrollButton(uint32_t button);
    bool setScrollButtonLock(enum libinput_config_scroll_button_lock_state lock);
    bool setDwt(enum libinput_config_dwt_state enable);
    bool setDwtp(enum libinput_config_dwtp_state enable);
    bool setCalibrationMatrix(float mat[6]);

private:
    friend class QWlrootsIntegration;
    void setQtDevice(QInputDevice *device);
};

WAYLIB_SERVER_END_NAMESPACE

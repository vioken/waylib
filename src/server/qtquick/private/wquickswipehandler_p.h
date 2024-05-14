// Copyright (C) 2023 tjd <tanjingdong@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <private/qquickmultipointhandler_p.h>


WAYLIB_SERVER_BEGIN_NAMESPACE


class WAYLIB_SERVER_EXPORT WQuickSwipeHandler : public QQuickMultiPointHandler
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SwipeHandler)

public:
    explicit WQuickSwipeHandler(QQuickItem *parent = nullptr);

protected:
    bool wantsPointerEvent(QPointerEvent *event) override;
    void onActiveChanged() override;
    void handlePointerEventImpl(QPointerEvent *event) override;
    void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point) override;

private:
    QPointF targetCentroidPosition();

private:
    QPointF m_pressTargetPos;
    bool m_pressedInsideTarget = false;
};


WAYLIB_SERVER_END_NAMESPACE

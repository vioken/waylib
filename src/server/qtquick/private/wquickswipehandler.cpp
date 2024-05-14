// Copyright (C) 2023 tjd <tanjingdong@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquickswipehandler_p.h"
#include <private/qquickwindow_p.h>
#include <private/qquickmultipointhandler_p_p.h>
#include <private/qquickpointerdevicehandler_p_p.h>
#include <private/qquickhandlerpoint_p.h>
#include <private/qquickpointerhandler_p_p.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

static const qreal DragAngleToleranceDegrees = 10;

WQuickSwipeHandler::WQuickSwipeHandler(QQuickItem *parent)
    : QQuickMultiPointHandler(parent, 3, 3)
{
}

void WQuickSwipeHandler::onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point)
{
    QQuickMultiPointHandler::onGrabChanged(grabber, transition, event, point);
    if (grabber == this && transition == QPointingDevice::GrabExclusive && target()) {
        if (m_pressTargetPos.isNull()) {
            m_pressTargetPos = targetCentroidPosition();
        }
    }
}

void WQuickSwipeHandler::onActiveChanged()
{
    QQuickMultiPointHandler::onActiveChanged();
    if (active()) {
        if (auto parent = parentItem()) {
            if (QQuickDeliveryAgentPrivate::isTouchEvent(currentEvent()))
                parent->setKeepTouchGrab(true);
            parent->setKeepMouseGrab(true);
        }
    } else {
        m_pressTargetPos = QPointF();
        m_pressedInsideTarget = false;
        if (auto parent = parentItem()) {
            parent->setKeepTouchGrab(false);
            parent->setKeepMouseGrab(false);
        }
    }
}

bool WQuickSwipeHandler::wantsPointerEvent(QPointerEvent *event)
{
    if (!QQuickMultiPointHandler::wantsPointerEvent(event))
        if (!active())
            return false;

#if QT_CONFIG(gestures)
    if (event->type() == QEvent::NativeGesture)
       return false;
#endif

    return true;
}

void WQuickSwipeHandler::handlePointerEventImpl(QPointerEvent *event)
{
    if (active() && !QQuickMultiPointHandler::wantsPointerEvent(event))
        return;

    QQuickMultiPointHandler::handlePointerEventImpl(event);
    event->setAccepted(true);

    if (!active()) {
        qreal minAngle =  361;
        qreal maxAngle = -361;
        bool allOverThreshold = !event->isEndEvent();
        QVector<QEventPoint> chosenPoints;

        if (event->isBeginEvent())
            m_pressedInsideTarget = target() && currentPoints().size() > 0;

        for (const QQuickHandlerPoint &p : currentPoints()) {
            if (!allOverThreshold)
                break;

            auto point = event->pointById(p.id());
            chosenPoints << *point;
            setPassiveGrab(event, *point);
            
            // Calculate drag delta
            QVector2D accumulatedDragDelta = QVector2D(point->scenePosition() - point->scenePressPosition());
            bool overThreshold = d_func()->dragOverThreshold(accumulatedDragDelta);
            if (allOverThreshold && !overThreshold)
                allOverThreshold = false;

            // Check that all points have been dragged in approximately the same direction
            qreal angle = std::atan2(accumulatedDragDelta.y(), accumulatedDragDelta.x()) * 180 / M_PI;
            minAngle = qMin(angle, minAngle);
            maxAngle = qMax(angle, maxAngle);

            if (event->isBeginEvent()) {
                if (target()) {
                    const QPointF localPressPos = target()->mapFromScene(point->scenePressPosition());
                    m_pressedInsideTarget &= target()->contains(localPressPos);
                    m_pressTargetPos = targetCentroidPosition();
                }

                point->setAccepted(QQuickDeliveryAgentPrivate::isMouseEvent(event));
            }
        }

        if (allOverThreshold) {
            qreal angleDiff = maxAngle - minAngle;
            if (angleDiff > 180)
                angleDiff = 360 - angleDiff;
            if (angleDiff < DragAngleToleranceDegrees && grabPoints(event, chosenPoints))
                setActive(true);
        }
    }

    if (active() && target() && target()->parentItem()) {
        const QPointF newTargetTopLeft = targetCentroidPosition() - m_pressTargetPos;
        const QPointF xformOrigin = target()->transformOriginPoint();
        const QPointF targetXformOrigin = newTargetTopLeft + xformOrigin;
        QPointF pos = target()->parentItem()->mapFromItem(target(), targetXformOrigin);
        pos -= xformOrigin;
        moveTarget(pos);
    }
}

QPointF WQuickSwipeHandler::targetCentroidPosition()
{
    QPointF pos = centroid().position();
    if (auto par = parentItem()) {
        if (target() != par)
            pos = par->mapToItem(target(), pos);
    }
    return pos;
}


WAYLIB_SERVER_END_NAMESPACE
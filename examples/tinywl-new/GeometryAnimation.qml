// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Tinywl

Item {
    id: root

    required property SurfaceWrapper surface
    property int duration: 200
    property rect fromGeometry
    property rect toGeometry

    signal started
    signal finished

    function start(targetGeometry) {
        fromGeometry = surface.geometry;
        toGeometry = targetGeometry;
        animation.start();
    }

    ShaderEffectSource {
        id: frontEffect

        readonly property real xScale: root.width / fromGeometry.width
        readonly property real yScale: root.height / fromGeometry.height

        live: false
        sourceItem: surface
        width: sourceRect.width * xScale
        height: sourceRect.height * yScale
        x: sourceRect.x * xScale
        y: sourceRect.y * yScale

        onScheduledUpdateCompleted: {
            root.started();
        }

        Component.onCompleted: {
            sourceRect = surface.boundingRect
        }
    }

    ShaderEffectSource {
        id: backgroundEffect

        readonly property real xScale: root.width / toGeometry.width
        readonly property real yScale: root.height / toGeometry.height

        live: true
        sourceItem: surface
        hideSource: true
        sourceRect: surface.boundingRect
        width: sourceRect.width * xScale
        height: sourceRect.height * yScale
        x: sourceRect.x * xScale
        y: sourceRect.y * yScale
    }

    ParallelAnimation {
        id: animation

        XAnimator {
            target: root
            duration: root.duration
            easing.type: Easing.OutCubic
            from: root.fromGeometry.x
            to: root.toGeometry.x
        }

        YAnimator {
            target: root
            duration: root.duration
            easing.type: Easing.OutCubic
            from: root.fromGeometry.y
            to: root.toGeometry.y
        }

        PropertyAnimation {
            target: root
            property: "width"
            duration: root.duration
            easing.type: Easing.OutCubic
            from: root.fromGeometry.width
            to: root.toGeometry.width
        }

        PropertyAnimation {
            target: root
            property: "height"
            duration: root.duration
            easing.type: Easing.OutCubic
            from: root.fromGeometry.height
            to: root.toGeometry.height
        }

        OpacityAnimator {
            target: frontEffect
            duration: root.duration
            easing.type: Easing.InCubic
            from: 1.0
            to: 0.0
        }

        OpacityAnimator {
            target: backgroundEffect
            duration: root.duration
            easing.type: Easing.OutCubic
            from: 0.5
            to: 1.0
        }

        onFinished: {
            root.finished();
        }
    }
}

// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Waylib.Server
import Tinywl

ListView {
    id: root
    required property QtObject output
    readonly property OutputItem outputItem: output.outputItem
    property int duration: 1000

    width: 250 + leftMargin + rightMargin
    height: Math.min(outputItem.height, contentHeight) + topMargin + bottomMargin
    x: outputItem.x
    y: (outputItem.height - height) / 2
    spacing: 80
    leftMargin: 40
    rightMargin: 40
    topMargin: 40
    bottomMargin: 40
    model: output.minimizedSurfaces
    delegate: Item {
        required property SurfaceWrapper surface

        width: proxy.width
        height: proxy.height

        SurfaceProxy {
            id: proxy
            opacity: 0

            live: true
            surface: parent.surface
            transformOrigin : Item.TopLeft
            maxSize: Qt.size(250, 150)
        }

        transform: Rotation {
            angle: 30
            axis {
                x: 0
                y: 1
                z: 0
            }

            origin {
                x: width / 2
                y: height / 2
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked:  {
                proxy.opacity = 0;
                animationLoader.active = true

                animationLoader.item.surface = proxy.surface
                animationLoader.item.startAnimation(false)
            }
        }

        Loader {
            id: animationLoader
            active: true
            sourceComponent: fakeAnimationComponent

            Connections {
                target: animationLoader.item

                function onFinished(direction) {
                    animationLoader.active = false
                    proxy.opacity = direction ? 1 : 0

                    if (!direction)
                        surface.requestCancelMinimize()
                }
            }
        }

        Component.onCompleted: {
            animationLoader.item.surface = proxy.surface
            animationLoader.item.startAnimation(true)
        }
    }

    Component {
        id: fakeAnimationComponent

        Item {
            id: fakeAnimation
            property SurfaceWrapper surface
            property bool forward: true

            signal finished(bool direction)

            width: origin.width
            height: origin.height

            SurfaceProxy {
                id: origin

                live: true
                surface: parent.surface
                transformOrigin: Item.TopLeft
            }

            ParallelAnimation {
                id: animation

                XAnimator {
                    target: origin
                    duration: root.duration
                    from: fakeAnimation.forward ? root.mapFromGlobal(fakeAnimation.surface.mapToGlobal(Qt.point(0, 0))).x : origin.x
                    to: fakeAnimation.forward ? origin.x : root.mapFromGlobal(fakeAnimation.surface.mapToGlobal(Qt.point(0, 0))).x
                }

                YAnimator {
                    target: origin
                    duration: root.duration
                    from: fakeAnimation.forward ? root.mapFromGlobal(fakeAnimation.surface.mapToGlobal(Qt.point(0, 0))).y : origin.y
                    to: fakeAnimation.forward ? origin.y : root.mapFromGlobal(fakeAnimation.surface.mapToGlobal(Qt.point(0, 0))).y
                }

                ScaleAnimator {
                    target: origin
                    duration: root.duration
                    from: fakeAnimation.forward ? 1.0 : Math.min(250 / fakeAnimation.surface.width,
                                                                 150 / fakeAnimation.surface.height)
                    to: fakeAnimation.forward ? Math.min(250 / fakeAnimation.surface.width,
                                                         150 / fakeAnimation.surface.height) : 1.0
                }

                onFinished: fakeAnimation.finished(fakeAnimation.forward)
            }

            function startAnimation(forward) {
                fakeAnimation.forward = forward
                animation.start()
            }
        }
    }
}

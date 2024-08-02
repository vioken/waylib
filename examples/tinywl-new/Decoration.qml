// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Effects
import Waylib.Server
import Tinywl

Item {
    id: root

    required property SurfaceWrapper surface
    readonly property SurfaceItem surfaceItem: surface.surfaceItem

    x: -shadow.blurMax
    y: -shadow.blurMax
    width: surface.implicitWidth + 2 * shadow.blurMax
    height: surface.implicitHeight + 2 * shadow.blurMax

    Rectangle {
        id: shadowSource
        width: surface.implicitWidth
        height: surface.implicitHeight
        color: shadow.shadowColor
        radius: surface.radius
        layer {
            enabled: true
            live: true
        }
        visible: false
    }

    MultiEffect {
        id: shadow
        x: blurMax
        y: blurMax
        width: shadowSource.width
        height: shadowSource.height
        shadowEnabled: root.visible
        shadowColor: "black"
        shadowBlur: 1.0
        blurMax: 64
        shadowVerticalOffset: 10
        autoPaddingEnabled: true
        maskEnabled: true
        maskSource: shadowSource
        maskInverted: true
        source: shadowSource
    }

    Loader {
        active: surface.radius > 0 && root.visible
        parent: surface.titleBar.parent
        x: surface.titleBar.x
        y: surface.titleBar.y
        width: surface.titleBar.width
        height: surface.titleBar.height

        sourceComponent: MultiEffect {
            width: surface.titleBar.width
            height: surface.titleBar.height
            source: surface.titleBar
            maskEnabled: true
            maskSource: ShaderEffectSource {
                hideSource: true
                sourceItem: Item {
                    width: surface.titleBar.width
                    height: surface.titleBar.height
                    Rectangle {
                        anchors {
                            fill: parent
                            bottomMargin: -radius
                        }
                        radius: surface.radius
                        antialiasing: true
                    }
                }
            }

            Component.onCompleted: {
                surface.titleBar.opacity = 0;
            }
            Component.onDestruction: {
                surface.titleBar.opacity = 1;
            }
        }
    }

    Component {
        id: surfaceContentComponent

        Item {
            required property SurfaceItem surface
            readonly property SurfaceWrapper wrapper: surface.parent
            readonly property real cornerRadius: wrapper.radius

            anchors.fill: parent
            SurfaceItemContent {
                id: content
                surface: parent.surface.surface
                anchors.fill: parent
                opacity: effectLoader.active ? 0 : 1
            }

            Loader {
                id: effectLoader
                active: cornerRadius > 0 && (wrapper.decoration?.visible ?? false)
                anchors.fill: parent
                sourceComponent: MultiEffect {
                    anchors.fill: parent
                    source: content
                    maskSource: ShaderEffectSource {
                        width: content.width
                        height: content.height
                        samples: 4
                        sourceItem: Item {
                            width: content.width
                            height: content.height
                            Rectangle {
                                anchors {
                                    fill: parent
                                    topMargin: -radius
                                }
                                radius: Math.min(cornerRadius, content.width / 2, content.height)
                                antialiasing: true
                            }
                        }
                    }

                    maskEnabled: true
                }
            }
        }
    }

    Component.onCompleted: {
        surfaceItem.delegate = surfaceContentComponent;
    }

    Rectangle {
        id: outsideBorder
        visible: root.visible
        parent: surfaceItem
        x: -border.width
        y: -border.width + surface.titleBar.y
        width: surface.implicitWidth + 2 * border.width
        height: surface.implicitHeight + 2 * border.width
        z: surface.titleBar.z + 1
        color: "transparent"
        border {
            color: "yellow"
            width: 1
        }
        radius: surface.radius + border.width
    }

    Rectangle {
        id: insideBorder
        visible: root.visible
        parent: surfaceItem
        y: surface.titleBar.y
        width: surface.implicitWidth
        height: surface.implicitHeight
        z: surface.titleBar.z + 1
        color: "transparent"
        border {
            color: "green"
            width: 1
        }
        radius: surface.radius
    }
}

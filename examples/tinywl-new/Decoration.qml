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

    visible: surface && surface.visibleDecoration
    x: -shadow.blurMax
    y: -shadow.blurMax
    width: surface.implicitWidth + 2 * shadow.blurMax
    height: surface.implicitHeight + 2 * shadow.blurMax

    MouseArea {
        property int edges: 0

        anchors {
            fill: shadow
            margins: -10
        }

        hoverEnabled: true
        Cursor.shape: {
            switch(edges) {
            case Qt.TopEdge:
                return Waylib.CursorShape.TopSide
            case Qt.RightEdge:
                return Waylib.CursorShape.RightSide
            case Qt.BottomEdge:
                return Waylib.CursorShape.BottomSide
            case Qt.LeftEdge:
                return Waylib.CursorShape.LeftSide
            case Qt.TopEdge | Qt.LeftEdge:
                return Waylib.CursorShape.TopLeftCorner
            case Qt.TopEdge | Qt.RightEdge:
                return Waylib.CursorShape.TopRightCorner
            case Qt.BottomEdge | Qt.LeftEdge:
                return Waylib.CursorShape.BottomLeftCorner
            case Qt.BottomEdge | Qt.RightEdge:
                return Waylib.CursorShape.BottomRightCorner
            }

            return Qt.ArrowCursor;
        }

        onPositionChanged: function (event) {
            edges = WaylibHelper.getEdges(Qt.rect(0, 0, width, height), Qt.point(event.x, event.y), 10)
        }

        onPressed: function (event) {
            // Maybe missing onPositionChanged when use touchscreen
            edges = WaylibHelper.getEdges(Qt.rect(0, 0, width, height), Qt.point(event.x, event.y), 10)
            if (edges)
                surface.requestResize(edges)
        }
    }

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
        shadowEnabled: surface.visibleDecoration
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
        active: surface.radius > 0 && surface.visibleDecoration && surface.titleBar
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
                if (surface.titleBar)
                    surface.titleBar.opacity = 1;
            }
        }
    }

    Component {
        id: surfaceContentComponent

        Item {
            required property SurfaceItem surface
            readonly property SurfaceWrapper wrapper: surface?.parent ?? null
            readonly property real cornerRadius: wrapper?.radius ?? cornerRadius

            anchors.fill: parent
            SurfaceItemContent {
                id: content
                surface: wrapper?.surface ?? null
                anchors.fill: parent
                opacity: effectLoader.active ? 0 : 1
            }

            Loader {
                id: effectLoader
                active: cornerRadius > 0 && (wrapper?.visibleDecoration ?? false)
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
        visible: surface.visibleDecoration
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
        visible: surface.visibleDecoration
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

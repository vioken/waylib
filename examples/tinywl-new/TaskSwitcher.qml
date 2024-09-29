// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Waylib.Server
import Tinywl

Rectangle {
    required property QtObject output
    readonly property OutputItem outputItem: output.outputItem

    width: outputItem.width
    height: outputItem.height
    color: "black"
    opacity: 0.8

    MouseArea {
        anchors.fill: parent
    }

    SurfaceProxy {
        id: activeSurface
        live: true
        x: surface.x
        y: surface.y
        surface: Helper.activatedSurface
    }

    ListView {
        id: windowsView
        x: (outputItem.width - width) / 2
        y: outputItem.height - height
        width: Math.min(outputItem.width, contentWidth) + leftMargin + rightMargin
        height: 210 + topMargin + bottomMargin
        spacing: 20
        leftMargin: 40
        rightMargin: 40
        topMargin: 40
        bottomMargin: 40
        orientation: ListView.Horizontal
        model: Helper.workspace.current.model
        delegate: Rectangle {
            id: windowItem

            required property SurfaceWrapper surface
            radius: 10

            width: proxy.width + 4
            height: proxy.height + 4
            border.width: 2

            SurfaceProxy {
                id: proxy
                live: true
                anchors.centerIn: parent
                surface: parent.surface
                maxSize: Qt.size(280, 210)
            }

            states: [
                State {
                    name: "highlighted"
                    when: Helper.activatedSurface === surface
                    PropertyChanges {
                        target: windowItem
                        border.color: "blue"
                    }
                },
                State {
                    name: "normal"
                    when: Helper.activatedSurface !== surface
                    PropertyChanges {
                        target: windowItem
                        border.color: "transparent"
                    }
                }
            ]

            transitions: Transition {
                from: "normal"
                to: "highlighted"

                ColorAnimation {
                    properties: "border.color"
                    duration: 200
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: Helper.activeSurface(surface, Qt.ActiveWindowFocusReason)
            }
        }
    }

    function previous() {
        var previousIndex = (windowsView.currentIndex - 1 + windowsView.count) % windowsView.count
        windowsView.currentIndex = previousIndex
        Helper.activeSurface(windowsView.currentItem.surface, Qt.BacktabFocusReason)
    }

    function next() {
        var nextIndex = (windowsView.currentIndex + 1) % windowsView.count
        windowsView.currentIndex = nextIndex
        Helper.activeSurface(windowsView.currentItem.surface, Qt.TabFocusReason)
    }
}

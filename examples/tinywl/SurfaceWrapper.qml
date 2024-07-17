// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Waylib.Server
import Tinywl

SurfaceItemFactory {
    id: root

    readonly property XWaylandSurfaceItem asXwayland: {
        surfaceItem as XWaylandSurfaceItem;
    }
    readonly property XdgSurfaceItem asXdg: {
        surfaceItem as XdgSurfaceItem;
    }
    readonly property InputPopupSurfaceItem asInputPopup: {
        surfaceItem as InputPopupSurfaceItem;
    }
    readonly property LayerSurfaceItem asLayerShell: {
        surfaceItem as LayerSurfaceItem;
    }

    // from helper surface
    required property ToplevelSurface wSurface
    required property string type
    required property DynamicCreatorComponent creator
    required property ListModel dockModel
    property int outputCounter: 0

    property alias decoration : decoration

    function doDestroy() {
        if (helper.status == Loader.Ready)
            helper.item.doDestroy();
    }

    Component.onCompleted: {
        console.debug(type, "completed", surfaceItem);
    }

    surfaceItem {

        shellSurface: wSurface
        topPadding: decoration.visible ? decoration.topMargin : 0
        bottomPadding: decoration.visible ? decoration.bottomMargin : 0
        leftPadding: decoration.visible ? decoration.leftMargin : 0
        rightPadding: decoration.visible ? decoration.rightMargin : 0
        focus: wSurface === Helper.activatedSurface

        states: [
            State {
                name: "maximize"
                when: helper.status == Loader.Ready && helper.item.isMaximize
                PropertyChanges {
                    restoreEntryValues: true
                    target: root.surfaceItem
                    x: helper.item.getMaximizeX()
                    y: helper.item.getMaximizeY()
                    width: helper.item.getMaximizeWidth()
                    height: helper.item.getMaximizeHeight()
                }
            },
            State {
                name: "fullscreen"
                when: helper.status == Loader.Ready && helper.item.isFullScreen
                PropertyChanges {
                    restoreEntryValues: true
                    target: root.surfaceItem
                    x: helper.item.getFullscreenX()
                    y: helper.item.getFullscreenY()
                    z: 100 + 1 // LayerType.Overlay + 1
                    width: helper.item.getFullscreenWidth()
                    height: helper.sourceComponent.getFullscreenHeight()
                }
            }
        ]
    }

    WindowDecoration {
        id: decoration
        enable: false
        parent: root.surfaceItem
        anchors.fill: parent
        z: SurfaceItem.ZOrder.ContentItem - 1
        surface: root.surfaceItem.shellSurface

        Binding {
            target: decoration
            property: "visible"
            value: decoration.enable
        }

        Connections {
            target: Helper.xdgDecorationManager

            function onSurfaceModeChanged(surface, mode) {
                if (root.type === "toplevel" && wSurface === surface)
                    enable = (mode !== XdgDecorationManager.Client);
            }
        }

        Component.onCompleted: {
            if (root.type === "toplevel")
            enable = Helper.xdgDecorationManager.modeBySurface(wSurface.surface) !== XdgDecorationManager.Client;
        }
    }

    OutputLayoutItem {
        parent: surfaceItem
        anchors.fill: parent
        layout: Helper.outputLayout

        onEnterOutput: function (output) {
            if (root.wSurface.surface)
                wSurface.surface.enterOutput(output);
            Helper.onSurfaceEnterOutput(wSurface, root.surfaceItem, output);
            outputCounter++;
            if (outputCounter == 1 && (root.type === "toplevel" || root.type === "xwayland")) {
                let outputDelegate = output.OutputItem.item;
                surfaceItem.x = outputDelegate.x + Helper.getLeftExclusiveMargin(wSurface) + 10;
                surfaceItem.y = outputDelegate.y + Helper.getTopExclusiveMargin(wSurface) + 10;
            }
        }
        onLeaveOutput: function (output) {
            if (root.wSurface && root.wSurface.surface)
                root.wSurface.surface.leaveOutput(output);
            Helper.onSurfaceLeaveOutput(wSurface, root.surfaceItem, output);
            outputCounter--;
        }
    }

    Loader {
        id: helper
        active: type === "toplevel" || type === "xwayland"
        sourceComponent: helperComponent
    }
    Component {
        id: helperComponent
        StackToplevelHelper {
            surface: root.surfaceItem
            waylandSurface: wSurface
            dockModel: root.dockModel
            creator: root.creator
            decoration: root.decoration
        }
    }
}

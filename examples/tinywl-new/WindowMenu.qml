// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Waylib.Server
import Tinywl

Menu {
    id: menu

    required property SurfaceWrapper surface

    Connections {
        target: surface
        function onRequestShowWindowMenu(pos) {
            menu.popup(pos)
        }
    }

    MenuItem {
        text: qsTr("Minimize")
        onTriggered: surface.requestMinimize()
    }

    MenuItem {
        text: surface.surfaceState === SurfaceWrapper.State.Maximized ? qsTr("Unmaximize") : qsTr("Maximize")
        onTriggered: surface.requestToggleMaximize()
    }

    MenuItem {
        text: qsTr("Move")
        onTriggered: surface.requestMove()
    }

    MenuItem {
        text: qsTr("Resize")
        // TODO:: not support now
    }

    MenuItem {
        text: surface.alwaysOnTop ? qsTr("Not always on Top") : qsTr("Always on Top")
        onTriggered: surface.alwaysOnTop = !surface.alwaysOnTop;
    }

    MenuItem {
        text: qsTr("Always on Visible Workspace")
        // TODO:: not support now
    }

    MenuItem {
        text: qsTr("Move to Work Space Left")
        enabled: surface.workspaceId !== 0
        onTriggered: Helper.workspace.addSurface(surface, surface.workspaceId - 1)
    }

    MenuItem {
        text: qsTr("Move to Work Space Right")
        enabled: surface.workspaceId !== Helper.workspace.count - 1
        onTriggered: Helper.workspace.addSurface(surface, surface.workspaceId + 1)
    }

    MenuItem {
        text: qsTr("Close")
        onTriggered: surface.shellSurface.close()
    }
}


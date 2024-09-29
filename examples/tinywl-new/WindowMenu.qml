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
        onTriggered: Helper.fakePressSurfaceBottomRightToReszie(surface)
    }

    MenuItem {
        text: surface.alwaysOnTop ? qsTr("Not always on Top") : qsTr("Always on Top")
        onTriggered: surface.alwaysOnTop = !surface.alwaysOnTop;
    }

    MenuItem {
        text: surface.showOnAllWorkspace ? qsTr("Only on Current Workspace") : qsTr("Always on Visible Workspace")
        onTriggered: surface.showOnAllWorkspace = !surface.showOnAllWorkspace
    }

    MenuItem {
        text: qsTr("Move to Left Work Space")
        enabled: surface.workspaceId !== 0 && !surface.showOnAllWorkspace
        onTriggered: Helper.workspace.addSurface(surface, surface.workspaceId - 1)
    }

    MenuItem {
        text: qsTr("Move to Right Work Space")
        enabled: surface.workspaceId !== Helper.workspace.count - 1 && !surface.showOnAllWorkspace
        onTriggered: Helper.workspace.addSurface(surface, surface.workspaceId + 1)
    }

    MenuItem {
        text: qsTr("Close")
        onTriggered: surface.shellSurface.close()
    }
}


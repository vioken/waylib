// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Waylib.Server
import Tinywl

Rectangle {
    id: titlebar
    height: 30
    color: Helper.activatedSurface === surface ? "white" : "gray"

    required property SurfaceWrapper surface

    MouseArea {
        anchors.fill: parent
        Cursor.shape: pressed ? Waylib.CursorShape.Grabbing : Qt.ArrowCursor

        onPressed: {
            surface.requestMove();
        }
    }

    Row {
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: 8
        }

        Button {
            width: titlebar.height
            text: "-"
            onClicked: surface.requestMinimize()
        }
        Button {
            width: titlebar.height
            text: "O"
            onClicked: surface.requestToggleMaximize()
        }
        Button {
            width: titlebar.height
            text: "X"
            onClicked: surface.requestClose()
        }
    }
}

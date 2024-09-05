// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Waylib.Server
import Tinywl

ListView {
    required property QtObject output
    readonly property OutputItem outputItem: output.outputItem

    width: 50
    height: Math.min(outputItem.height, contentHeight)
    x: outputItem.x
    y: (outputItem.height - height) / 2
    model: output.minimizedSurfaces
    delegate: ShaderEffectSource {
        required property SurfaceWrapper surface
        sourceItem: surface
        sourceRect: surface?.boundedRect ?? null
        live: false
        mipmap: true
        width: Math.min(sourceRect.width, 150)
        height: sourceRect.height * (width / sourceRect.width)

        MouseArea {
            anchors.fill: parent
            onClicked: surface.requestUnminimize()
        }
    }

    transform: Rotation {
        angle: 45
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
}

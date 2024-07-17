// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Waylib.Server
import Tinywl

Item {
    id: root

    function getSurfaceItemFromWaylandSurface(surface) {
        let finder = function (props) {
            if (!props.wSurface)
                return false;
            // surface is WToplevelSurface or WSurfce
            if (props.wSurface === surface || props.wSurface.surface === surface)
                return true;
            return false;
        };
        let toplevel = Helper.surfaceCreator.getIf(toplevelComponent, finder);
        if (toplevel)
            return toplevel.surfaceItem;
        let xwayland = Helper.surfaceCreator.getIf(xwaylandComponent, finder);
        if (xwayland)
            return xwayland.surfaceItem;
        let popup = Helper.surfaceCreator.getIf(popupComponent, finder);
        if (popup)
            return popup.surfaceItem;
        let layer = Helper.surfaceCreator.getIf(layerComponent, finder);
        if (layer)
            return layer.surfaceItem;
        return null;
    }

    MiniDock {
        id: dock
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            margins: 8
        }
        width: 250
    }

    DynamicCreatorComponent {
        id: toplevelComponent
        creator: Helper.surfaceCreator
        chooserRole: "type"
        chooserRoleValue: "toplevel"
        contextProperties: ({ surfType: "toplevel" }) // Object/QVariantMap type should use `({})`
        autoDestroy: false

        onObjectRemoved: function (obj) {
            obj.doDestroy();
        }

        SurfaceWrapper {
            id: toplevelSurfaceItem
            creator: toplevelComponent
            dockModel: dock.model
        }
    }

    DynamicCreatorComponent {
        id: xwaylandComponent
        creator: Helper.surfaceCreator
        chooserRole: "type"
        chooserRoleValue: "xwayland"
        contextProperties: ({ surfType: "xwayland" }) // Object/QVariantMap type should use `({})`
        autoDestroy: false

        onObjectRemoved: function (obj) {
            obj.doDestroy();
        }

        SurfaceWrapper {
            id: xwaylandSurfaceItem
            property bool forcePositionToSurface: false
            readonly property XWaylandSurface asXSurface: {
                wSurface as XWaylandSurface;
            }
            creator: xwaylandComponent
            dockModel: dock.model
            decoration.enable: !asXSurface.bypassManager
                                && asXSurface.decorationsType !== XWaylandSurface.DecorationsNoBorder
            z: asXSurface.bypassManager ? 1 : 0 // TODO: make to enum type
            asXwayland.parentSurfaceItem: root.getSurfaceItemFromWaylandSurface(asXSurface.parentXWaylandSurface)
            asXwayland.positionMode: {
                if (!surfaceItem.effectiveVisible)
                    return XWaylandSurfaceItem.ManualPosition;
                return (Helper.movingItem === surfaceItem || surfaceItem.resizeMode === SurfaceItem.SizeToSurface)
                    ? XWaylandSurfaceItem.PositionToSurface : XWaylandSurfaceItem.PositionFromSurface;
            }

            asXwayland.onEffectiveVisibleChanged: {
                if (xwaylandSurfaceItem.effectiveVisible)
                    xwaylandSurfaceItem.move(XWaylandSurfaceItem.PositionToSurface);
            }
        }
    }

    DynamicCreatorComponent {
        id: popupComponent
        creator: Helper.surfaceCreator
        chooserRole: "type"
        chooserRoleValue: "popup"
        contextProperties: ({ surfType: "popup" }) // Object/QVariantMap type should use `({})`

        SurfaceWrapper {
            id: popupSurfaceItem
            property var parentItem: root.getSurfaceItemFromWaylandSurface(wSurface.parentSurface)
            property alias xdgItem: popupSurfaceItem.asXdg

            creator: popupComponent
            dockModel: dock.model
            parent: parentItem ? parentItem : root
            visible: parentItem && parentItem.effectiveVisible
                && wSurface.surface.mapped && wSurface.WaylandSocket.rootSocket.enabled
            z: 201
            x: {
                let retX = 0; // X coordinate relative to parent
                let minX = 0;
                let maxX = root.width - wSurface.width;
                if (!parentItem) {
                    retX = xdgItem.implicitPosition.x;
                    if (retX > maxX)
                        retX = maxX;
                    if (retX < minX)
                        retX = minX;
                } else {
                    retX = xdgItem.implicitPosition.x / parentItem.surfaceSizeRatio + parentItem.contentItem.x;
                    let parentX = parent.mapToItem(root, 0, 0).x;
                    if (retX + parentX > maxX) {
                        if (parentItem.type === "popup")
                            retX = retX - wSurface.width - parent.width;
                        else
                            retX = maxX - parentX;
                    }
                    if (retX + parentX < minX)
                        retX = minX - parentX;
                }
                return retX;
            }
            y: {
                let retY = 0 // Y coordinate relative to parent
                let minY = 0
                let maxY = root.height - wSurface.height
                if (!parentItem) {
                    retY = xdgItem.implicitPosition.y
                    if (retY > maxY)
                        retY = maxY
                    if (retY < minY)
                        retY = minY
                } else {
                    retY = xdgItem.implicitPosition.y / parentItem.surfaceSizeRatio + parentItem.contentItem.y
                    let parentY = parent.mapToItem(root, 0, 0).y
                    if (retY + parentY > maxY)
                        retY = maxY - parentY
                    if (retY + parentY < minY)
                        retY = minY - parentY
                }
                return retY
            }
        }
    }
    DynamicCreatorComponent {
        id: layerComponent
        creator: Helper.surfaceCreator
        chooserRole: "type"
        chooserRoleValue: "layerShell"
        contextProperties: ({ surfType: "layerShell" }) // Object/QVariantMap type should use `({})`
        autoDestroy: false

        onObjectRemoved: function (obj) {
            obj.doDestroy()
        }

        SurfaceWrapper  {
            id: layerSurfaceItem
            creator: popupComponent
            dockModel: dock.model
        }
    }

    DynamicCreatorComponent {
        id: inputPopupComponent
        creator: Helper.surfaceCreator
        chooserRole: "type"
        chooserRoleValue: "inputPopup"
        contextProperties: ({ surfType: "inputPopup" }) // Object/QVariantMap type should use `({})`
        autoDestroy: false

        onObjectRemoved: function (obj) {
            obj.doDestroy()
        }

        SurfaceWrapper  {
            id: inputPopupSurfaceItem
            property var parentItem: root.getSurfaceItemFromWaylandSurface(wSurface.parentSurface)
            property var cursorPoint: Qt.point(surfaceItem.referenceRect.x, surfaceItem.referenceRect.y)
            property real cursorWidth: surfaceItem.referenceRect.width
            property real cursorHeight: surfaceItem.referenceRect.height

            parent: parentItem ? parentItem : root
            creator: popupComponent
            dockModel: dock.model
            x: {
                var x = cursorPoint.x + cursorWidth
                if (x + width > parent.width) {
                    x = Math.max(0, parent.width - width)
                }
                return x
            }

            y: {
                var y = cursorPoint.y + cursorHeight
                if (y + height > parent.height) {
                    y = Math.min(cursorPoint.y - height, parent.height - height)
                }
                return y
            }

        }
    }
}

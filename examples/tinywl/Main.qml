// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Waylib.Server
import Tinywl

Item {
    id :root

    WaylandServer {
        id: server

        LayerShell {
            id: layerShell

            onSurfaceAdded: function(surface) {
                QmlHelper.layerSurfaceManager.add({waylandSurface: surface})
            }
            onSurfaceRemoved: function(surface) {
                QmlHelper.layerSurfaceManager.removeIf(function(prop) {
                    return prop.waylandSurface === surface
                })
            }
        }

        GammaControlManager {
            onGammaChanged: function(output, gamma_control, ramp_size, r, g, b) {
                if (!output.setGammaLut(ramp_size, r, g, b)) {
                    sendFailedAndDestroy(gamma_control);
                };
            }
        }

        OutputManager {
            id: outputManagerV1

            onRequestTestOrApply: function(config, onlyTest) {
                var states = outputManagerV1.stateListPending()
                var ok = true

                for (const i in states) {
                    let output = states[i].output
                    output.enable(states[i].enabled)
                    if (states[i].enabled) {
                        if (states[i].mode)
                            output.setMode(states[i].mode)
                        else
                            output.setCustomMode(states[i].custom_mode_size,
                                                  states[i].custom_mode_refresh)

                        output.enableAdaptiveSync(states[i].adaptive_sync_enabled)
                        if (!onlyTest) {
                            let outputDelegate = output.OutputItem.item
                            outputDelegate.setTransform(states[i].transform)
                            outputDelegate.setScale(states[i].scale)
                            outputDelegate.x = states[i].x
                            outputDelegate.y = states[i].y
                        }
                    }

                    if (onlyTest) {
                        ok &= output.test()
                        output.rollback()
                    } else {
                        ok &= output.commit()
                    }
                }
                outputManagerV1.sendResult(config, ok)
            }
        }

        CursorShapeManager { }

        WaylandSocket {
            id: masterSocket

            freezeClientWhenDisable: false

            Component.onCompleted: {
                console.info("Listing on:", socketFile)
                Helper.startDemoClient(socketFile)
            }
        }

        InputMethodManagerV2 {
            id: inputMethodManagerV2
        }

        TextInputManagerV1 {
            id: textInputManagerV1
        }

        TextInputManagerV3 {
            id: textInputManagerV3
        }

        VirtualKeyboardManagerV1 {
            id: virtualKeyboardManagerV1
        }

        XWayland {
            id: xwayland
            compositor: Helper.compositor
            seat: Helper.seat
            lazy: false

            onReady: function () {
                // xwaylandXdgOutputManager.targetClients.push(client())
            }

            onSurfaceAdded: function(surface) {
                QmlHelper.xwaylandSurfaceManager.add({waylandSurface: surface})
            }
            onSurfaceRemoved: function(surface) {
                QmlHelper.xwaylandSurfaceManager.removeIf(function(prop) {
                    return prop.waylandSurface === surface
                })
            }
        }

        ScreenCopyManager { }
    }

    Binding {
        target: Helper.seat
        property: "keyboardFocus"
        value: Helper.getFocusSurfaceFrom(renderWindow.activeFocusItem)
    }

    Binding {
        target: QmlHelper
        property: "masterSocket"
        value: masterSocket
    }

    InputMethodHelper {
        id: inputMethodHelperSeat0
        seat: Helper.seat
        textInputManagerV1: textInputManagerV1
        textInputManagerV3: textInputManagerV3
        inputMethodManagerV2: inputMethodManagerV2
        virtualKeyboardManagerV1: virtualKeyboardManagerV1
        activeFocusItem: renderWindow.activeFocusItem.parent
        onInputPopupSurfaceV2Added: function (surface) {
            QmlHelper.inputPopupSurfaceManager.add({ popupSurface: surface, inputMethodHelper: inputMethodHelperSeat0 })
        }
        onInputPopupSurfaceV2Removed: function (surface) {
            QmlHelper.inputPopupSurfaceManager.removeIf(function (prop) {
                return prop.popupSurface === surface
            })
        }
    }

    OutputRenderWindow {
        id: renderWindow

        width: Helper.outputLayout.implicitWidth
        height: Helper.outputLayout.implicitHeight

        onOutputViewportInitialized: function (viewport) {
            // Trigger QWOutput::frame signal in order to ensure WOutputHelper::renderable
            // property is true, OutputRenderWindow when will render this output in next frame.
            Helper.enableOutput(viewport.output)
        }

        EventJunkman {
            anchors.fill: parent
        }

        Item {
            DynamicCreatorComponent {
                creator: Helper.outputCreator

                OutputDelegate {
                    property real topMargin: topbar.height
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent

            TabBar {
                id: topbar

                Layout.fillWidth: true

                TabButton {
                    text: qsTr("Stack Layout")
                    onClicked: {
                        Helper.xdgDecorationManager.preferredMode = XdgDecorationManager.Client
                    }
                }
                TabButton {
                    text: qsTr("Tiled Layout")
                    onClicked: {
                        Helper.xdgDecorationManager.preferredMode = XdgDecorationManager.Server
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                StackWorkspace {
                    visible: topbar.currentIndex === 0
                    anchors.fill: parent
                }

                TiledWorkspace {
                    visible: topbar.currentIndex === 1
                    anchors.fill: parent
                }
            }
        }
    }
}

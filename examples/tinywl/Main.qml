import QtQuick
import QtQuick.Controls
import Waylib.Server

Item {
    id: root

    property ListModel surfaceModel

    Row {
        Repeater {
            model: ListModel {
                id: outputModel
            }

            OutputWindowItem {
                id: outputWindow
                output: modelData

                Image {
                    id: background
                    sourceSize: Qt.size(parent.width, parent.height)
                    source: "file:///usr/share/backgrounds/deepin/desktop.jpg"
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }

                Item {
                    anchors.fill: parent

                    Repeater {
                        model: ListModel {
                            Component.onCompleted: {
                                if (!root.surfaceModel)
                                    root.surfaceModel = this
                            }
                        }

                        SurfaceItem {
                            required property WaylandSurface waylandSurface

                            surface: waylandSurface
                            anchors.centerIn: parent
                        }
                    }
                }

                Column {
                    anchors {
                        bottom: parent.bottom
                        right: parent.right
                        margins: 10
                    }

                    spacing: 10

                    Button {
                        text: "1X"
                        onClicked: {
                            outputWindow.output.scale = 1
                        }
                    }

                    Button {
                        text: "1.5X"
                        onClicked: {
                            outputWindow.output.scale = 1.5
                        }
                    }

                    Button {
                        text: "Normal"
                        onClicked: {
                            outputWindow.output.orientation = Output.Normal
                        }
                    }

                    Button {
                        text: "R90"
                        onClicked: {
                            outputWindow.output.orientation = Output.R90
                        }
                    }

                    Button {
                        text: "R270"
                        onClicked: {
                            outputWindow.output.orientation = Output.R270
                        }
                    }

                    Button {
                        text: "Quit"
                        onClicked: {
                            Qt.quit()
                        }
                    }
                }
            }
        }
    }

    WaylandBackend {
        id: backend
        server: waylandServer

        onOutputAdded: function(output) {
            outputModel.append({waylandOutput: output})
        }
        onOutputRemoved: function(output) {

        }
    }

    XdgShell {
        id: shell
        server: waylandServer

        onSurfaceAdded: function(surface) {
            surfaceModel.append({waylandSurface: surface})
        }
    }
}

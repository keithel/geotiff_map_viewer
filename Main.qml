import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import geotiff_viewer

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("GeoTIFF Viewer")

    GeoTiffHandler {
        id: geoTiffHandler
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        ToolBar {
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                Button {
                    text: "Open GeoTIFF"
                    onClicked: fileDialog.open()
                }

                Label {
                    Layout.fillWidth: true
                    text: geoTiffHandler.currentFile || "No file loaded"
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignHCenter
                }

                Button {
                    text: "Zoom in"
                    onClicked: imageViewer.zoomIn()
                }

                Button {
                    text: "Zoom out"
                    onClicked: imageViewer.zoomOut()
                }

                Button {
                    text: "Fit to View"
                    onClicked: imageViewer.resetZoom()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ScrollView {
                id: scrollView
                SplitView.preferredWidth: parent.width * 0.7
                SplitView.minimumWidth: 300
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                Item {
                    id: imageViewer
                    width: Math.max(scrollView.width, image.width * image.scale)
                    height: Math.max(scrollView.height, image.height * image.scale)

                    property real minScale: 0.1
                    property real maxScale: 10.0

                    function zoomIn() {
                        var newScale = image.scale * 1.2;
                        if (newScale <= maxScale) {
                            image.scale = newScale;
                        }
                    }

                    function zoomOut() {
                        var newScale = image.scale * 0.8;
                        if (newScale >= minScale) {
                            image.scale = newScale;
                        }
                    }

                    function resetZoom() {
                        image.scale = 1.0;
                        image.x = 0;
                        image.y = 0;
                    }

                    Image {
                        id: image
                        anchors.centerIn: parent
                        source: geoTiffHandler.imageSource
                        cache: false
                        transformOrigin: Item.Center
                        scale: 1.0

                        DragHandler {}
                        WheelHandler {
                            onWheel: (event) => {
                                if(event.angleDelta.y > 0) {
                                    imageViewer.zoomIn();
                                } else {
                                    imageViewer.zoomOut();
                                }
                            }
                        }
                        TapHandler {
                            onDoubleTapped: (point, button) => {
                                imageViewer.resetZoom();
                            }
                        }
                    }
                }
            }

            Pane {
                SplitView.preferredWidth: parent.width * 0.3
                SplitView.minimumWidth: 200

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    GroupBox {
                        Layout.fillWidth: true
                        title: "GeoTIFF Information"

                        ColumnLayout {
                            anchors.fill: parent

                            Label {
                                text: "<b>File:</b> " + (geoTiffHandler.fileName || "None")
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "<b>Dimensions:</b> " + (geoTiffHandler.dimensions || "Unknown")
                                visible: geoTiffHandler.dimensions !== ""
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "<b>Coordinate System:</b> " + (geoTiffHandler.coordinateSystem || "Unknown")
                                visible: geoTiffHandler.coordinateSystem !== ""
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "<b>Projection:</b> " + (geoTiffHandler.projection || "Unknown")
                                visible: geoTiffHandler.projection !== ""
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        title: "Geospatial Bounds"

                        GridLayout {
                            anchors.fill: parent
                            columns: 2

                            Label { text: "Min X:" }
                            Label {
                                text: geoTiffHandler.boundsMinX || "Unknown"
                                Layout.fillWidth: true
                            }

                            Label { text: "Min Y:" }
                            Label {
                                text: geoTiffHandler.boundsMinY || "Unknown"
                                Layout.fillWidth: true
                            }

                            Label { text: "Max X:" }
                            Label {
                                text: geoTiffHandler.boundsMaxX || "Unknown"
                                Layout.fillWidth: true
                            }

                            Label { text: "Max Y:" }
                            Label {
                                text: geoTiffHandler.boundsMaxY || "Unknown"
                                Layout.fillWidth: true
                            }
                        }
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        title: "Bands Information"

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ListView {
                                model: geoTiffHandler.bandsModel
                                delegate: ItemDelegate {
                                    width: parent.width
                                    contentItem: Label {
                                        text: modelData
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }

        RowLayout {
            id: statusBar
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: geoTiffHandler.statusMessage || "Ready"
                elide: Text.ElideRight
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a GeoTIFF file"
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        nameFilters: ["GeoTIFF files (*.tif *.tiff)", "All files (*)"]

        onAccepted: {
            geoTiffHandler.loadGeoTIFF(fileDialog.selectedFile)
        }
    }
}

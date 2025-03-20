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

            // Replace ScrollView with Flickable for better panning control
            Flickable {
                id: flickable
                SplitView.preferredWidth: parent.width * 0.7
                SplitView.minimumWidth: 300
                clip: true

                // Set the content size based on the scaled image
                contentWidth: Math.max(image.width * imageViewer.currentScale, width)
                contentHeight: Math.max(image.height * imageViewer.currentScale, height)

                // Enable flicking behavior for touch screens and smooth scrolling
                // flickableDirection: Flickable.HorizontalAndVerticalFlick
                // boundsMovement: Flickable.StopAtBounds

                // Center small images in the view
                property real effectiveWidth: Math.max(contentWidth, width)
                property real effectiveHeight: Math.max(contentHeight, height)

                // Add ScrollBars to the Flickable
                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    active: true
                }
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    active: true
                }

                Image {
                    id: image
                    cache: false
                    width: sourceSize.width
                    height: sourceSize.height
                    scale: imageViewer.currentScale
                    transformOrigin: Item.TopLeft

                    // Center image when smaller than the view
                    x: Math.max(0, (flickable.width - width * scale) / 2)
                    y: Math.max(0, (flickable.height - height * scale) / 2)

                    // Update when image loads
                    onStatusChanged: {
                        if (status === Image.Ready) {
                            imageViewer.resetZoom();
                        }
                    }

                    // WheelHandler for zooming
                    WheelHandler {
                        id: wheelHandler
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

                        onWheel: (event) => {
                            // Remember the position under the mouse in scene coordinates
                            var positionInFlickableX = event.x + flickable.contentX
                            var positionInFlickableY = event.y + flickable.contentY

                            // Calculate position relative to image (0-1)
                            var relativeX = positionInFlickableX / (image.width * imageViewer.currentScale)
                            var relativeY = positionInFlickableY / (image.height * imageViewer.currentScale)
                            console.log("x " + event.x + " relativeX " + relativeX + " posInFlickable (" + positionInFlickableX + ", " + positionInFlickableY + "), scaledImageWidth " + (image.width * imageViewer.currentScale));

                            // Apply zoom
                            if (event.angleDelta.y > 0) {
                                imageViewer.zoomIn()
                            } else {
                                imageViewer.zoomOut()
                            }

                            // Calculate new content position to keep mouse over same point
                            var newContentX = (image.width * imageViewer.currentScale) * relativeX - event.x
                            var newContentY = (image.height * imageViewer.currentScale) * relativeY - event.y
                            console.log("newContentX " + newContentX + " scaledImageWidth " + (image.width * imageViewer.currentScale));

                            // // Update Flickable position
                            // flickable.contentX = newContentX
                            // flickable.contentY = newContentY
                        }
                    }

                    // TapHandler for double-tap to reset zoom
                    TapHandler {
                        onDoubleTapped: {
                            imageViewer.resetZoom()
                        }
                    }
                }
            }

            // Image view control functions
            QtObject {
                id: imageViewer
                property real currentScale: 1.0
                property real minScale: 0.1
                property real maxScale: 10.0

                function zoomIn() {
                    var newScale = currentScale * 1.2
                    if (newScale <= maxScale) {
                        currentScale = newScale
                    }
                }

                function zoomOut() {
                    var newScale = currentScale * 0.8
                    if (newScale >= minScale) {
                        currentScale = newScale
                    }
                }

                function resetZoom() {
                    // Calculate scale to fit
                    if (image.sourceSize.width > 0 && image.sourceSize.height > 0) {
                        var scaleX = flickable.width / image.sourceSize.width
                        var scaleY = flickable.height / image.sourceSize.height
                        currentScale = Math.min(1.0, Math.min(scaleX, scaleY))
                    } else {
                        currentScale = 1.0
                    }

                    // Reset position
                    flickable.contentX = 0
                    flickable.contentY = 0
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
            image.source = "image://geotiff/" + fileDialog.selectedFile;
            geoTiffHandler.loadMetadata(fileDialog.selectedFile)
        }
    }
}

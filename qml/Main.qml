import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtLocation
import QtPositioning

import geotiff_viewer

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("GeoTIFF Viewer")

    function loadTiff(url) {
        image.source = "image://geotiff/" + url;
        console.log("set image.source to " + image.source)
        GeoTiffHandler.loadMetadata(url)
        tiffImgMQI.coordinate = QtPositioning.coordinate(GeoTiffHandler.boundsMaxY, GeoTiffHandler.boundsMinX)
        tiffImgMQI.zoomLevel = 14
    }

    Component.onCompleted: loadTiff("file:///home/kyzik/Build/l3h-insight/austro-hungarian-maps/sheets_geo/2868_000_geo.tif")

    Plugin {
        id: mapPlugin
        name: "osm"
        PluginParameter {
            name: "osm.mapping.providersrepository.address"
            value: AppConfig.osmMappingProvidersRepositoryAddress
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        ToolBar {
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                SpinBox {
                    id: mapChoice
                    from: 0
                    to: 6
                    value: 0
                    wrap: true
                    WheelHandler {
                        onWheel: (wheel) => {
                            if(wheel.angleDelta.y > 0)
                                mapChoice.increase();
                            else
                                mapChoice.decrease();
                        }
                    }
                }

                SpinBox {
                    id: imgOpacityChoice
                    from: 0
                    to: 100
                    value: 75
                    stepSize: 5
                    WheelHandler { onWheel: (wheel) => { if(wheel.angleDelta.y > 0) imgOpacityChoice.increase(); else imgOpacityChoice.decrease() } }
                }

                Button {
                    text: "Open GeoTIFF"
                    onClicked: fileDialog.open()
                }

                Label {
                    Layout.fillWidth: true
                    text: GeoTiffHandler.currentFile || "No file loaded"
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

            Item {
                SplitView.preferredWidth: parent.width * 0.7
                SplitView.minimumWidth: 300
                SplitView.fillHeight: true

                Map {
                    id: mapBase
                    anchors.fill: parent

                    plugin: mapPlugin
                    center: QtPositioning.coordinate(52.74,21.83) // Austro-Hungary
                    zoomLevel: 14
                    activeMapType: supportedMapTypes[mapChoice.value]
                    property geoCoordinate startCentroid
                    property geoCoordinate cursorCoordinate;

                    HoverHandler {
                        onPointChanged: {
                            mapBase.cursorCoordinate = mapBase.toCoordinate(point.position, false);
                        }
                    }

                    PinchHandler {
                        id: pinch
                        target: null
                        onActiveChanged: if (active) {
                            mapBase.startCentroid = mapBase.toCoordinate(pinch.centroid.position, false)
                        }
                        onScaleChanged: (delta) => {
                            mapBase.zoomLevel += Math.log2(delta)
                            mapBase.alignCoordinateToPoint(mapBase.startCentroid, pinch.centroid.position)
                        }
                        onRotationChanged: (delta) => {
                            mapBase.bearing -= delta
                            mapBase.alignCoordinateToPoint(mapBase.startCentroid, pinch.centroid.position)
                        }
                        grabPermissions: PointerHandler.TakeOverForbidden
                    }
                    WheelHandler {
                        id: wheel
                        // workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432:
                        // Magic Mouse pretends to be a trackpad but doesn't work with PinchHandler
                        // and we don't yet distinguish mice and trackpads on Wayland either
                        acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
                                         ? PointerDevice.Mouse | PointerDevice.TouchPad
                                         : PointerDevice.Mouse
                        rotationScale: 1/120
                        property: "zoomLevel"
                    }
                    DragHandler {
                        id: drag
                        target: null
                        onTranslationChanged: (delta) => mapBase.pan(-delta.x, -delta.y)
                    }
                    Shortcut {
                        enabled: mapBase.zoomLevel < mapBase.maximumZoomLevel
                        sequence: StandardKey.ZoomIn
                        onActivated: mapBase.zoomLevel = mapBase.zoomLevel + 0.05 //Math.round(mapBase.zoomLevel + 1)
                    }
                    Shortcut {
                        enabled: mapBase.zoomLevel > mapBase.minimumZoomLevel
                        sequence: StandardKey.ZoomOut
                        onActivated: mapBase.zoomLevel = mapBase.zoomLevel - 0.05 //Math.round(mapBase.zoomLevel - 1)
                    }
                }
                Map {
                    id: mapOverlay
                    anchors.fill: mapBase
                    plugin: Plugin { name: "itemsoverlay" }
                    center: mapBase.center
                    color: 'transparent' // Necessary to make this map transparent
                    minimumFieldOfView: mapBase.minimumFieldOfView
                    maximumFieldOfView: mapBase.maximumFieldOfView
                    minimumTilt: mapBase.minimumTilt
                    maximumTilt: mapBase.maximumTilt
                    minimumZoomLevel: mapBase.minimumZoomLevel
                    maximumZoomLevel: mapBase.maximumZoomLevel
                    zoomLevel: mapBase.zoomLevel
                    tilt: mapBase.tilt;
                    bearing: mapBase.bearing
                    fieldOfView: mapBase.fieldOfView
                    z: mapBase.z + 1

                    MapQuickItem {
                        id: tiffImgMQI
                        sourceItem: Image {
                            id: image
                        }
                        coordinate: QtPositioning.coordinate(42.99486, -71.463457)
                        anchorPoint: Qt.point(0,0)
                        zoomLevel: 17
                        opacity: (imgOpacityChoice.value*1.0)/100
                    }
                }
            }

            Flickable {
                id: flickable
                SplitView.preferredWidth: parent.width * 0.3
                SplitView.minimumWidth: 200
                SplitView.fillHeight: true
                contentWidth: width
                contentHeight: columnLayout.height
                clip: true

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }

                ColumnLayout {
                    id: columnLayout
                    width: parent.width
                    spacing: 10

                    GroupBox {
                        Layout.fillWidth: true
                        title: "GeoTIFF Information"

                        ColumnLayout {
                            anchors.fill: parent

                            Label {
                                text: "<b>File:</b> " + (GeoTiffHandler.fileName || "None")
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "<b>Dimensions:</b> " + (GeoTiffHandler.dimensions || "Unknown")
                                visible: GeoTiffHandler.dimensions !== ""
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "<b>Coordinate System:</b> " + (GeoTiffHandler.coordinateSystem || "Unknown")
                                visible: GeoTiffHandler.coordinateSystem !== ""
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "<b>Projection:</b> " + (GeoTiffHandler.projection || "Unknown")
                                visible: GeoTiffHandler.projection !== ""
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
                            TextField {
                                text: GeoTiffHandler.boundsMinX || "Unknown"
                                Layout.fillWidth: true
                                readOnly: true
                                background: Item {}
                            }

                            Label { text: "Min Y:" }
                            TextField {
                                text: GeoTiffHandler.boundsMinY || "Unknown"
                                Layout.fillWidth: true
                                readOnly: true
                                background: Item {}
                            }

                            Label { text: "Max X:" }
                            TextField {
                                text: GeoTiffHandler.boundsMaxX || "Unknown"
                                Layout.fillWidth: true
                                readOnly: true
                                background: Item {}
                            }

                            Label { text: "Max Y:" }
                            TextField {
                                text: GeoTiffHandler.boundsMaxY || "Unknown"
                                Layout.fillWidth: true
                                readOnly: true
                                background: Item {}
                            }
                        }
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: bandsInfoLV.height + implicitLabelHeight + verticalPadding
                        title: "Bands Information"

                        ListView {
                            id: bandsInfoLV
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: contentHeight

                            model: GeoTiffHandler.bandsModel
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
            }
        }

        RowLayout {
            id: statusBar
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: GeoTiffHandler.statusMessage || "Ready"
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
            loadTiff(fileDialog.selectedFile);
        }
    }
}

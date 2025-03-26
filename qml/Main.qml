import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtLocation
import QtPositioning

import geotiff_viewer

ApplicationWindow {
    width: 1024
    height: 768
    visible: true
    title: qsTr("GeoTIFF Viewer")

    function loadTiff(url) {
        // image.source = "image://geotiff/" + url;
        console.log("image source " + url)
        GeoTiffHandler.loadMetadata(url)

        // tiffImgMQI.coordinate = QtPositioning.coordinate(GeoTiffHandler.boundsMaxY, GeoTiffHandler.boundsMinX)
        // imgZoomLevelChoice.value = 140;
        var jsurl = new URL(url)
        geotiffoverlay.source = jsurl.pathname
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
            Layout.preferredHeight: contentHeight + 10

            RowLayout {
                anchors.fill: parent
                Layout.alignment: Qt.AlignVCenter

                Button {
                    text: "Open GeoTIFF"
                    onClicked: fileDialog.open()
                }

                SpinBox {
                    id: mapChoice
                    from: 0
                    to: 6
                    value: 0
                    wrap: true
                    WheelHandler { onWheel: (wheel) => { if(wheel.angleDelta.y > 0) mapChoice.increase(); else mapChoice.decrease(); } }
                    hoverEnabled: true
                    ToolTip.text: "Choose between map tilesets"
                    ToolTip.visible: hovered
                    ToolTip.delay: Application.styleHints.mousePressAndHoldInterval
                }

                SpinBox {
                    id: imgOpacityChoice
                    from: 0
                    to: 100
                    value: 75
                    stepSize: 5
                    WheelHandler { onWheel: (wheel) => { if(wheel.angleDelta.y > 0) imgOpacityChoice.increase(); else imgOpacityChoice.decrease() } }
                    hoverEnabled: true
                    ToolTip.text: "Set opacity of image"
                    ToolTip.visible: hovered
                    ToolTip.delay: Application.styleHints.mousePressAndHoldInterval
                }

                SpinBox {
                    id: imgZoomLevelChoice
                    from: 10
                    to: 200
                    // stepSize: 5
                    WheelHandler { onWheel: (wheel) => { if(wheel.angleDelta.y > 0) imgZoomLevelChoice.increase(); else imgZoomLevelChoice.decrease() } }
                    hoverEnabled: true
                    ToolTip.text: "Set the map zoomlevel at which the image is shown at 100% scale"
                    ToolTip.visible: hovered
                    ToolTip.delay: Application.styleHints.mousePressAndHoldInterval
                }

                Label {
                    Layout.fillWidth: true
                    text: GeoTiffHandler.currentFile || "No file loaded"
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignHCenter
                    HoverHandler {
                        id: fileLabelHoverHandler
                    }
                    ToolTip.text: "path of GeoTIFF file that is showing"
                    ToolTip.visible: fileLabelHoverHandler.hovered
                    ToolTip.delay: Application.styleHints.mousePressAndHoldInterval
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
                    center: QtPositioning.coordinate(52.9,22) // Austro-Hungary
                    zoomLevel: 10.5
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
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
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

                    GeoTiffOverlay {
                        id: geotiffoverlay
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

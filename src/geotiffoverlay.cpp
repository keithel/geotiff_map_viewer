#include "geotiffoverlay.h"
#include <QPainter>
#include <QSGTransformNode>
#include <QSGSimpleTextureNode>
#include <QGeoRectangle>

GeoTiffOverlay::GeoTiffOverlay(QQuickItem *parent) : QQuickItem(parent), m_map(nullptr)
{
    // Register GDAL drivers
    GDALAllRegister();

    // Enable item to receive paint events
    setFlag(QQuickItem::ItemHasContents, true);
}

GeoTiffOverlay::~GeoTiffOverlay()
{
    if (m_dataset) {
        GDALClose(m_dataset);
    }
}

void GeoTiffOverlay::setGeoTiffPath(const QString &path)
{
    if (m_geoTiffPath != path) {
        m_geoTiffPath = path;
        loadGeoTiff();
        emit geoTiffPathChanged();
        update();
    }
}

void GeoTiffOverlay::setMap(QObject *map)
{
    if (m_map != map) {
        m_map = map;
        // Connect to map's center and zoom level changes
        if (m_map) {
            connect(m_map, SIGNAL(centerChanged(QGeoCoordinate)), this, SLOT(updateTransform()));
            connect(m_map, SIGNAL(zoomLevelChanged()), this, SLOT(updateTransform()));
            updateTransform();
        }
        emit mapChanged();
    }
}

QSGNode *GeoTiffOverlay::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    // if (!m_map || !m_dataset || m_geoTransform.isEmpty())
    //     return nullptr;

    // // Create a texture with our transformed image
    // if (m_transformedImage.isNull())
    //     return nullptr;

    // // Here you would create/update your QSGNode tree with the transformed image
    // // For brevity, the full implementation is omitted, but you would:
    // // 1. Create a texture from m_transformedImage
    // // 2. Create/update a textured rectangle node
    // // 3. Position the node correctly

    // return oldNode; // Placeholder - actual implementation would return the updated node

    if (!m_map || !m_dataset || m_geoTransform.isEmpty() || m_transformedImage.isNull())
        return nullptr;

    // Cast the oldNode to a QSGTransformNode, or create a new one if it doesn't exist
    QSGTransformNode* rootNode = static_cast<QSGTransformNode*>(oldNode);

    if (!rootNode) {
        rootNode = new QSGTransformNode();
    }

    // Get the current transform - in this case, we just use identity since
    // our image is already transformed to match the map in updateTransform()
    QMatrix4x4 identityMatrix;
    rootNode->setMatrix(identityMatrix);

    // Find the texture node if it exists, otherwise create a new one
    QSGSimpleTextureNode* textureNode = nullptr;
    if (rootNode->childCount() > 0) {
        textureNode = static_cast<QSGSimpleTextureNode*>(rootNode->firstChild());
    } else {
        textureNode = new QSGSimpleTextureNode();
        rootNode->appendChildNode(textureNode);
    }

    // Create or update the texture from our transformed image
    QSGTexture* texture = window()->createTextureFromImage(
        m_transformedImage,
        QQuickWindow::TextureHasAlphaChannel
        );

    if (!texture) {
        qWarning() << "Failed to create texture from GeoTIFF image";
        delete rootNode; // Clean up
        return nullptr;
    }

    // Set the texture on the node
    textureNode->setTexture(texture);
    textureNode->setOwnsTexture(true); // The node will take ownership of the texture

    // Set the rect to cover the entire item
    textureNode->setRect(0, 0, width(), height());

    // Set texture filtering mode if needed
    texture->setFiltering(QSGTexture::Linear);

    // // Set blending mode for transparency handling
    // textureNode->setBlending(QSGTexture::BlendTranslucent);

    return rootNode;
}

void GeoTiffOverlay::loadGeoTiff()
{
    // Close previous dataset if any
    if (m_dataset) {
        GDALClose(m_dataset);
        m_dataset = nullptr;
    }

    // Open GeoTIFF file
    m_dataset = static_cast<GDALDataset*>(GDALOpen(m_geoTiffPath.toUtf8().constData(), GA_ReadOnly));
    if (!m_dataset) {
        qWarning() << "Failed to open GeoTIFF file:" << m_geoTiffPath;
        return;
    }

    // Get geotransform information
    double geoTransform[6];
    if (m_dataset->GetGeoTransform(geoTransform) != CE_None) {
        qWarning() << "Failed to get geotransform from GeoTIFF";
        GDALClose(m_dataset);
        m_dataset = nullptr;
        return;
    }

    // Store the geotransform
    m_geoTransform.clear();
    for (int i = 0; i < 6; ++i) {
        m_geoTransform.append(geoTransform[i]);
    }

    // Get projection information
    const char* projWkt = m_dataset->GetProjectionRef();
    if (projWkt && strlen(projWkt) > 0) {
        OGRSpatialReference srcSRS;
        srcSRS.importFromWkt(projWkt);

        // Create destination SRS (EPSG:4326, which Qt Location uses)
        OGRSpatialReference dstSRS;
        dstSRS.importFromEPSG(4326);

        // Create transform between the two SRS
        m_coordTransform = OGRCreateCoordinateTransformation(&srcSRS, &dstSRS);
        if (!m_coordTransform) {
            qWarning() << "Failed to create coordinate transformation";
        }
    } else {
        qWarning() << "GeoTIFF has no projection information";
    }

    updateTransform();
}

void GeoTiffOverlay::updateTransform()
{
    if (!m_map || !m_dataset || m_geoTransform.isEmpty())
        return;

    // Get the visible region of the map
    QVariant visibleRegion = m_map->property("visibleRegion");
    Q_ASSERT(visibleRegion.userType() == QGeoShape::staticMetaObject.metaType().id());
    QGeoShape shape = visibleRegion.value<QGeoShape>();
    Q_ASSERT(shape.type() == QGeoShape::RectangleType);
    QGeoRectangle *rectangle = static_cast<QGeoRectangle*>(&shape);
    QGeoCoordinate topLeft = rectangle->topLeft();
    QGeoCoordinate bottomRight = rectangle->bottomRight();

    // Calculate the bounds of our GeoTIFF in lat/lon coordinates
    double minX = m_geoTransform[0];
    double maxY = m_geoTransform[3];
    double maxX = minX + m_geoTransform[1] * m_dataset->GetRasterXSize();
    double minY = maxY + m_geoTransform[5] * m_dataset->GetRasterYSize();

    // Transform these coordinates if needed
    if (m_coordTransform) {
        double points[] = {
            minX, minY,
            maxX, minY,
            maxX, maxY,
            minX, maxY
        };

        // Transform all corners
        if (!m_coordTransform->Transform(4, points, points + 1, nullptr, nullptr)) {
            qWarning() << "Coordinate transformation failed";
            return;
        }

        // Update min/max values
        minX = std::min({points[0], points[2], points[4], points[6]});
        maxX = std::max({points[0], points[2], points[4], points[6]});
        minY = std::min({points[1], points[3], points[5], points[7]});
        maxY = std::max({points[1], points[3], points[5], points[7]});
    }

    // Calculate the pixel coordinates in the map view
    QVariant centerVar = m_map->property("center");
    QGeoCoordinate center = centerVar.value<QGeoCoordinate>();
    double zoomLevel = m_map->property("zoomLevel").toDouble();

    // Calculate scaling factor based on zoom level
    double scale = pow(2, zoomLevel);

    // Get map width and height
    double mapWidth = width();
    double mapHeight = height();

    // Calculate the pixel coordinates of the GeoTIFF corners
    QPointF topLeftPx = geoToPixel(QGeoCoordinate(maxY, minX));
    QPointF bottomRightPx = geoToPixel(QGeoCoordinate(minY, maxX));

    // Create transformation matrix
    QTransform transform;

    // Handle rotation if needed - depends on the GeoTIFF and its alignment with the map
    // This example assumes north-up GeoTIFF with no rotation needed

    // Create a QImage from the GeoTIFF
    int width = m_dataset->GetRasterXSize();
    int height = m_dataset->GetRasterYSize();

    // Read all raster bands
    int bandCount = m_dataset->GetRasterCount();

    QImage image(width, height, bandCount >= 4 ? QImage::Format_RGBA8888 :
                                    (bandCount == 3 ? QImage::Format_RGB888 : QImage::Format_Grayscale8));

    // Read bands
    // This is simplified - you might want to handle different band types properly
    GDALRasterBand* redBand = m_dataset->GetRasterBand(1);

    if (bandCount == 1) {
        // Grayscale image
        std::vector<uint8_t> buffer(width * height);
        redBand->RasterIO(GF_Read, 0, 0, width, height, buffer.data(), width, height, GDT_Byte, 0, 0);

        for (int y = 0; y < height; ++y) {
            memcpy(image.scanLine(y), buffer.data() + y * width, width);
        }
    }
    else if (bandCount >= 3) {
        // RGB or RGBA image
        GDALRasterBand* greenBand = m_dataset->GetRasterBand(2);
        GDALRasterBand* blueBand = m_dataset->GetRasterBand(3);
        GDALRasterBand* alphaBand = bandCount >= 4 ? m_dataset->GetRasterBand(4) : nullptr;

        std::vector<uint8_t> redBuffer(width * height);
        std::vector<uint8_t> greenBuffer(width * height);
        std::vector<uint8_t> blueBuffer(width * height);
        std::vector<uint8_t> alphaBuffer(width * height);

        redBand->RasterIO(GF_Read, 0, 0, width, height, redBuffer.data(), width, height, GDT_Byte, 0, 0);
        greenBand->RasterIO(GF_Read, 0, 0, width, height, greenBuffer.data(), width, height, GDT_Byte, 0, 0);
        blueBand->RasterIO(GF_Read, 0, 0, width, height, blueBuffer.data(), width, height, GDT_Byte, 0, 0);

        if (alphaBand) {
            alphaBand->RasterIO(GF_Read, 0, 0, width, height, alphaBuffer.data(), width, height, GDT_Byte, 0, 0);
        }
        else {
            // No alpha channel, fill with 255 (fully opaque)
            std::fill(alphaBuffer.begin(), alphaBuffer.end(), 255);
        }

        for (int y = 0; y < height; ++y) {
            uint8_t* scanline = image.scanLine(y);
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                if (bandCount >= 4) {
                    scanline[x*4] = redBuffer[idx];
                    scanline[x*4+1] = greenBuffer[idx];
                    scanline[x*4+2] = blueBuffer[idx];
                    scanline[x*4+3] = alphaBuffer[idx];
                }
                else {
                    scanline[x*3] = redBuffer[idx];
                    scanline[x*3+1] = greenBuffer[idx];
                    scanline[x*3+2] = blueBuffer[idx];
                }
            }
        }
    }

    // Now transform the image to match the map
    QRectF sourceRect(0, 0, width, height);
    QRectF targetRect(topLeftPx, bottomRightPx);

    // Create a new image with the correct size for our map view
    QImage transformedImage(mapWidth, mapHeight, QImage::Format_RGBA8888);
    transformedImage.fill(Qt::transparent);

    QPainter painter(&transformedImage);

    // Calculate the mapping from source to target
    QTransform mapTransform;
    mapTransform = QTransform::fromTranslate(targetRect.left(), targetRect.top());
    mapTransform.scale(targetRect.width() / sourceRect.width(),
                       targetRect.height() / sourceRect.height());

    // Draw the image with transformation
    painter.setTransform(mapTransform);
    painter.drawImage(sourceRect, image);
    painter.end();

    m_transformedImage = transformedImage;
    update(); // Request a redraw
}

QPointF GeoTiffOverlay::geoToPixel(const QGeoCoordinate &coord)
{
    if (!m_map)
        return QPointF(0, 0);

    // Use Qt Location's mapping from geo coordinates to screen coordinates
    QVariant point;
    QMetaObject::invokeMethod(m_map, "fromCoordinate",
                                               Q_RETURN_ARG(QVariant, point),
                                               Q_ARG(QVariant, QVariant::fromValue(coord)));
    return point.toPointF();
}

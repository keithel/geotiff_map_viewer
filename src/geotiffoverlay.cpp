#include "geotiffoverlay.h"
#include <QPainter>
#include <QSGTransformNode>
#include <QSGSimpleTextureNode>
#include <QGeoRectangle>
#include <QGeoPolygon>

#include <QImageWriter>

#include <6.9.0/QtLocation/private/qdeclarativegeomap_p.h>

GeoTiffOverlay::GeoTiffOverlay(QQuickItem *parent) : QQuickItem(parent)
{
    // Register GDAL drivers
    GDALAllRegister();
}

GeoTiffOverlay::~GeoTiffOverlay()
{
    if (m_dataset) {
        GDALClose(m_dataset);
    }
}

void GeoTiffOverlay::setSource(const QString &source)
{

    QQuickItem *p = parentItem();
    m_map = qobject_cast<QDeclarativeGeoMap *>(p);
    if (m_map == nullptr)
        qWarning() << "Parent of GeoTiffOverlay must be a Qt Location `Map` item.";
    else {
        // Enable item to receive paint events if there is a map parent.
        setFlag(QQuickItem::ItemHasContents, true);

        connect(m_map, &QDeclarativeGeoMap::visibleRegionChanged, this, &GeoTiffOverlay::updateTransform);
    }

    if (m_map && m_source != source) {
        m_source = source;
        loadSource();
        emit sourceChanged();
        update();
    }
}

QSGNode *GeoTiffOverlay::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
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

void GeoTiffOverlay::loadSource()
{
    // Close previous dataset if any
    if (m_dataset) {
        GDALClose(m_dataset);
        m_dataset = nullptr;
    }

    // Open GeoTIFF file
    m_dataset = static_cast<GDALDataset*>(GDALOpen(m_source.toUtf8().constData(), GA_ReadOnly));
    if (!m_dataset) {
        qWarning() << "Failed to open GeoTIFF file:" << m_source;
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

QString geoRectToDMSString(const QGeoRectangle &gRect) {
    QList<QGeoCoordinate> coords = {
        gRect.bottomRight(),
        gRect.bottomLeft(),
        gRect.topLeft(),
        gRect.topRight()
    };

    QString str("QGeoRectangle([ ");
    for(auto &coord : coords) {
        str.append(coord.toString());
        double alt = coord.altitude();
        alt = (alt == alt) ? alt : 0; // Set to zero if NaN
        str.append(QString(", %1m").arg(alt, 0, 'f', 0));
        str.append(",");
    }
    str.append(" ])");
    return str;
}

void reportCplErrWarning(CPLErr errType, const QString& msg)
{
    QString errTypeStr;
    switch(errType) {
    case CPLErr::CE_Debug:
        errTypeStr = "debug";
        break;
    case CPLErr::CE_Failure:
        errTypeStr = "failure";
        break;
    case CPLErr::CE_Fatal:
        errTypeStr = "fatal";
        break;
    case CPLErr::CE_Warning:
        errTypeStr = "warning";
        break;
    case CPLErr::CE_None:
        errTypeStr = "none";
        break;
    default:
        errTypeStr = "unknown";
    }
    qWarning() << msg << errTypeStr << ": " << CPLGetLastErrorMsg();
}

void GeoTiffOverlay::updateTransform()
{
    qDebug() << "updateTransform";
    if (!m_map || !m_dataset || m_geoTransform.isEmpty())
        return;

    double mapWidth = m_map->width();
    double mapHeight = m_map->height();
    if (mapWidth <= 0 || mapHeight <= 0)
        return;

    // Get the visible region of the map
    QVariant visibleRegion = m_map->property("visibleRegion");
    Q_ASSERT(visibleRegion.userType() == QGeoShape::staticMetaObject.metaType().id());
    QGeoShape shape = visibleRegion.value<QGeoShape>();
    // qDebug() << "QGeoShape:" << shape;
    QGeoShape::ShapeType shapeType = shape.type();

    Q_ASSERT(shapeType == QGeoShape::RectangleType || shapeType == QGeoShape::PolygonType);
    QGeoCoordinate topLeft;
    QGeoCoordinate bottomRight;
    if (shapeType == QGeoShape::RectangleType) {
        QGeoRectangle *rectangle = static_cast<QGeoRectangle*>(&shape);
        // qDebug() << "Rectangle" << geoRectToDMSString(*rectangle);
        topLeft = rectangle->topLeft();
        bottomRight = rectangle->bottomRight();
    }
    else {
        QGeoPolygon *poly = static_cast<QGeoPolygon*>(&shape);
        // qDebug() << "Polygon  " << poly->toString();
        const QList<QGeoCoordinate> &perimeter = poly->perimeter();
        Q_ASSERT(perimeter.length() == 4);
        QGeoRectangle boundingGRect = poly->boundingGeoRectangle();
        // qDebug() << "RBounds" << geoRectToDMSString(boundingGRect);
        topLeft = boundingGRect.topLeft();
        bottomRight = boundingGRect.bottomRight();
    }

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
    // QVariant centerVar = m_map->property("center");
    // QGeoCoordinate center = centerVar.value<QGeoCoordinate>();
    // double zoomLevel = m_map->property("zoomLevel").toDouble();

    // // Calculate scaling factor based on zoom level
    // double scale = pow(2, zoomLevel);

    // Get map width and height
    double mapWidth = m_map->width();
    double mapHeight = m_map->height();

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

    CPLErr err;
    if (bandCount == 1) {
        // Grayscale image
        std::vector<uint8_t> buffer(width * height);
        err = redBand->RasterIO(GF_Read, 0, 0, width, height, buffer.data(), width, height, GDT_Byte, 0, 0);
        if (err)
            reportCplErrWarning(err, "GDALRasterBand::RasterIO call on redBand failed with ");

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
        // If no alpha channel, fill with 255 (completely opaque)
        std::vector<uint8_t> alphaBuffer = bandCount > 3 ? std::vector<uint8_t>(width*height) : std::vector<uint8_t>(width*height, 255);

        err = redBand->RasterIO(GF_Read, 0, 0, width, height, redBuffer.data(), width, height, GDT_Byte, 0, 0);
        if (err)
            reportCplErrWarning(err, "GDALRasterBand::RasterIO call on redBand failed with ");

        err = greenBand->RasterIO(GF_Read, 0, 0, width, height, greenBuffer.data(), width, height, GDT_Byte, 0, 0);
        if (err)
            reportCplErrWarning(err, "GDALRasterBand::RasterIO call on greenBand failed with ");

        err = blueBand->RasterIO(GF_Read, 0, 0, width, height, blueBuffer.data(), width, height, GDT_Byte, 0, 0);
        if (err)
            reportCplErrWarning(err, "GDALRasterBand::RasterIO call on blueBand failed with ");

        if (alphaBand) {
            err = alphaBand->RasterIO(GF_Read, 0, 0, width, height, alphaBuffer.data(), width, height, GDT_Byte, 0, 0);
            if (err)
                reportCplErrWarning(err, "GDALRasterBand::RasterIO call on alphaBand failed with ");
        }

        // int usedBandCount = usedBandCount > 4 ? 4 : usedBandCount;
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
    // QImageWriter w("/tmp/img.png");
    // // w.write(image);
    // w.setFileName("/tmp/img_xform.png");
    // w.write(m_transformedImage);
    update(); // Request a redraw
}

QPointF GeoTiffOverlay::geoToPixel(const QGeoCoordinate &coord)
{
    if (!m_map)
        return QPointF(0, 0);

    // Use Qt Location's mapping from geo coordinates to screen coordinates
    return m_map->fromCoordinate(coord, false);
}

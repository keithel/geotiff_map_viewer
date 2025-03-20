#include "geotiffimageprovider.h"

GeoTiffImageProvider::GeoTiffImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    m_geoTiffHandler = GeoTiffHandler::instance();
}

QImage GeoTiffImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    qDebug() << "Tiff image" << id << "requested";
    QImage image = m_geoTiffHandler->loadGeoTiffImage(id);
    *size = image.size();
    return requestedSize.isValid() ? image.copy(QRect(QPoint(0,0), requestedSize)) : image;
}

#ifndef GEOTIFFIMAGEPROVIDER_H
#define GEOTIFFIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include "geotiffhandler.h"

class GeoTiffImageProvider : public QQuickImageProvider
{
public:
    GeoTiffImageProvider();

    // QQuickImageProvider interface
public:
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    GeoTiffHandler m_geoTiffHandler;
};

#endif // GEOTIFFIMAGEPROVIDER_H

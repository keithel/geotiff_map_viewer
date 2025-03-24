#ifndef GEOTIFFOVERLAY_H
#define GEOTIFFOVERLAY_H

#include <QQmlEngine>
#include <QQuickItem>
#include <QImage>
#include <QGeoCoordinate>
#include <gdal_priv.h>


class GeoTiffOverlay : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString geoTiffPath READ geoTiffPath WRITE setGeoTiffPath NOTIFY geoTiffPathChanged)
    Q_PROPERTY(QObject* map READ map WRITE setMap NOTIFY mapChanged)

public:
    GeoTiffOverlay(QQuickItem *parent = nullptr);

    ~GeoTiffOverlay();

    inline QString geoTiffPath() const { return m_geoTiffPath; }
    inline QObject* map() const { return m_map; }

    void setGeoTiffPath(const QString &path);

    void setMap(QObject *map);

signals:
    void geoTiffPathChanged();
    void mapChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private slots:
    void loadGeoTiff();

    void updateTransform();

    QPointF geoToPixel(const QGeoCoordinate &coord);

private:
    QString m_geoTiffPath;
    QObject* m_map;
    GDALDataset* m_dataset = nullptr;
    QList<double> m_geoTransform;
    OGRCoordinateTransformation* m_coordTransform = nullptr;
    QImage m_transformedImage;
};

#endif // GEOTIFFOVERLAY_H

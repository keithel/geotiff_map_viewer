#ifndef GEOTIFFOVERLAY_H
#define GEOTIFFOVERLAY_H

#include <QQmlEngine>
#include <QQuickItem>
#include <QImage>
#include <QGeoCoordinate>
#include <gdal_priv.h>

class QDeclarativeGeoMap;

class GeoTiffOverlay : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

public:
    GeoTiffOverlay(QQuickItem *parent = nullptr);
    ~GeoTiffOverlay();

    inline QString source() const { return m_source; }
    void setSource(const QString &source);

signals:
    void sourceChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
    void updateTransform();
    void transformImage();
    QPointF geoToPixel(const QGeoCoordinate &coord);

private slots:
    void loadSource();

private:
    QDeclarativeGeoMap *m_map = nullptr;
    QString m_source;
    GDALDataset* m_dataset = nullptr;
    QList<double> m_geoTransform;
    OGRCoordinateTransformation* m_coordTransform = nullptr;
    bool m_dirty = true;
    QImage m_transformedImage;
};

#endif // GEOTIFFOVERLAY_H

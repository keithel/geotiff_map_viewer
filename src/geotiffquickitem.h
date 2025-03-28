#ifndef GEOTIFFQUICKITEM_H
#define GEOTIFFQUICKITEM_H

#include <QQmlEngine>
#include <QQuickItem>
#include <QImage>
#include <QGeoCoordinate>
#include <memory>
#include <gdal_priv.h>

class QDeclarativeGeoMap;

class GeoTiffQuickItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

public:
    GeoTiffQuickItem(QQuickItem *parent = nullptr);
    ~GeoTiffQuickItem();

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
    std::unique_ptr<GDALDataset> m_dataset;
    std::vector<double> m_geoTransform;
    std::unique_ptr<OGRCoordinateTransformation> m_coordTransform;
    bool m_dirty = true;
    QImage m_transformedImage;
};

#endif // GEOTIFFQUICKITEM_H

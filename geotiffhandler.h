#ifndef GEOTIFFHANDLER_H
#define GEOTIFFHANDLER_H

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QImage>
#include <QUrl>
#include <QStringList>
#include <gdal_priv.h>
#include <gdal.h>

class GeoTiffHandler : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged FINAL)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileNameChanged FINAL)
    Q_PROPERTY(QString dimensions READ dimensions NOTIFY dimensionsChanged FINAL)
    Q_PROPERTY(QString coordinateSystem READ coordinateSystem NOTIFY coordinateSystemChanged FINAL)
    Q_PROPERTY(QString projection READ projection NOTIFY projectionChanged FINAL)
    Q_PROPERTY(QString boundsMinX READ boundsMinX NOTIFY boundsChanged FINAL)
    Q_PROPERTY(QString boundsMinY READ boundsMinY NOTIFY boundsChanged FINAL)
    Q_PROPERTY(QString boundsMaxX READ boundsMaxX NOTIFY boundsChanged FINAL)
    Q_PROPERTY(QString boundsMaxY READ boundsMaxY NOTIFY boundsChanged FINAL)
    Q_PROPERTY(QStringList bandsModel READ bandsModel NOTIFY bandsModelChanged FINAL)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)
public:
    explicit GeoTiffHandler(QObject *parent = nullptr);
    ~GeoTiffHandler();

    QImage loadGeoTiffImage(const QUrl &fileUrl);
    Q_INVOKABLE void loadMetadata(const QUrl &fileUrl);

    inline QString currentFile() const { return m_currentFile; }
    inline QString fileName() const { return m_fileName; }
    inline QString dimensions() const { return m_dimensions; }
    inline QString coordinateSystem() const { return m_coordinateSystem; }
    inline QString projection() const { return m_projection; }
    inline QString boundsMinX() const { return m_boundsMinX; }
    inline QString boundsMinY() const { return m_boundsMinY; }
    inline QString boundsMaxX() const { return m_boundsMaxX; }
    inline QString boundsMaxY() const { return m_boundsMaxY; }
    inline QStringList bandsModel() const { return m_bandsModel; }
    inline QString statusMessage() const { return m_statusMessage; }

signals:
    void currentFileChanged();
    void fileNameChanged();
    void dimensionsChanged();
    void coordinateSystemChanged();
    void projectionChanged();
    void boundsChanged();
    void bandsModelChanged();
    void statusMessageChanged();

private:
    GDALDataset* openGeoTiff(const QUrl &fileUrl);
    void closeDataset();
    void extractMetadata();
    QImage exportToQImage(GDALDatasetH dataset);

private:
    GDALDataset *m_dataset = nullptr;
    QString m_currentFile;
    QString m_fileName;
    QString m_dimensions;
    QString m_coordinateSystem;
    QString m_projection;
    QString m_boundsMinX;
    QString m_boundsMinY;
    QString m_boundsMaxX;
    QString m_boundsMaxY;
    QStringList m_bandsModel;
    QString m_statusMessage;
};

#endif // GEOTIFFHANDLER_H

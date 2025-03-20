#include "geotiffhandler.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QImage>
#include <QDebug>

// Public GDAL headers
#include <gdal.h>
#include <gdal_version.h>
#include <ogr_core.h>
#include <ogr_srs_api.h>
#include <cpl_conv.h>

GeoTiffHandler::GeoTiffHandler(QObject *parent)
    : QObject{parent}
    , m_statusMessage{"Ready"}
{
    GDALAllRegister();
}

GeoTiffHandler::~GeoTiffHandler()
{
    closeDataset();
}

GeoTiffHandler *GeoTiffHandler::instance() {
    if (s_singletonInstance == nullptr)
        s_singletonInstance = new GeoTiffHandler(qApp);
    return s_singletonInstance;
}

GeoTiffHandler *GeoTiffHandler::create(QQmlEngine *, QJSEngine *engine)
{
    Q_ASSERT(s_singletonInstance);
    Q_ASSERT(engine->thread() == s_singletonInstance->thread());
    if (s_engine)
        Q_ASSERT(engine == s_engine);
    else
        s_engine = engine;

    QJSEngine::setObjectOwnership(s_singletonInstance, QJSEngine::CppOwnership);
    return s_singletonInstance;
}

QImage GeoTiffHandler::loadGeoTiffImage(const QUrl &fileUrl)
{
    closeDataset();
    m_dataset = openGeoTiff(fileUrl);
    QImage image(exportToQImage(m_dataset));
    m_statusMessage = image.isNull() ? "Failed to load GeoTiff into QImage" : "GeoTiff loaded into QImage successfully";
    emit statusMessageChanged();
    return image;
}

void GeoTiffHandler::loadMetadata(const QUrl &fileUrl)
{
    if(fileUrl.toLocalFile() != m_currentFile) {
        closeDataset();
        m_dataset = openGeoTiff(fileUrl);
        if (m_dataset == nullptr)
            return;
    }

    // Extract metadata
    extractMetadata();

    m_statusMessage = "GeoTiff metadata loaded successfully.";
    emit statusMessageChanged();
}

GDALDataset *GeoTiffHandler::openGeoTiff(const QUrl &fileUrl)
{
    // Convert QUrl to local file path
    QString filePath = fileUrl.toLocalFile();

    // Set current file information
    m_currentFile = filePath;
    m_fileName = QFileInfo(filePath).fileName();
    emit currentFileChanged();
    emit fileNameChanged();

    // Update status
    m_statusMessage = "Loading " + m_fileName + "...";
    emit statusMessageChanged();

    // Open GeoTIFF file with GDAL public API
    CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");
    GDALDataset *dataset = static_cast<GDALDataset*>(GDALOpen(filePath.toUtf8().constData(), GA_ReadOnly));

    if (!dataset) {
        m_statusMessage = "Failed to open GeoTIFF file";
        emit statusMessageChanged();
        return nullptr;
    }

    return dataset;
}

void GeoTiffHandler::closeDataset()
{
    if (m_dataset) {
        GDALClose(m_dataset);
        m_dataset = nullptr;
    }
}

void GeoTiffHandler::extractMetadata()
{
    if (!m_dataset)
        return;

    // Get image dimensions
    int width = GDALGetRasterXSize(m_dataset);
    int height = GDALGetRasterYSize(m_dataset);
    m_dimensions = QString("%1 × %2").arg(width).arg(height);
    emit dimensionsChanged();

    // Get coordinate system
    const char* projWkt = GDALGetProjectionRef(m_dataset);
    if (projWkt && strlen(projWkt) > 0) {
        OGRSpatialReferenceH srs = OSRNewSpatialReference(projWkt);

        // Extract projection info
        const char* projName = OSRGetAttrValue(srs, "PROJECTION", 0);
        m_projection = projName ? QString(projName) : "Unknown";
        emit projectionChanged();

        // Get coordinate system name
        char *pszPrettyWkt = nullptr;
        OSRExportToPrettyWkt(srs, &pszPrettyWkt, 0);
        if (pszPrettyWkt) {
            m_coordinateSystem = QString(pszPrettyWkt);
            CPLFree(pszPrettyWkt);
        } else {
            m_coordinateSystem = QString(projWkt);
        }
        OSRDestroySpatialReference(srs);
        emit coordinateSystemChanged();
    } else {
        m_projection = "None";
        m_coordinateSystem = "None";
        emit projectionChanged();
        emit coordinateSystemChanged();
    }

    // Get geospatial bounds
    double geoTransform[6];
    if (GDALGetGeoTransform(m_dataset, geoTransform) == CE_None) {
        double minX = geoTransform[0];
        double maxY = geoTransform[3];
        double maxX = minX + geoTransform[1] * width;
        double minY = maxY + geoTransform[5] * height;

        m_boundsMinX = QString::number(minX, 'f', 6);
        m_boundsMinY = QString::number(minY, 'f', 6);
        m_boundsMaxX = QString::number(maxX, 'f', 6);
        m_boundsMaxY = QString::number(maxY, 'f', 6);
        emit boundsChanged();
    } else {
        m_boundsMinX = "Unknown";
        m_boundsMinY = "Unknown";
        m_boundsMaxX = "Unknown";
        m_boundsMaxY = "Unknown";
        emit boundsChanged();
    }

    // Get band information
    m_bandsModel.clear();
    int bandCount = GDALGetRasterCount(m_dataset);
    for (int i = 1; i <= bandCount; i++) {
        GDALRasterBandH band = GDALGetRasterBand(m_dataset, i);
        if (band) {
            QString bandInfo = QString("Band %1: ").arg(i);

            // Get data type
            GDALDataType dataType = GDALGetRasterDataType(band);
            bandInfo += QString("Type: %1, ").arg(GDALGetDataTypeName(dataType));

            // Get color interpretation
            GDALColorInterp colorInterp = GDALGetRasterColorInterpretation(band);
            bandInfo += QString("Color: %1, ").arg(GDALGetColorInterpretationName(colorInterp));

            // Get other band metadata
            int blockXSize, blockYSize;
            GDALGetBlockSize(band, &blockXSize, &blockYSize);
            bandInfo += QString("Block: %1x%2").arg(blockXSize).arg(blockYSize);

            // Add to model
            m_bandsModel.append(bandInfo);
        }
    }
    emit bandsModelChanged();
}

QImage GeoTiffHandler::exportToQImage(GDALDatasetH dataset)
{
    if (!dataset)
        return QImage();

    int width = GDALGetRasterXSize(dataset);
    int height = GDALGetRasterYSize(dataset);
    int bandCount = GDALGetRasterCount(dataset);

    QImage image;
    // For simplicity, we will assume 3 bands (RGB) or convert to RGB
    if (bandCount >= 3) {
        GDALRasterBandH redBand = GDALGetRasterBand(dataset, 1);
        GDALRasterBandH greenBand = GDALGetRasterBand(dataset, 2);
        GDALRasterBandH blueBand = GDALGetRasterBand(dataset, 3);

        // Check if bands are valid
        if (!redBand || !greenBand || !blueBand) {
            return QImage();
        }

        // Allocate mem for band data
        std::vector<uint8_t> redData(width*height);
        std::vector<uint8_t> greenData(width*height);
        std::vector<uint8_t> blueData(width*height);

        CPLErr err = GDALRasterIO(redBand, GF_Read, 0, 0, width, height, redData.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return QImage();
        err = GDALRasterIO(greenBand, GF_Read, 0, 0, width, height, greenData.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return QImage();
        err = GDALRasterIO(blueBand, GF_Read, 0, 0, width, height, greenData.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return QImage();

        image = QImage(width, height, QImage::Format_RGB32);
        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                int index = y * width + x;
                image.setPixel(x, y, qRgb(redData[index], greenData[index], blueData[index]));
            }
        }
    } else if (bandCount == 1) {
        // Single band - grayscale
        GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
        if (!band) {
            return QImage();
        }

        // Allocate memory for band data
        std::vector<uint8_t> data(width*height);

        // Read band data
        CPLErr err = GDALRasterIO(band, GF_Read, 0, 0, width, height, data.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return QImage();

        image = QImage(width, height, QImage::Format_RGB32);
        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                int index = y * width + x;
                uint8_t value = data[index];
                image.setPixel(x, y, qRgb(value, value, value));
            }
        }
    } else {
        // Unsupported band count
        return QImage();
    }

    return image;
}

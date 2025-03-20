#include "geotiffhandler.h"
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

void GeoTiffHandler::loadGeoTIFF(const QUrl &fileUrl)
{
    closeDataset();

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
    m_dataset = static_cast<GDALDataset*>(GDALOpen(filePath.toUtf8().constData(), GA_ReadOnly));

    if (!m_dataset) {
        m_statusMessage = "Failed to open GeoTIFF file";
        emit statusMessageChanged();
        return;
    }

    // Extract metadata
    extractMetadata();

    // Export to QImage for display
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                       QDir::separator() + "geotiff_preview.png";

    if (exportToQImage(tempPath)) {
        m_imageSource = "file:///" + tempPath;
        emit imageSourceChanged();
        m_statusMessage = "GeoTIFF loaded successfully";
    } else {
        m_statusMessage = "Failed to generate preview image";
    }

    emit statusMessageChanged();
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

bool GeoTiffHandler::exportToQImage(const QString &outputPath)
{
    if (!m_dataset)
        return false;

    int width = GDALGetRasterXSize(m_dataset);
    int height = GDALGetRasterYSize(m_dataset);
    int bandCount = GDALGetRasterCount(m_dataset);

    // Create a QImage with the same dimensions
    QImage image(width, height, QImage::Format_RGB32);

    // For simplicity, we will assume 3 bands (RGB) or convert to RGB
    if (bandCount >= 3) {
        GDALRasterBandH redBand = GDALGetRasterBand(m_dataset, 1);
        GDALRasterBandH greenBand = GDALGetRasterBand(m_dataset, 2);
        GDALRasterBandH blueBand = GDALGetRasterBand(m_dataset, 3);

        // Check if bands are valid
        if (!redBand || !greenBand || !blueBand) {
            return false;
        }

        // Allocate mem for band data
        std::vector<uint8_t> redData(width*height);
        std::vector<uint8_t> greenData(width*height);
        std::vector<uint8_t> blueData(width*height);

        CPLErr err = GDALRasterIO(redBand, GF_Read, 0, 0, width, height, redData.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return false;
        err = GDALRasterIO(greenBand, GF_Read, 0, 0, width, height, greenData.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return false;
        err = GDALRasterIO(blueBand, GF_Read, 0, 0, width, height, greenData.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return false;

        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                int index = y * width + x;
                image.setPixel(x, y, qRgb(redData[index], greenData[index], blueData[index]));
            }
        }
    } else if (bandCount == 1) {
        // Single band - grayscale
        GDALRasterBandH band = GDALGetRasterBand(m_dataset, 1);
        if (!band) {
            return false;
        }

        // Allocate memory for band data
        std::vector<uint8_t> data(width*height);

        // Read band data
        CPLErr err = GDALRasterIO(band, GF_Read, 0, 0, width, height, data.data(), width, height, GDT_Byte, 0, 0);
        if (err > CPLErr::CE_Warning)
            return false;

        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                int index = y * width + x;
                uint8_t value = data[index];
                image.setPixel(x, y, qRgb(value, value, value));
            }
        }
    } else {
        // Unsupported band count
        return false;
    }

    return image.save(outputPath);
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "geotiffimageprovider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QLatin1String("geotiff"), new GeoTiffImageProvider);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("geotiff_viewer", "Main");

    return app.exec();
}

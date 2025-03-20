#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "appconfig.h"
#include "thunderforestconfigserver.h"
#include "geotiffimageprovider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    AppConfig *appConfig = AppConfig::instance();

    ThunderForestConfigServer *mapConfigServer = new ThunderForestConfigServer(appConfig->thunderforestApiKey(), &app);
    mapConfigServer->listen();
    appConfig->setOsmMappingProvidersRepositoryAddress(QString("http://localhost:%1/").arg(mapConfigServer->serverPort()));
    qDebug() << "osmMappingProvidersRepositoryAddress" << appConfig->osmMappingProvidersRepositoryAddress();

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

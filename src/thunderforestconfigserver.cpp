#include "thunderforestconfigserver.h"

#include <QTcpServer>
#include <QJsonObject>

std::map<QString, QString> s_osmToThunderforestMapNames = { {"street", "atlas"}, {"satellite", ""}, { "cycle", "cycle" }, {"transit", "transport"}, {"night-transit", "transport-dark"}, {"terrain", "outdoors"}, {"hiking", "outdoors"} };

QJsonObject createOsmJson(const QString &apiKey, const QString &mapType) {
    QJsonObject json;
    json["UrlTemplate"] = QString("https://tile.thunderforest.com/%2/%z/%x/%y.png?apikey=%1").arg(apiKey, mapType);
    json["ImageFormat"] = "png";
    json["QImageFormat"] = "Indexed8";
    json["ID"] = QString("thf-%1").arg(mapType);
    json["MaximumZoomLevel"] = 20;
    json["MapCopyRight"] = "<a href='https://www.thunderforest.com/'>Thunderforest</a>";
    json["DataCopyRight"] = "<a href='https://www.openstreetmap.org/copyright'>OpenStreetMap</a> contributors";
    return json;
}

ThunderForestConfigServer::ThunderForestConfigServer(const QString &apiKey, QObject *parent)
    : QAbstractHttpServer{parent}
    , m_apiKey{apiKey}
{}

bool ThunderForestConfigServer::listen()
{
    m_tcpServer = new QTcpServer(this);
    if(!m_tcpServer->listen(QHostAddress::Any, 0) || !bind(m_tcpServer)) {
        delete m_tcpServer;
        m_tcpServer = nullptr;
        return false;
    }
    return true;
}

quint16 ThunderForestConfigServer::serverPort()
{
    QList<quint16> serverPorts = this->serverPorts();
    return serverPorts.first();
}

bool ThunderForestConfigServer::handleRequest(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    QString path = request.url().path();
    if(path == "/") {
        // httpServer.route("/", []() {
        qDebug() << "Request for /";
        responder.write();
        return true;
    }

    for (auto &mapType : s_osmToThunderforestMapNames)
    {
        QString targetPath = QString("/%1").arg(mapType.first);
        // httpServer.route(QString("/%1").arg(mapType.first), [mapType, appConfig]() {
        if(path == targetPath) {
            qDebug().nospace().noquote() << "Request for /" << mapType.first << " mapping to thunderforest tileset " << mapType.second;
            QJsonDocument doc(createOsmJson(m_apiKey, mapType.second));
            responder.write(doc);
            return true;
        }
    }
    return false;
}

void ThunderForestConfigServer::missingHandler(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    qDebug() << "Missing" << request.url();
    responder.write(QHttpServerResponder::StatusCode::NotFound);
}

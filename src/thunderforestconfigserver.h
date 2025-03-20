#ifndef THUNDERFORESTCONFIGSERVER_H
#define THUNDERFORESTCONFIGSERVER_H

#include <QAbstractHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponder>

class QTcpServer;

class ThunderForestConfigServer : public QAbstractHttpServer
{
public:
    explicit ThunderForestConfigServer(const QString &apiKey = QString(), QObject *parent = nullptr);
    bool listen();
    quint16 serverPort();

    // QAbstractHttpServer interface
protected:
    bool handleRequest(const QHttpServerRequest &request, QHttpServerResponder &responder);
    void missingHandler(const QHttpServerRequest &request, QHttpServerResponder &responder);

private:
    QTcpServer *m_tcpServer;
    QString m_apiKey;
};

#endif // THUNDERFORESTCONFIGSERVER_H

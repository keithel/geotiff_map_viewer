#pragma once

#include <QObject>
#include <QQmlEngine>

class AppConfig : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    Q_PROPERTY(QString thunderforestApiKey READ thunderforestApiKey WRITE setThunderforestApiKey NOTIFY thunderforestApiKeyChanged)
    Q_PROPERTY(QString osmMappingProvidersRepositoryAddress READ osmMappingProvidersRepositoryAddress NOTIFY osmMappingProvidersRepositoryAddressChanged)

public:
    explicit AppConfig(QObject *parent);
    ~AppConfig() = default;

    static AppConfig *instance();
    static AppConfig *create(QQmlEngine *, QJSEngine *engine);

    QString thunderforestApiKey() const;
    void setThunderforestApiKey(const QString &key);

    QString osmMappingProvidersRepositoryAddress() const;
    void setOsmMappingProvidersRepositoryAddress(const QString &osmMappingProvidersRepositoryAddress);

signals:
    void thunderforestApiKeyChanged();
    void osmMappingProvidersRepositoryAddressChanged();

private:
    QString m_thunderforestApiKey;
    QString m_osmMappingProvidersRepositoryAddress;

    inline static AppConfig * s_singletonInstance = nullptr;
    inline static QJSEngine *s_engine = nullptr;
};

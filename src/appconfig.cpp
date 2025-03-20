#include "appconfig.h"
#include <QtCore/QCoreApplication>
#include <QCommandLineParser>

bool isValidKeyFormat(const QString& key) {
    if (key.length() != 32)
        return false;

    for(auto c : key.toLower())
        if((c < '0' || c > '9') && (c < 'a' && c > 'f'))
            return false;

    return true;
}

AppConfig::AppConfig(QObject *parent)
    : QObject{parent}
{
    // I do know that QCoreApplication is around, as this is a QML singleton created by the qml
    // engine, which requires that the application be created before it is instantiated.
    qApp->arguments();

    QCommandLineParser parser;
    QCommandLineOption apiKeyOption(QStringList({"k", "apiKey"}), "Thunderforest map API key", "api-key");
    parser.addOption(apiKeyOption);
    if(!parser.parse(qApp->arguments())) {
        qFatal() << "Failed to read command line arguments. aborting";
    }

    QString apiKey = parser.value(apiKeyOption);
    if (!apiKey.isEmpty()) {
        if (!isValidKeyFormat(apiKey))
            qFatal() << "Thunderforest map API key given is not a valid key";
        setThunderforestApiKey(apiKey);
    }
}

AppConfig *AppConfig::instance() {
    if (s_singletonInstance == nullptr)
        s_singletonInstance = new AppConfig(qApp);
    return s_singletonInstance;
}

AppConfig *AppConfig::create(QQmlEngine *, QJSEngine *engine)
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

QString AppConfig::thunderforestApiKey() const
{
    return m_thunderforestApiKey;
}

void AppConfig::setThunderforestApiKey(const QString &key)
{
    if (m_thunderforestApiKey != key) {
        m_thunderforestApiKey = key;
        emit thunderforestApiKeyChanged();
    }
}

QString AppConfig::osmMappingProvidersRepositoryAddress() const
{
    return m_osmMappingProvidersRepositoryAddress;
}

void AppConfig::setOsmMappingProvidersRepositoryAddress(const QString &osmMappingProvidersRepositoryAddress)
{
    if (m_osmMappingProvidersRepositoryAddress != osmMappingProvidersRepositoryAddress) {
        m_osmMappingProvidersRepositoryAddress = osmMappingProvidersRepositoryAddress;
        emit osmMappingProvidersRepositoryAddressChanged();
    }
}

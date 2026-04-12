#pragma once

#include <QString>
#include <QUrl>

class OIDCConfig
{
public:
    OIDCConfig();

    // Configuration setters
    void setAuthorizationUrl(const QString &url);
    void setTokenUrl(const QString &url);
    void setClientId(const QString &id);
    void setClientSecret(const QString &secret);
    void setRedirectUri(const QString &uri);
    void setScope(const QString &scope);

    // Configuration getters
    QString authorizationUrl() const { return m_authorizationUrl; }
    QString tokenUrl() const { return m_tokenUrl; }
    QString clientId() const { return m_clientId; }
    QString clientSecret() const { return m_clientSecret; }
    QString redirectUri() const { return m_redirectUri; }
    QString scope() const { return m_scope; }

    // Get authorization endpoint with parameters
    QUrl getAuthorizationEndpoint() const;
    
    // Generate a random state parameter for security
    static QString generateState();

private:
    QString m_authorizationUrl;
    QString m_tokenUrl;
    QString m_clientId;
    QString m_clientSecret;
    QString m_redirectUri;
    QString m_scope;
};

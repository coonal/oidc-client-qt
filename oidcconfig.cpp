#include "oidcconfig.h"
#include <QUrlQuery>
#include <QUuid>

OIDCConfig::OIDCConfig()
    : m_scope("openid profile email")
{
}

void OIDCConfig::setAuthorizationUrl(const QString &url)
{
    m_authorizationUrl = url;
}

void OIDCConfig::setTokenUrl(const QString &url)
{
    m_tokenUrl = url;
}

void OIDCConfig::setClientId(const QString &id)
{
    m_clientId = id;
}

void OIDCConfig::setClientSecret(const QString &secret)
{
    m_clientSecret = secret;
}

void OIDCConfig::setRedirectUri(const QString &uri)
{
    m_redirectUri = uri;
}

void OIDCConfig::setScope(const QString &scope)
{
    m_scope = scope;
}

QUrl OIDCConfig::getAuthorizationEndpoint() const
{
    QUrl url(m_authorizationUrl);
    QUrlQuery query;
    
    query.addQueryItem("response_type", "code");
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("scope", m_scope);
    query.addQueryItem("state", generateState());
    
    url.setQuery(query);
    return url;
}

QString OIDCConfig::generateState()
{
    return QUuid::createUuid().toString();
}

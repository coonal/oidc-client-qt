#pragma once

#include <QDialog>
#include <QWebEngineView>
#include <memory>
#include "oidcconfig.h"

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(const OIDCConfig &config, QWidget *parent = nullptr);
    ~LoginDialog();

    // Get authorization code from the callback
    QString getAuthorizationCode() const { return m_authorizationCode; }
    
    // Get state parameter for verification
    QString getState() const { return m_state; }

signals:
    void loginSucceeded(const QString &authCode, const QString &state);
    void loginFailed(const QString &error);

private slots:
    void onUrlChanged(const QUrl &url);
    void onLoadFinished(bool success);

private:
    void parseRedirectUrl(const QUrl &url);
    
    std::unique_ptr<QWebEngineView> m_webView;
    OIDCConfig m_config;
    QString m_authorizationCode;
    QString m_state;
    QString m_expectedState;
};

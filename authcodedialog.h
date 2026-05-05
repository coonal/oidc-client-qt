#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>
#include "oidcconfig.h"

class AuthCodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthCodeDialog(const OIDCConfig &config, QWidget *parent = nullptr);
    ~AuthCodeDialog();

    // Get the access token from successful authentication
    QString getAccessToken() const { return m_accessToken; }
    
    // Get the ID token (if available)
    QString getIdToken() const { return m_idToken; }

signals:
    void loginSucceeded(const QString &accessToken);
    void loginFailed(const QString &error);

private slots:
    void onOpenBrowserClicked();
    void onExchangeCodeClicked();
    void onTokenResponseFinished();

private:
    void createUI();
    void exchangeAuthorizationCode(const QString &code);
    void copyAuthUrlToClipboard();
    
    QLabel *m_instructionsLabel;
    QLineEdit *m_authCodeEdit;
    QPushButton *m_openBrowserButton;
    QPushButton *m_exchangeButton;
    QPushButton *m_cancelButton;
    
    OIDCConfig m_config;
    QString m_generatedState;
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QNetworkReply *m_currentReply;
    
    QString m_accessToken;
    QString m_idToken;
};

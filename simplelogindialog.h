#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>
#include "oidcconfig.h"

class SimpleLoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SimpleLoginDialog(const OIDCConfig &config, QWidget *parent = nullptr);
    ~SimpleLoginDialog();

    // Get the access token from successful authentication
    QString getAccessToken() const { return m_accessToken; }
    
    // Get the ID token (if available)
    QString getIdToken() const { return m_idToken; }

signals:
    void loginSucceeded(const QString &accessToken);
    void loginFailed(const QString &error);

private slots:
    void onLoginClicked();
    void onTokenResponseFinished();
    void onNetworkError();

private:
    void createUI();
    void authenticateWithCredentials(const QString &username, const QString &password);
    
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QPushButton *m_cancelButton;
    
    OIDCConfig m_config;
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QNetworkReply *m_currentReply;
    
    QString m_accessToken;
    QString m_idToken;
};

#pragma once

#include <QMainWindow>
#include <memory>
#include "oidcconfig.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class LoginDialog;
class QNetworkAccessManager;
class QNetworkReply;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onLoginSucceeded(const QString &authCode, const QString &state);
    void onLoginFailed(const QString &error);
    void onTokenExchangeFinished();

private:
    void initializeOIDCConfig();
    bool loadConfigFromFile();
    void showLoginDialog();
    void grantApplicationAccess();
    void exchangeAuthCodeForTokens(const QString &authCode);

    Ui::MainWindow *ui;
    std::unique_ptr<OIDCConfig> m_oidcConfig;
    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_tokenExchangeReply = nullptr;
    bool m_isAuthenticated = false;
    bool m_loginAttempted = false;
    QString m_accessToken;
    QString m_refreshToken;
    QString m_idToken;
};

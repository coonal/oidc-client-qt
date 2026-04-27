#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QShowEvent>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QFileInfo>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // ui->setupUi(this);
    setWindowTitle("OIDC Client");
    setGeometry(300, 300, 800, 800);

    // Initialize OIDC configuration
    initializeOIDCConfig();

    // Initialize network manager for token exchange
    m_networkManager = new QNetworkAccessManager(this);

    // Setup main window UI for authenticated users
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    QLabel *welcomeLabel = new QLabel("Welcome to the application!", this);
    layout->addWidget(welcomeLabel);
    layout->addStretch();

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event)
{
    // Show login dialog on first show if not already attempted
    if (!m_loginAttempted) {
        m_loginAttempted = true;
        QTimer::singleShot(0, this, &MainWindow::showLoginDialog);
    }
    QMainWindow::showEvent(event);
}

void MainWindow::initializeOIDCConfig()
{
    m_oidcConfig = std::unique_ptr<OIDCConfig>(new OIDCConfig);

    // Try to load configuration from file
    if (loadConfigFromFile()) {
        qDebug() << "OIDC configuration loaded from config.ini";
        return;
    }

    // Fallback to default values if config file not found
    qWarning() << "Config file not found, using default values";

    m_oidcConfig->setAuthorizationUrl("https://auth.example.com/oauth/authorize");
    m_oidcConfig->setTokenUrl("https://auth.example.com/oauth/token");
    m_oidcConfig->setClientId("your-client-id");
    m_oidcConfig->setClientSecret("your-client-secret");
    m_oidcConfig->setRedirectUri("http://localhost:8080/callback");
    m_oidcConfig->setScope("openid profile email");
}

bool MainWindow::loadConfigFromFile()
{
    // Look for config.ini in the application directory
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";

    // Also check one level up (useful for development)
    if (!QFileInfo::exists(configPath)) {
        configPath = QCoreApplication::applicationDirPath() + "/../config.ini";
    }

    qDebug() << "Looking for config file at:" << configPath;

    if (!QFileInfo::exists(configPath)) {
        qDebug() << "Config file not found at:" << configPath;
        return false;
    }

    // Load configuration using QSettings
    QSettings settings(configPath, QSettings::IniFormat);

    // Verify the OIDC section exists
    if (!settings.contains("OIDC/AuthorizationUrl")) {
        qWarning() << "Invalid config file: missing [OIDC] section";
        return false;
    }

    // Read settings with defaults
    QString authUrl = settings.value("OIDC/AuthorizationUrl", "").toString();
    QString tokenUrl = settings.value("OIDC/TokenUrl", "").toString();
    QString clientId = settings.value("OIDC/ClientId", "").toString();
    QString clientSecret = settings.value("OIDC/ClientSecret", "").toString();
    QString redirectUri = settings.value("OIDC/RedirectUri", "").toString();
    QString scope = settings.value("OIDC/Scope", "openid profile email").toString();

    // Validate required fields
    if (authUrl.isEmpty() || tokenUrl.isEmpty() || clientId.isEmpty() || redirectUri.isEmpty()) {
        qCritical() << "Config file missing required OIDC parameters";
        return false;
    }

    // Apply configuration
    m_oidcConfig->setAuthorizationUrl(authUrl);
    m_oidcConfig->setTokenUrl(tokenUrl);
    m_oidcConfig->setClientId(clientId);
    m_oidcConfig->setClientSecret(clientSecret);
    m_oidcConfig->setRedirectUri(redirectUri);
    m_oidcConfig->setScope(scope);

    qDebug() << "Configuration loaded successfully:";
    qDebug() << "  Authorization URL:" << authUrl;
    qDebug() << "  Token URL:" << tokenUrl;
    qDebug() << "  Client ID:" << clientId;
    qDebug() << "  Redirect URI:" << redirectUri;
    qDebug() << "  Scope:" << scope;

    return true;
}

void MainWindow::showLoginDialog()
{
    if (!m_oidcConfig) {
        QMessageBox::critical(this, "Configuration Error",
                            "OIDC configuration not initialized");
        return;
    }

    qDebug() << "Showing login dialog";

    LoginDialog loginDialog(*m_oidcConfig, this);

    // Connect signals
    connect(&loginDialog, &LoginDialog::loginSucceeded,
            this, &MainWindow::onLoginSucceeded);
    connect(&loginDialog, &LoginDialog::loginFailed,
            this, &MainWindow::onLoginFailed);

    int result = loginDialog.exec();
    // connect(&loginDialog, &QDialog::rejected, [&] {
        if (result != QDialog::Accepted && !m_isAuthenticated) {
            // Login was cancelled or failed
            qDebug() << "Login failed or cancelled";
            close();
        }
    // });
}

void MainWindow::onLoginSucceeded(const QString &authCode, const QString &state)
{
    qDebug() << "Login succeeded with auth code:" << authCode;

    // Exchange the authorization code for tokens
    exchangeAuthCodeForTokens(authCode);
}

void MainWindow::onLoginFailed(const QString &error)
{
    qDebug() << "Login failed:" << error;

    QMessageBox::critical(this, "Login Failed",
        QString("Authentication failed:\n%1").arg(error));

    close();
}

void MainWindow::exchangeAuthCodeForTokens(const QString &authCode)
{
    if (!m_oidcConfig) {
        onLoginFailed("OIDC configuration not available");
        return;
    }

    qDebug() << "Exchanging authorization code for tokens...";

    // Create POST request to token endpoint
    QUrl tokenUrl(m_oidcConfig->tokenUrl());
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Prepare request body with auth code, client id, and secret
    QByteArray body = m_oidcConfig->getTokenExchangeBody(authCode);

    qDebug() << "Token URL:" << m_oidcConfig->tokenUrl();
    qDebug() << "Sending token exchange request...";

    // Send POST request
    m_tokenExchangeReply = m_networkManager->post(request, body);

    // Connect signals for response handling
    connect(m_tokenExchangeReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, [this](QNetworkReply::NetworkError error) {
        qWarning() << "Token exchange error:" << error;
        qWarning() << "Response:" << m_tokenExchangeReply->errorString();
        onLoginFailed("Token exchange failed: " + m_tokenExchangeReply->errorString());
    });

    connect(m_tokenExchangeReply, &QNetworkReply::finished,
            this, &MainWindow::onTokenExchangeFinished);
}

void MainWindow::onTokenExchangeFinished()
{
    if (!m_tokenExchangeReply) {
        return;
    }

    // Check for network errors
    if (m_tokenExchangeReply->error() != QNetworkReply::NoError) {
        qWarning() << "Token exchange failed:" << m_tokenExchangeReply->errorString();
        onLoginFailed("Token exchange failed: " + m_tokenExchangeReply->errorString());
        m_tokenExchangeReply->deleteLater();
        m_tokenExchangeReply = nullptr;
        return;
    }

    // Parse JSON response
    QByteArray responseData = m_tokenExchangeReply->readAll();
    qDebug() << "Token response received";
    qDebug() << responseData;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        qWarning() << "Invalid token response format";
        onLoginFailed("Invalid token response format");
        m_tokenExchangeReply->deleteLater();
        m_tokenExchangeReply = nullptr;
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // Check for error in response
    if (jsonObj.contains("error")) {
        QString error = jsonObj.value("error").toString();
        QString errorDescription = jsonObj.value("error_description").toString();
        qWarning() << "Token endpoint error:" << error << "-" << errorDescription;
        onLoginFailed(QString("Token exchange failed: %1").arg(
            errorDescription.isEmpty() ? error : errorDescription));
        m_tokenExchangeReply->deleteLater();
        m_tokenExchangeReply = nullptr;
        return;
    }

    // Extract tokens from response
    if (!jsonObj.contains("access_token")) {
        qWarning() << "No access token in response";
        onLoginFailed("No access token in token response");
        m_tokenExchangeReply->deleteLater();
        m_tokenExchangeReply = nullptr;
        return;
    }

    m_accessToken = jsonObj.value("access_token").toString();
    m_refreshToken = jsonObj.value("refresh_token").toString();
    m_idToken = jsonObj.value("id_token").toString();

    int expiresIn = jsonObj.value("expires_in").toInt(3600); // Default to 1 hour

    qDebug() << "Tokens received successfully";
    qDebug() << "  Access Token: [" << m_accessToken << "]";
    qDebug() << "  Refresh Token: [" << m_refreshToken << "]";
    qDebug() << "  ID Token: [" << m_idToken << "]";
    qDebug() << "  Expires in:" << expiresIn << "seconds";

    m_isAuthenticated = true;

    // Display success message
    QMessageBox::information(this, "Login Successful",
        QString("Authentication successful!\n\n"
                "Tokens received:\n"
                "- Access Token: %1 chars\n"
                "- Refresh Token: %2 chars\n"
                "- ID Token: %3 chars\n"
                "- Expires in: %4 seconds\n\n"
                "You can now use the access token for API requests.").arg(
                    QString::number(m_accessToken.length()),
                    QString::number(m_refreshToken.length()),
                    QString::number(m_idToken.length()),
                    QString::number(expiresIn)));

    m_tokenExchangeReply->deleteLater();
    m_tokenExchangeReply = nullptr;

    grantApplicationAccess();
}

void MainWindow::grantApplicationAccess()
{
    // This method is called after successful authentication and token exchange
    // You can perform additional setup here, such as:
    // - Fetching user profile information
    // - Loading user preferences
    // - Initializing the application with user data

    qDebug() << "Application access granted";

    // The main window is already visible at this point
    // The UI was already set up in the constructor
    // Tokens are stored in m_accessToken, m_refreshToken, and m_idToken
}


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include "simplelogindialog.h"
#include "authcodedialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QShowEvent>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QFileInfo>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // ui->setupUi(this);
    setWindowTitle("OIDC Client");
    setGeometry(300, 300, 800, 800);
    
    // Initialize OIDC configuration
    initializeOIDCConfig();
    
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
        QTimer::singleShot(0, this, &MainWindow::showSimpleLoginDialog);
    }
    QMainWindow::showEvent(event);
}

void MainWindow::initializeOIDCConfig()
{
    m_oidcConfig = std::make_unique<OIDCConfig>();
    
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
    
    m_isAuthenticated = true;
    
    // Display success message
    QMessageBox::information(this, "Login Successful", 
        QString("Authorization code received.\n\n"
                "In a production application, you would now:\n"
                "1. Exchange this authorization code for tokens via the token endpoint\n"
                "2. Store the access and refresh tokens securely\n"
                "3. Use the access token for API requests\n\n"
                "Auth Code: %1").arg(authCode));
    
    grantApplicationAccess();
}

void MainWindow::onLoginFailed(const QString &error)
{
    qDebug() << "Login failed:" << error;
    
    QMessageBox::critical(this, "Login Failed", 
        QString("Authentication failed:\n%1").arg(error));
    
    close();
}

void MainWindow::showSimpleLoginDialog()
{
    if (!m_oidcConfig) {
        QMessageBox::critical(this, "Configuration Error", 
                            "OIDC configuration not initialized");
        return;
    }
    
    qDebug() << "Showing simple login dialog";
    
    SimpleLoginDialog loginDialog(*m_oidcConfig, this);
    
    // Connect signals - note: this uses access token instead of auth code
    connect(&loginDialog, &SimpleLoginDialog::loginSucceeded,
            [this](const QString &accessToken) {
                m_isAuthenticated = true;
                qDebug() << "Login succeeded with access token";
                
                QMessageBox::information(this, "Login Successful", 
                    "Authentication successful!\n\n"
                    "Access token received and ready for use.");
                
                grantApplicationAccess();
            });
    
    connect(&loginDialog, &SimpleLoginDialog::loginFailed,
            this, &MainWindow::onLoginFailed);
    
    int result = loginDialog.exec();
    if (result != QDialog::Accepted && !m_isAuthenticated) {
        qDebug() << "Login failed or cancelled";
        close();
    }
}

void MainWindow::showAuthCodeDialog()
{
    if (!m_oidcConfig) {
        QMessageBox::critical(this, "Configuration Error", 
                            "OIDC configuration not initialized");
        return;
    }
    
    qDebug() << "Showing authorization code dialog";
    
    AuthCodeDialog loginDialog(*m_oidcConfig, this);
    
    // Connect signals
    connect(&loginDialog, &AuthCodeDialog::loginSucceeded,
            [this](const QString &accessToken) {
                m_isAuthenticated = true;
                qDebug() << "Login succeeded with authorization code flow";
                
                QMessageBox::information(this, "Login Successful", 
                    "Authentication successful!\n\n"
                    "Access token received and ready for use.");
                
                grantApplicationAccess();
            });
    
    connect(&loginDialog, &AuthCodeDialog::loginFailed,
            this, &MainWindow::onLoginFailed);
    
    int result = loginDialog.exec();
    if (result != QDialog::Accepted && !m_isAuthenticated) {
        qDebug() << "Login failed or cancelled";
        close();
    }
}

void MainWindow::grantApplicationAccess()
{
    // This method is called after successful authentication
    // You can perform additional setup here, such as:
    // - Fetching user profile information
    // - Loading user preferences
    // - Initializing the application with user data
    
    qDebug() << "Application access granted";
    
    // The main window is already visible at this point
    // The UI was already set up in the constructor
}


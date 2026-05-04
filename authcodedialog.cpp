#include "authcodedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QClipboard>
#include <QApplication>
#include <QSslConfiguration>
#include <QSslSocket>

AuthCodeDialog::AuthCodeDialog(const OIDCConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
    , m_currentReply(nullptr)
{
    setWindowTitle("OIDC Authorization");
    setWindowModality(Qt::WindowModal);
    setGeometry(100, 100, 600, 400);
    
    m_networkManager = std::make_unique<QNetworkAccessManager>();
    m_generatedState = OIDCConfig::generateState();
    
    createUI();
}

AuthCodeDialog::~AuthCodeDialog()
{
    if (m_currentReply) {
        m_currentReply->deleteLater();
    }
}

void AuthCodeDialog::createUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Instructions
    m_instructionsLabel = new QLabel(
        "1. Click 'Open in Browser' to authorize\n"
        "2. Copy the authorization code from the URL\n"
        "3. Paste it below and click 'Exchange Code'\n\n"
        "Authorization URL copied to clipboard", this);
    m_instructionsLabel->setWordWrap(true);
    mainLayout->addWidget(m_instructionsLabel);
    
    // Authorization code input
    QHBoxLayout *codeLayout = new QHBoxLayout();
    QLabel *codeLabel = new QLabel("Auth Code:", this);
    m_authCodeEdit = new QLineEdit(this);
    m_authCodeEdit->setPlaceholderText("Paste authorization code here");
    codeLayout->addWidget(codeLabel);
    codeLayout->addWidget(m_authCodeEdit);
    mainLayout->addLayout(codeLayout);
    
    mainLayout->addStretch();
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_openBrowserButton = new QPushButton("Open in Browser", this);
    m_exchangeButton = new QPushButton("Exchange Code", this);
    m_cancelButton = new QPushButton("Cancel", this);
    
    connect(m_openBrowserButton, &QPushButton::clicked, this, &AuthCodeDialog::onOpenBrowserClicked);
    connect(m_exchangeButton, &QPushButton::clicked, this, &AuthCodeDialog::onExchangeCodeClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addWidget(m_openBrowserButton);
    buttonLayout->addWidget(m_exchangeButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
    
    // Automatically copy auth URL to clipboard and prepare it
    copyAuthUrlToClipboard();
}

void AuthCodeDialog::copyAuthUrlToClipboard()
{
    QUrl authUrl = m_config.getAuthorizationEndpoint();
    
    // Add state to URL for CSRF protection
    QUrlQuery query(authUrl.query());
    query.addQueryItem("state", m_generatedState);
    authUrl.setQuery(query);
    
    QString authUrlStr = authUrl.toString();
    
    // Copy to clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(authUrlStr);
    
    qDebug() << "Authorization URL copied to clipboard:" << authUrlStr;
}

void AuthCodeDialog::onOpenBrowserClicked()
{
    QUrl authUrl = m_config.getAuthorizationEndpoint();
    
    // Add state to URL for CSRF protection
    QUrlQuery query(authUrl.query());
    query.addQueryItem("state", m_generatedState);
    authUrl.setQuery(query);
    
    qDebug() << "Opening authorization URL:" << authUrl.toString();
    
    if (!QDesktopServices::openUrl(authUrl)) {
        QMessageBox::warning(this, "Error", 
            "Could not open browser. Please visit this URL manually:\n\n" + authUrl.toString());
    }
}

void AuthCodeDialog::onExchangeCodeClicked()
{
    QString code = m_authCodeEdit->text();
    
    if (code.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", 
                           "Please enter the authorization code");
        return;
    }
    
    m_exchangeButton->setEnabled(false);
    m_exchangeButton->setText("Exchanging...");
    m_openBrowserButton->setEnabled(false);
    
    exchangeAuthorizationCode(code);
}

void AuthCodeDialog::exchangeAuthorizationCode(const QString &code)
{
    QUrl tokenUrl(m_config.tokenUrl());
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    // WSL 2 SSL Fix: Ignore SSL certificate errors for development
    if (tokenUrl.scheme() == "https") {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    }
    
    // Prepare the request body for Authorization Code flow
    QUrlQuery query;
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", code);
    query.addQueryItem("client_id", m_config.clientId());
    query.addQueryItem("client_secret", m_config.clientSecret());
    query.addQueryItem("redirect_uri", m_config.redirectUri());
    
    QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
    
    qDebug() << "Exchanging authorization code for tokens";
    qDebug() << "Token URL:" << tokenUrl;
    
    m_currentReply = m_networkManager->post(request, postData);
    
    // SSL error handling
    connect(m_currentReply, QOverload<const QList<QSslError>&>::of(&QNetworkReply::sslErrors),
            [this](const QList<QSslError> &errors) {
                qWarning() << "SSL errors encountered:" << errors.count();
                m_currentReply->ignoreSslErrors();
            });
    
    connect(m_currentReply, &QNetworkReply::finished, 
            this, &AuthCodeDialog::onTokenResponseFinished);
}

void AuthCodeDialog::onTokenResponseFinished()
{
    if (!m_currentReply) {
        return;
    }
    
    int statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = m_currentReply->readAll();
    
    qDebug() << "Token response status:" << statusCode;
    qDebug() << "Response:" << responseData;
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    m_exchangeButton->setEnabled(true);
    m_exchangeButton->setText("Exchange Code");
    m_openBrowserButton->setEnabled(true);
    
    // Parse JSON response
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        QMessageBox::critical(this, "Error", "Invalid response from authentication server");
        emit loginFailed("Invalid response format");
        return;
    }
    
    QJsonObject jsonObj = jsonDoc.object();
    
    // Check for error response
    if (jsonObj.contains("error")) {
        QString error = jsonObj.value("error").toString();
        QString errorDescription = jsonObj.value("error_description").toString();
        
        qWarning() << "OIDC Error:" << error << "-" << errorDescription;
        
        QString displayError = errorDescription.isEmpty() ? error : errorDescription;
        QMessageBox::critical(this, "Authentication Failed", displayError);
        emit loginFailed(displayError);
        return;
    }
    
    // Extract tokens
    if (jsonObj.contains("access_token")) {
        m_accessToken = jsonObj.value("access_token").toString();
        m_idToken = jsonObj.value("id_token").toString();
        
        qDebug() << "Authentication successful!";
        qDebug() << "Access token received";
        
        emit loginSucceeded(m_accessToken);
        accept();
    } else {
        QMessageBox::critical(this, "Error", "No access token in response");
        emit loginFailed("No access token received");
    }
}

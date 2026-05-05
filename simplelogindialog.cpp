#include "simplelogindialog.h"
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
#include <QSslConfiguration>
#include <QSslSocket>

SimpleLoginDialog::SimpleLoginDialog(const OIDCConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
    , m_currentReply(nullptr)
{
    setWindowTitle("OIDC Login");
    setWindowModality(Qt::WindowModal);
    setGeometry(100, 100, 400, 250);
    
    m_networkManager = std::make_unique<QNetworkAccessManager>();
    
    createUI();
}

SimpleLoginDialog::~SimpleLoginDialog()
{
    if (m_currentReply) {
        m_currentReply->deleteLater();
    }
}

void SimpleLoginDialog::createUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Username field
    QHBoxLayout *usernameLayout = new QHBoxLayout();
    QLabel *usernameLabel = new QLabel("Username:", this);
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Enter your username");
    usernameLayout->addWidget(usernameLabel);
    usernameLayout->addWidget(m_usernameEdit);
    mainLayout->addLayout(usernameLayout);
    
    // Password field
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    QLabel *passwordLabel = new QLabel("Password:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Enter your password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(m_passwordEdit);
    mainLayout->addLayout(passwordLayout);
    
    mainLayout->addStretch();
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_loginButton = new QPushButton("Login", this);
    m_cancelButton = new QPushButton("Cancel", this);
    
    connect(m_loginButton, &QPushButton::clicked, this, &SimpleLoginDialog::onLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
}

void SimpleLoginDialog::onLoginClicked()
{
    QString username = m_usernameEdit->text();
    QString password = m_passwordEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", 
                           "Please enter both username and password");
        return;
    }
    
    m_loginButton->setEnabled(false);
    m_loginButton->setText("Logging in...");
    
    authenticateWithCredentials(username, password);
}

void SimpleLoginDialog::authenticateWithCredentials(const QString &username, const QString &password)
{
    QUrl tokenUrl(m_config.tokenUrl());
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    // WSL 2 SSL Fix: Ignore SSL certificate errors for development
    // WARNING: This is insecure for production! Only use for development/testing.
    // if (tokenUrl.scheme() == "https") {
    //     QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    //     sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    //     request.setSslConfiguration(sslConfig);
    //     qWarning() << "SSL verification disabled for development - use caution!";
    // }
    
    // Prepare the request body for Resource Owner Password Credentials flow
    QUrlQuery query;
    query.addQueryItem("grant_type", "password");
    query.addQueryItem("username", username);
    query.addQueryItem("password", password);
    query.addQueryItem("client_id", m_config.clientId());
    query.addQueryItem("client_secret", m_config.clientSecret());
    query.addQueryItem("scope", m_config.scope());
    
    QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
    
    qDebug() << "Sending token request to:" << tokenUrl;
    qDebug() << "Username:" << username;
    
    m_currentReply = m_networkManager->post(request, postData);
    
    // WSL 2 SSL Fix: Ignore SSL errors for development
    // WARNING: This is insecure for production! Only use for development/testing.
    connect(m_currentReply, QOverload<const QList<QSslError>&>::of(&QNetworkReply::sslErrors),
            [this](const QList<QSslError> &errors) {
                qWarning() << "SSL errors encountered:" << errors.count();
                for (const QSslError &error : errors) {
                    qWarning() << "  -" << error.errorString();
                }
                m_currentReply->ignoreSslErrors();
            });
    
    connect(m_currentReply, &QNetworkReply::finished, 
            this, &SimpleLoginDialog::onTokenResponseFinished);
    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &SimpleLoginDialog::onNetworkError);
}

void SimpleLoginDialog::onTokenResponseFinished()
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
    
    // Parse JSON response
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        m_loginButton->setEnabled(true);
        m_loginButton->setText("Login");
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
        
        m_loginButton->setEnabled(true);
        m_loginButton->setText("Login");
        
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
        m_loginButton->setEnabled(true);
        m_loginButton->setText("Login");
        QMessageBox::critical(this, "Error", "No access token in response");
        emit loginFailed("No access token received");
    }
}

void SimpleLoginDialog::onNetworkError()
{
    if (!m_currentReply) {
        return;
    }
    
    QString errorString = m_currentReply->errorString();
    qWarning() << "Network error:" << errorString;
    
    m_loginButton->setEnabled(true);
    m_loginButton->setText("Login");
    
    QMessageBox::critical(this, "Network Error", 
                         "Failed to connect to authentication server:\n" + errorString);
    emit loginFailed(errorString);
}

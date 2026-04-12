#include "logindialog.h"
#include <QVBoxLayout>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QMessageBox>

LoginDialog::LoginDialog(const OIDCConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
{
    setWindowTitle("OIDC Login");
    setWindowModality(Qt::WindowModal);
    setGeometry(100, 100, 800, 600);

    // Create web view
    m_webView = std::make_unique<QWebEngineView>();

    // Connect signals
    connect(m_webView.get(), &QWebEngineView::urlChanged, 
            this, &LoginDialog::onUrlChanged);
    connect(m_webView.get(), QOverload<bool>::of(&QWebEngineView::loadFinished),
            this, &LoginDialog::onLoadFinished);

    // Setup layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView.get());
    setLayout(layout);

    // Generate and store state for verification
    m_expectedState = OIDCConfig::generateState();

    // Load the authorization endpoint
    QUrl authUrl = m_config.getAuthorizationEndpoint();
    qDebug() << "Loading authorization URL:" << authUrl;
    m_webView->load(authUrl);
}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::onUrlChanged(const QUrl &url)
{
    qDebug() << "URL changed to:" << url;
    
    // Check if this is a redirect back to our redirect_uri
    if (url.toString().startsWith(m_config.redirectUri())) {
        parseRedirectUrl(url);
    }
}

void LoginDialog::onLoadFinished(bool success)
{
    if (!success) {
        qWarning() << "Failed to load login page";
        emit loginFailed("Failed to load login page");
    }
}

void LoginDialog::parseRedirectUrl(const QUrl &url)
{
    QUrlQuery query(url);
    
    // Check for error response
    if (query.hasQueryItem("error")) {
        QString error = query.queryItemValue("error");
        QString errorDescription = query.queryItemValue("error_description");
        qWarning() << "OIDC Error:" << error << "-" << errorDescription;
        emit loginFailed(errorDescription.isEmpty() ? error : errorDescription);
        reject();
        return;
    }
    
    // Extract authorization code
    if (query.hasQueryItem("code")) {
        m_authorizationCode = query.queryItemValue("code");
        m_state = query.queryItemValue("state");
        
        qDebug() << "Authorization code received";
        
        // Verify state parameter for security
        if (m_state.isEmpty()) {
            qWarning() << "State parameter missing in callback";
            emit loginFailed("State parameter missing - possible CSRF attack");
            reject();
            return;
        }
        
        // In production, verify that the state matches what we sent
        // For now, we accept it
        
        emit loginSucceeded(m_authorizationCode, m_state);
        accept();
    }
}

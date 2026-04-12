# OIDC Qt Client - Developer Guide

## Table of Contents
1. [Getting Started](#getting-started)
2. [Project Structure](#project-structure)
3. [Key Concepts](#key-concepts)
4. [Component Walkthrough](#component-walkthrough)
5. [Common Tasks](#common-tasks)
6. [Debugging Guide](#debugging-guide)
7. [Testing](#testing)
8. [Performance Considerations](#performance-considerations)
9. [Troubleshooting](#troubleshooting)

---

## Getting Started

### Prerequisites
- Qt 5.15+ (with WebEngineWidgets)
- CMake 3.16+
- C++17 compatible compiler
- Linux/Windows/macOS

### Quick Setup

```bash
# 1. Clone the repository
git clone <repo-url>
cd oidc-client-qt

# 2. Create build directory
mkdir build && cd build

# 3. Configure with CMake
cmake ..

# 4. Build
cmake --build .

# 5. Run
./oidc-client-qt
```

### Configuration

```bash
# 1. Copy example config
cp config.ini.example config.ini

# 2. Edit with your OIDC provider details
nano config.ini

# 3. Set your credentials:
# - AuthorizationUrl
# - TokenUrl  
# - ClientId
# - ClientSecret
# - RedirectUri
```

---

## Project Structure

### Directory Layout

```
oidc-client-qt/
│
├── Source Code
│   ├── main.cpp                          Entry point
│   ├── mainwindow.{h,cpp}                Main application window
│   ├── oidcconfig.{h,cpp}                Configuration management
│   ├── logindialog.{h,cpp}               Login UI dialog
│   └── mainwindow.ui                     Qt Designer UI file (optional)
│
├── Build System
│   ├── CMakeLists.txt                    Build configuration
│   ├── build/                            Build output directory
│   │   ├── oidc-client-qt               Executable
│   │   ├── CMakeCache.txt
│   │   └── ...
│   └── run.sh                            Running script with env setup
│
├── Configuration
│   ├── config.ini                        Runtime configuration
│   ├── config.ini.example                Configuration template
│   
├── Documentation
│   ├── ARCHITECTURE.md                   System design & architecture
│   ├── DIAGRAMS.md                       Visual diagrams & flows
│   ├── OIDC_IMPLEMENTATION.md            Implementation details
│   └── README.md (implicit)              Project overview
│
└── Version Control
    └── .git/                             Git repository
```

### File Dependencies

```
main.cpp
  └─► QApplication, MainWindow

MainWindow
  ├─► mainwindow.ui (Qt Designer)
  ├─► oidcconfig.{h,cpp}
  ├─► logindialog.{h,cpp}
  ├─► QSettings (config loading)
  └─► Qt Widgets

LoginDialog
  ├─► oidcconfig.{h,cpp}
  ├─► QWebEngineView
  └─► Qt Core/Gui

OIDCConfig
  ├─► QString, QUrl (Qt Core)
  └─► QUuid (for state generation)
```

---

## Key Concepts

### 1. OIDC Authentication Flow

**Authorization Code Flow** (most secure for native apps):

```
1. User clicks "Login" → Shows login web page
2. User enters credentials → Provider validates
3. Provider grants code → Redirects to your app with code
4. App extracts code → [Future] Exchanges code for tokens
5. App has tokens → Can access protected resources
```

Why this flow?
- Client secret stays on server (never exposed)
- No tokens in browser history
- Tokens never visible to redirects
- Suitable for all app types

### 2. State Parameter

**Purpose**: Protect against CSRF attacks

```cpp
// Generation (client-side)
QString state = OIDCConfig::generateState(); // UUID
// Result: "550e8400-e29b-41d4-a716-446655440000"

// Added to authorization URL
QUrlQuery query;
query.addQueryItem("state", state);

// Validation (after redirect)
QString returnedState = query.queryItemValue("state");
if (returnedState == expectedState) {
    // Safe - request came from same client
} else {
    // Unsafe - possible CSRF, reject
}
```

### 3. Redirect URI Matching

```cpp
// Must match EXACTLY what's registered with provider
// Registered: http://localhost:8080/callback
// Actual: http://localhost:8080/callback

// These will NOT match (case-sensitive):
// - http://localhost:8080/CALLBACK  ✗
// - http://localhost:8080/callback/  ✗
// - http://localhost:8080:8080/callback ✗

// Detection in LoginDialog::onUrlChanged()
if (url.toString().startsWith(m_config.redirectUri())) {
    parseRedirectUrl(url);  // Process this redirect
}
```

### 4. Configuration Hierarchy

```
Application searches in this order:

1. ./config.ini                 [App directory]
   ├─ Found? → Load and use
   └─ Not found? → Check #2

2. ../config.ini                [Parent directory]
   ├─ Found? → Load and use
   └─ Not found? → Check #3

3. Hardcoded defaults
   └─ Use fallback values in code

Result:
- Development: Use ../config.ini from build/ dir
- Deployment: Use ./config.ini in same dir as executable
- Broken config: Falls back to defaults
```

---

## Component Walkthrough

### OIDCConfig Class

**Responsibilities**: Store and manage OIDC provider settings

**Key Methods**:

```cpp
// Setters - Called during configuration
void setAuthorizationUrl(const QString &url);
void setClientId(const QString &id);
void setRedirectUri(const QString &uri);
// ... etc

// Getters - Called when building auth request
QString authorizationUrl() const;
QString clientId() const;
QString redirectUri() const;
// ... etc

// Authorization URL building
QUrl getAuthorizationEndpoint() const {
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

// State generation for security
static QString generateState() {
    return QUuid::createUuid().toString();
    // Example: "{550e8400-e29b-41d4-a716-446655440000}"
}
```

**Usage Pattern**:

```cpp
// 1. Create and configure
OIDCConfig config;
config.setAuthorizationUrl("https://auth.example.com/oauth/authorize");
config.setClientId("my-app-id");

// 2. Get authorization endpoint URL
QUrl authUrl = config.getAuthorizationEndpoint();
// Result: https://auth.example.com/oauth/authorize?response_type=code&...

// 3. Display in web view
webView->load(authUrl);
```

---

### LoginDialog Class

**Responsibilities**: Display login form and extract authorization code

**Architecture**:

```cpp
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    // Constructor - called when login needed
    explicit LoginDialog(const OIDCConfig &config, QWidget *parent = nullptr);
    
    // Accessors - get results after dialog closes
    QString getAuthorizationCode() const;
    QString getState() const;

signals:
    // Emitted when auth succeeds
    void loginSucceeded(const QString &authCode, const QString &state);
    
    // Emitted when auth fails
    void loginFailed(const QString &error);

private slots:
    // Qt signal handlers
    void onUrlChanged(const QUrl &url);      // Browser redirects
    void onLoadFinished(bool success);       // Page loaded

private:
    // URL parsing
    void parseRedirectUrl(const QUrl &url);
    
    // Members
    std::unique_ptr<QWebEngineView> m_webView;
    OIDCConfig m_config;
    QString m_authorizationCode;
    QString m_state;
    QString m_expectedState;
};
```

**Execution Flow**:

```cpp
// 1. Constructor
LoginDialog dialog(*m_oidcConfig, parentWindow);

// 2. Setup signal handlers
connect(&dialog, &LoginDialog::loginSucceeded,
        this, &MainWindow::onLoginSucceeded);
connect(&dialog, &LoginDialog::loginFailed,
        this, &MainWindow::onLoginFailed);

// 3. Show as modal (blocks until user closes)
int result = dialog.exec();

// 4. Check result
if (result == QDialog::Accepted) {
    // User authenticated successfully
    QString code = dialog.getAuthorizationCode();
    QString state = dialog.getState();
}
```

**URL Monitoring**:

```cpp
// When browser navigates to any URL:
void LoginDialog::onUrlChanged(const QUrl &url)
{
    qDebug() << "URL changed to:" << url;
    
    // Check if this is our redirect
    if (url.toString().startsWith(m_config.redirectUri())) {
        // Yes! Parse it
        parseRedirectUrl(url);
    }
    // Otherwise ignore (navigating within provider's site)
}
```

**Authorization Code Extraction**:

```cpp
void LoginDialog::parseRedirectUrl(const QUrl &url)
{
    QUrlQuery query(url);
    
    // Check for error first
    if (query.hasQueryItem("error")) {
        QString error = query.queryItemValue("error");
        QString description = query.queryItemValue("error_description");
        emit loginFailed(description.isEmpty() ? error : description);
        reject();
        return;
    }
    
    // Extract authorization code
    if (query.hasQueryItem("code")) {
        m_authorizationCode = query.queryItemValue("code");
        m_state = query.queryItemValue("state");
        
        // Success - emit signal
        emit loginSucceeded(m_authorizationCode, m_state);
        accept();  // Close dialog
    }
}
```

---

### MainWindow Class

**Responsibilities**: Orchestrate authentication and app lifecycle

**Construction**:

```cpp
MainWindow::MainWindow(QWidget *parent)
{
    setWindowTitle("OIDC Client");
    
    // 1. Load OIDC configuration
    initializeOIDCConfig();
    
    // 2. Create main UI
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    QLabel *welcomeLabel = new QLabel("Welcome!");
    layout->addWidget(welcomeLabel);
    
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
}
```

**Configuration Loading**:

```cpp
void MainWindow::initializeOIDCConfig()
{
    // 1. Create config object
    m_oidcConfig = std::make_unique<OIDCConfig>();
    
    // 2. Try to load from file
    if (loadConfigFromFile()) {
        qDebug() << "Config loaded from file";
        return;
    }
    
    // 3. Fallback to defaults
    m_oidcConfig->setAuthorizationUrl("https://auth.example.com/oauth/authorize");
    m_oidcConfig->setClientId("default-client-id");
    // ... etc
}

bool MainWindow::loadConfigFromFile()
{
    // Search for config file
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    if (!QFileInfo::exists(configPath)) {
        configPath = QCoreApplication::applicationDirPath() + "/../config.ini";
    }
    
    if (!QFileInfo::exists(configPath)) {
        return false;  // File not found
    }
    
    // Parse INI file
    QSettings settings(configPath, QSettings::IniFormat);
    
    // Extract values
    QString authUrl = settings.value("OIDC/AuthorizationUrl", "").toString();
    QString clientId = settings.value("OIDC/ClientId", "").toString();
    // ... etc
    
    // Validate
    if (authUrl.isEmpty() || clientId.isEmpty()) {
        return false;  // Invalid config
    }
    
    // Apply
    m_oidcConfig->setAuthorizationUrl(authUrl);
    m_oidcConfig->setClientId(clientId);
    // ... etc
    
    return true;
}
```

**Authentication Flow**:

```cpp
// Triggered on first window show
void MainWindow::showEvent(QShowEvent *event)
{
    if (!m_loginAttempted) {
        m_loginAttempted = true;
        // Schedule login dialog to show after current event
        QTimer::singleShot(0, this, &MainWindow::showLoginDialog);
    }
    QMainWindow::showEvent(event);
}

// Show the login dialog
void MainWindow::showLoginDialog()
{
    if (!m_oidcConfig) {
        QMessageBox::critical(this, "Configuration Error", 
                            "OIDC configuration failed");
        return;
    }
    
    // Create dialog with config
    LoginDialog loginDialog(*m_oidcConfig, this);
    
    // Connect signals
    connect(&loginDialog, &LoginDialog::loginSucceeded,
            this, &MainWindow::onLoginSucceeded);
    connect(&loginDialog, &LoginDialog::loginFailed,
            this, &MainWindow::onLoginFailed);
    
    // Show as modal (blocks until result)
    int result = loginDialog.exec();
    
    // If dialog wasn't accepted and we're not authenticated
    if (result != QDialog::Accepted && !m_isAuthenticated) {
        // Login failed or cancelled - exit
        close();
    }
}

// Called when user successfully authenticates
void MainWindow::onLoginSucceeded(const QString &authCode, const QString &state)
{
    qDebug() << "Login succeeded with code:" << authCode;
    
    m_isAuthenticated = true;
    
    // Show confirmation to user
    QMessageBox::information(this, "Login Successful", 
        QString("Authorization code received:\n%1").arg(authCode));
    
    grantApplicationAccess();
}

// Called when authentication fails
void MainWindow::onLoginFailed(const QString &error)
{
    qWarning() << "Login failed:" << error;
    
    QMessageBox::critical(this, "Login Failed", 
        QString("Authentication failed:\n%1").arg(error));
    
    // Exit application
    close();
}

// Post-authentication setup
void MainWindow::grantApplicationAccess()
{
    // TODO: Additional setup after successful auth
    // - Fetch user profile
    // - Load user preferences
    // - Initialize authenticated features
    qDebug() << "Application access granted";
}
```

---

## Common Tasks

### Adding a New OIDC Provider

```cpp
// 1. Edit config.ini
[OIDC]
AuthorizationUrl=https://new-provider.com/oauth/authorize
TokenUrl=https://new-provider.com/oauth/token
ClientId=your-new-client-id
ClientSecret=your-client-secret
RedirectUri=http://localhost:8080/callback
Scope=openid profile email

// 2. If specific scopes needed:
Scope=openid profile email custom_scope

// 3. Test with: ./oidc-client-qt
```

### Changing the Redirect URI

```cpp
// Edit config.ini
[OIDC]
RedirectUri=http://localhost:9090/callback

// NOTE: This URI MUST match exactly what's registered
// with your OIDC provider. Register new URI there first,
// then update config.ini
```

### Implementing Token Exchange (Future)

```cpp
// Future enhancement - in onLoginSucceeded():
void MainWindow::exchangeCodeForTokens(const QString &authCode)
{
    // 1. Create HTTP request to token endpoint
    QNetworkRequest request(QUrl(m_oidcConfig->tokenUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, 
                     "application/x-www-form-urlencoded");
    
    // 2. Prepare POST data
    QUrlQuery postData;
    postData.addQueryItem("grant_type", "authorization_code");
    postData.addQueryItem("code", authCode);
    postData.addQueryItem("client_id", m_oidcConfig->clientId());
    postData.addQueryItem("client_secret", m_oidcConfig->clientSecret());
    postData.addQueryItem("redirect_uri", m_oidcConfig->redirectUri());
    
    // 3. Send request (requires QNetworkAccessManager)
    // manager->post(request, postData.toString().toUtf8());
    
    // 4. Handle response (get access_token, id_token, etc.)
}
```

### Adding Debug Logging

```cpp
// Already in place:
qDebug() << "Message";     // Debug info
qWarning() << "Message";   // Warnings  
qCritical() << "Message";  // Errors
qFatal("Message");         // Fatal

// Run with output visible:
./oidc-client-qt 2>&1 | tee output.log

// Or use Qt Creator's Application Output pane
```

---

## Debugging Guide

### Enable Debug Output

```bash
# Run with debug output
QT_DEBUG_PLUGINS=1 ./oidc-client-qt

# Or from code:
qSetMessagePattern("%{type} [%{file}:%{line}] %{message}");
```

### Common Issues

**Issue**: Login dialog doesn't appear
```cpp
// 1. Check if config loaded
qDebug() << m_oidcConfig->clientId();  // Should not be empty

// 2. Check if showEvent() called
// Add logging to showEvent()

// 3. Verify QWebEngineView initialized
// Try loading a simple test URL first
```

**Issue**: Authorization URL malformed
```cpp
// Debug the generated URL
QUrl authUrl = m_oidcConfig->getAuthorizationEndpoint();
qDebug() << "Auth URL:" << authUrl.toString();

// Verify it contains all needed parameters:
// - response_type=code
// - client_id
// - redirect_uri
// - scope
// - state
```

**Issue**: Redirect not detected
```cpp
// Add logging to onUrlChanged()
void LoginDialog::onUrlChanged(const QUrl &url)
{
    qDebug() << "URL:" << url.toString();
    qDebug() << "Redirect URI:" << m_config.redirectUri();
    qDebug() << "Matches:" << url.toString().startsWith(m_config.redirectUri());
}

// Check if redirect URL starts with registered URI
// Remember: URLs are case-sensitive!
```

**Issue**: Authorization code not extracted
```cpp
// Debug URL parsing
void LoginDialog::parseRedirectUrl(const QUrl &url)
{
    QUrlQuery query(url);
    qDebug() << "Query items:" << query.queryItems();
    
    // Check each expected parameter
    if (query.hasQueryItem("code")) {
        qDebug() << "Code found:" << query.queryItemValue("code");
    } else {
        qDebug() << "No code parameter!";
    }
    
    if (query.hasQueryItem("error")) {
        qDebug() << "Error:" << query.queryItemValue("error");
    }
}
```

### Qt Creator Debugging

1. **Set breakpoints**:
   - Click left margin in editor
   - Run project with Debug build type

2. **Inspector**:
   - Step through code
   - View variables in panel
   - Watch expressions

3. **Application Output**:
   - View qDebug() output
   - Filter by message type
   - Copy for analysis

---

## Testing

### Unit Test Example

```cpp
#include <QtTest/QTest>
#include "oidcconfig.h"

class TestOIDCConfig : public QObject
{
    Q_OBJECT

private slots:
    void testStateGeneration()
    {
        QString state1 = OIDCConfig::generateState();
        QString state2 = OIDCConfig::generateState();
        
        QVERIFY(!state1.isEmpty());
        QVERIFY(!state2.isEmpty());
        QVERIFY(state1 != state2);  // Should be unique
    }
    
    void testAuthorizationUrl()
    {
        OIDCConfig config;
        config.setAuthorizationUrl("https://auth.example.com/oauth/authorize");
        config.setClientId("test-id");
        config.setRedirectUri("http://localhost:8080/callback");
        
        QUrl url = config.getAuthorizationEndpoint();
        
        QVERIFY(url.toString().contains("response_type=code"));
        QVERIFY(url.toString().contains("client_id=test-id"));
        QVERIFY(url.toString().contains("redirect_uri"));
    }
};

#include "tst_oidcconfig.moc"
QTEST_MAIN(TestOIDCConfig)
```

### Integration Testing

```cpp
// Test with real OIDC provider
// 1. Register test application
// 2. Get test credentials
// 3. Test full auth flow
// 4. Verify code extraction
```

---

## Performance Considerations

### Startup Time

```
Typical startup sequence timing:
1. QApplication creation      ~10ms
2. Config file loading        ~5ms (or ~0ms fallback to defaults)
3. UI creation                ~5ms
4. WebEngine initialization   ~100-500ms (first time)
5. Auth page loading          ~500-1000ms (network dependent)

Total: ~600-1500ms typical
```

**Optimization Tips**:
- Cache WebEngine initialization across runs
- Load config.ini once (already done)
- Lazy-load WebEngine if possible

### Memory Usage

```
Typical memory profile:
- Qt framework + WebEngine: ~100-150 MB
- OIDC Client application: ~50-100 MB
- Total: ~150-250 MB

WebEngine is memory intensive due to Chromium
```

---

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| "Config file not found" | config.ini missing | Create from config.ini.example |
| "Missing AuthorizationUrl" | Incomplete config.ini | Check [OIDC] section in config file |
| Dialog doesn't show | WebEngine not initialized | Check Qt WebEngineWidgets installed |
| Redirect not detected | Wrong RedirectUri registered | Verify RedirectUri matches provider registration exactly |
| "Access denied" from provider | Wrong scope/permissions | Check scopes in config.ini match provider requirements |
| URL malformed | Missing query parameters | Check OIDCConfig::getAuthorizationEndpoint() |
| Application freezes | Thin UI thread | Avoid blocking in main thread, use QTimer |

---

## Additional Resources

- [Qt Documentation](https://doc.qt.io)
- [OIDC Specification](https://openid.net/specs/openid-connect-core-1_0.html)
- [OAuth 2.0 RFC 6749](https://tools.ietf.org/html/rfc6749)
- Project ARCHITECTURE.md file
- Project DIAGRAMS.md file


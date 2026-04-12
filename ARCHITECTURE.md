# OIDC Qt Client - Architecture Documentation

## Table of Contents
1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Component Architecture](#component-architecture)
4. [Data Flow](#data-flow)
5. [Class Hierarchy & Relationships](#class-hierarchy--relationships)
6. [Configuration System](#configuration-system)
7. [Authentication Flow](#authentication-flow)
8. [Build System](#build-system)
9. [Deployment Architecture](#deployment-architecture)
10. [Security Architecture](#security-architecture)
11. [Extension Points](#extension-points)

---

## Overview

**OIDC Qt Client** is a Qt5-based desktop application that implements the OpenID Connect (OIDC) authentication flow using the Authorization Code Grant flow. The application provides a web-based login interface through Qt's QWebEngineView component and handles token exchange with OIDC providers.

### Key Characteristics
- **Framework**: Qt 5.15+
- **Language**: C++17
- **Build System**: CMake 3.16+
- **Authentication Protocol**: OpenID Connect (OAuth 2.0 based)
- **UI Rendering**: QWebEngineView (Chromium-based)
- **Configuration**: INI-format configuration files with fallback to defaults

### Design Principles
1. **Separation of Concerns** - Configuration, UI, and authentication logic are separated
2. **Modularity** - Easy to extend with token exchange and user profile fetching
3. **Security First** - State parameter validation, secure code exchange flow
4. **Configurability** - All OIDC parameters loaded from config file with sensible defaults

---

## System Architecture

### High-Level View

```
┌─────────────────────────────────────────────────────────────┐
│                                                               │
│  OIDC Qt Client Application                                  │
│                                                               │
│  ┌──────────────┐                                             │
│  │ MainWindow   │ (Main UI Container)                        │
│  └──────┬───────┘                                             │
│         │                                                     │
│         ├─► config.ini ──────────────────────────────────────┤
│         │   (Configuration file)                              │
│         │                                                     │
│         └─► OIDCConfig                                        │
│             (Configuration Manager)                          │
│                                                               │
│  ┌──────────────┐                                             │
│  │ LoginDialog  │ (OIDC Login UI)                            │
│  │              │                                             │
│  │ ┌──────────┐ │                                             │
│  │ │QWebEngine│ │ ─────────► OIDC Provider                   │
│  │ │  View    │ │ (Browser)   (Authorization Server)        │
│  │ └──────────┘ │                                             │
│  └──────────────┘                                             │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Layered Architecture

```
┌─────────────────────────────────────────────┐
│   Presentation Layer                         │
│  (MainWindow, LoginDialog, QWebEngineView)  │
└────────────────────┬────────────────────────┘
                     │
┌────────────────────▼────────────────────────┐
│   Business Logic Layer                       │
│  (OIDCConfig, LoginDialog flow management)  │
└────────────────────┬────────────────────────┘
                     │
┌────────────────────▼────────────────────────┐
│   Configuration Layer                        │
│  (QSettings, config.ini, environment)       │
└────────────────────┬────────────────────────┘
                     │
┌────────────────────▼────────────────────────┐
│   External Services (OIDC Provider)         │
│  (Authorization, Token endpoints)           │
└────────────────────────────────────────────┘
```

---

## Component Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        Qt5 Framework                          │
│  (QMainWindow, QDialog, QWebEngineView, QSettings, etc.)    │
└─────────────────────────────────────────────────────────────┘
                                                               │
        ┌───────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│                   OIDC Qt Client Components                  │
│                                                               │
│  ┌──────────────────┐  ┌──────────────────┐                 │
│  │   MainWindow     │  │   OIDCConfig     │                 │
│  │                  │  │                  │                 │
│  │ ┌──────────────┐ │  │ • Configuration  │                 │
│  │ │Configuration │─┼─→│   Management     │                 │
│  │ │  Management  │ │  │ • URL Building   │                 │
│  │ ├──────────────┤ │  │ • State Gen.     │                 │
│  │ │ Login Flow   │ │  └──────────────────┘                 │
│  │ │ Management   │─┼──┐                                    │
│  │ ├──────────────┤ │  │                                    │
│  │ │ Access       │ │  │                                    │
│  │ │ Control      │ │  │                                    │
│  │ └──────────────┘ │  │                                    │
│  └──────────────────┘  │                                    │
│         │              │                                    │
│         │         ┌────▼──────────────────┐                 │
│         └────────→│   LoginDialog        │                 │
│                   │                      │                 │
│                   │ ┌──────────────────┐ │                 │
│                   │ │ QWebEngineView   │ │                 │
│                   │ │ • URL Monitoring │ │                 │
│                   │ │ • Code Parsing   │ │                 │
│                   │ └──────────────────┘ │                 │
│                   │ • Error Handling   │ │                 │
│                   │ • Signal Emission  │ │                 │
│                   └────────────────────┘                 │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

#### **OIDCConfig**
**Purpose**: Centralized OIDC configuration management

**Responsibilities**:
- Store OIDC provider parameters (URLs, IDs, URIs)
- Generate authorization endpoint URLs with query parameters
- Generate secure state parameters for CSRF protection
- Provide getters for configuration values

**Key Methods**:
```cpp
QUrl getAuthorizationEndpoint()    // Builds complete auth URL
static QString generateState()     // Creates random state
void setAuthorizationUrl(...)      // Configuration setters
QString authorizationUrl() const   // Configuration getters
```

**Dependencies**: Qt Core (QString, QUrl, QUuid)

---

#### **LoginDialog**
**Purpose**: Display OIDC login interface using web browser

**Responsibilities**:
- Create and display modal dialog with web engine
- Monitor URL changes to detect redirect callback
- Parse authorization code from redirect URL
- Handle OIDC provider error responses
- Emit signals for success/failure
- Validate state parameter

**Key Methods**:
```cpp
void onUrlChanged(const QUrl &url)      // Detect redirects
void onLoadFinished(bool success)       // Handle page load
void parseRedirectUrl(const QUrl &url)  // Extract auth code
QString getAuthorizationCode() const    // Retrieve auth code
```

**Signals**:
```cpp
void loginSucceeded(const QString &authCode, const QString &state)
void loginFailed(const QString &error)
```

**Dependencies**: Qt WebEngineWidgets, OIDCConfig

---

#### **MainWindow**
**Purpose**: Main application container and authentication orchestrator

**Responsibilities**:
- Load OIDC configuration from file or defaults
- Orchestrate login flow on application startup
- Handle successful/failed authentication responses
- Control application access based on authentication state
- Manage main application UI

**Key Methods**:
```cpp
void initializeOIDCConfig()      // Config initialization
bool loadConfigFromFile()        // Load from config.ini
void showLoginDialog()            // Start login flow
void onLoginSucceeded(...)        // Handle success
void onLoginFailed(...)           // Handle failure
void grantApplicationAccess()    // Post-auth setup
```

**State Variables**:
```cpp
bool m_isAuthenticated           // Authentication status
bool m_loginAttempted            // Prevent re-showing dialog
std::unique_ptr<OIDCConfig> m_oidcConfig  // Config holder
```

**Dependencies**: Qt Widgets, OIDCConfig, LoginDialog, oidcconfig.h

---

#### **main.cpp**
**Purpose**: Application entry point

**Responsibilities**:
- Initialize Qt application
- Create and display main window
- Start event loop

**Key Code**:
```cpp
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
```

---

## Data Flow

### Startup Flow

```
Application Start
    │
    ▼
main()
    │
    ├─► QApplication::create
    │
    ▼
MainWindow::MainWindow()
    │
    ├─► initializeOIDCConfig()
    │       │
    │       ├─► Create OIDCConfig instance
    │       │
    │       ├─► loadConfigFromFile()
    │       │   ├─► Search for config.ini
    │       │   ├─► Parse INI file (QSettings)
    │       │   └─► Validate parameters
    │       │
    │       └─► [If file not found] Use defaults
    │
    ├─► Create UI widgets (centralWidget, layout, labels)
    │
    └─► setCentralWidget()
    │
    ▼
main() → window.show()
    │
    ▼
showEvent() triggered
    │
    ├─► m_loginAttempted = true
    │
    └─► QTimer::singleShot(0, this, &MainWindow::showLoginDialog)
        │
        ▼
    showLoginDialog() immediately
```

### Authentication Flow

```
showLoginDialog()
    │
    ├─► Create LoginDialog instance
    │
    ├─► Connect signals to slots
    │   ├─► loginSucceeded → onLoginSucceeded()
    │   └─► loginFailed → onLoginFailed()
    │
    ▼
LoginDialog::exec() [Modal Dialog]
    │
    ├─► OIDCConfig::getAuthorizationEndpoint()
    │   │
    │   ├─► Build URL with query params:
    │   │   ├─ response_type=code
    │   │   ├─ client_id=...
    │   │   ├─ redirect_uri=...
    │   │   ├─ scope=...
    │   │   └─ state=<random-uuid>
    │   │
    │   └─► Return QUrl
    │
    ├─► m_webView->load(authUrl)
    │
    ▼
[Browser Renders OIDC Login Page]
    │
    ├─► User enters credentials
    │
    ├─► Provider validates credentials
    │
    ▼
[Provider Redirects to RedirectUri with code]
    │
    ▼
onUrlChanged() slot triggered
    │
    ├─► Detect RedirectUri match
    │
    ├─► parseRedirectUrl(url)
    │   │
    │   ├─► QUrlQuery query(url)
    │   │
    │   ├─► Check for error parameters
    │   │   └─► emit loginFailed(error)
    │   │
    │   ├─► Extract "code" parameter
    │   │
    │   ├─► Extract "state" parameter
    │   │
    │   ├─► Validate state (optional, for CSRF)
    │   │
    │   └─► Extract authorization code
    │
    ├─► emit loginSucceeded(authCode, state)
    │
    └─► dialog.accept()
        │
        ▼
    Signal emitted: loginSucceeded(authCode, state)
        │
        ▼
    onLoginSucceeded() in MainWindow
        │
        ├─► m_isAuthenticated = true
        │
        ├─► QMessageBox::information() [User feedback]
        │
        ├─► grantApplicationAccess()
        │   └─► [Future: Token exchange, user data loading]
        │
        └─► Dialog closes, application continues
```

### Error Flow

```
parseRedirectUrl() detects error parameter
    │
    ├─► Extract error name and description
    │
    ├─► emit loginFailed(error)
    │   │
    │   └─► Signal propagates to MainWindow
    │       │
    │       └─► onLoginFailed(error) executed
    │           │
    │           ├─► QMessageBox::critical() [Show error to user]
    │           │
    │           └─► close() [Close application]
    │
    └─► Dialog rejects
```

---

## Class Hierarchy & Relationships

### Class Diagram

```
QMainWindow                    QDialog
    ▲                            ▲
    │ inherits                   │ inherits
    │                            │
┌───┴──────────┐           ┌────┴────────────┐
│  MainWindow  │           │  LoginDialog    │
└───┬──────────┘           └────┬────────────┘
    │                            │
    │ uses                       │ uses
    │                            │
    │                      ┌─────▼────────────┐
    │                      │ QWebEngineView   │
    │                      └──────────────────┘
    │
    │ creates                    ▲
    │ and composes               │ uses
    │                            │
    └────────────────┬───────────┘
                     │
                     │ uses
                     ▼
        ┌────────────────────────┐
        │   OIDCConfig           │
        │                        │
        │ Attributes:            │
        │ - m_authorizationUrl   │
        │ - m_tokenUrl           │
        │ - m_clientId           │
        │ - m_clientSecret       │
        │ - m_redirectUri        │
        │ - m_scope              │
        │                        │
        │ Methods:               │
        │ + setters/getters      │
        │ + getAuthEndpoint()    │
        │ + generateState()      │
        └────────────────────────┘
```

### Object Relationships

```
MainWindow
├── std::unique_ptr<OIDCConfig> m_oidcConfig
│   └── Contains OIDC provider configuration
│
└── [In showLoginDialog()]
    └── LoginDialog (stack-allocated, modal)
        ├── OIDCConfig (reference to MainWindow's config)
        └── std::unique_ptr<QWebEngineView> m_webView
            └── Renders OIDC provider login page

Connection Chain:
LoginDialog::loginSucceeded signal 
    → MainWindow::onLoginSucceeded() slot
    
LoginDialog::loginFailed signal 
    → MainWindow::onLoginFailed() slot
```

---

## Configuration System

### Configuration Loading Strategy

```
┌─────────────────────────────────────────────┐
│ MainWindow::initializeOIDCConfig()          │
└────────────────┬────────────────────────────┘
                 │
                 ▼
    ┌──────────────────────────┐
    │ loadConfigFromFile()     │
    └────────────┬─────────────┘
                 │
        ┌────────┴─────────┐
        │                  │
        ▼                  ▼
   Search Path 1:     Search Path 2:
   ./config.ini       ../config.ini
        │                  │
        └────────┬─────────┘
                 │
        ┌────────▼──────────┐
        │ File found?       │
        └────────┬──────────┘
                 │
         ┌───────┴───────┐
         │               │
        YES              NO
         │               │
         ▼               ▼
    ┌────────┐     ┌──────────────┐
    │Parse   │     │Use Defaults  │
    │with    │     │(Fallback)    │
    │QSettings     └──────────────┘
    └────────┘
         │
         ▼
    ┌─────────────────┐
    │Validate         │
    │Parameters       │
    └────────┬────────┘
             │
      ┌──────┴──────┐
      │             │
     YES           NO
      │             │
      ▼             ▼
   Valid      ┌──────────┐
   Config     │Return    │
      │       │false     │
      ▼       └──────────┘
   ┌──────────────────┐
   │Apply to          │
   │OIDCConfig        │
   │Return true       │
   └──────────────────┘
```

### Configuration File Format (config.ini)

```ini
[OIDC]
# Authorization and token URLs
AuthorizationUrl=https://auth.provider.com/oauth/authorize
TokenUrl=https://auth.provider.com/oauth/token

# Application credentials
ClientId=your-app-id
ClientSecret=your-app-secret

# Redirect handler (must be registered with provider)
RedirectUri=http://localhost:8080/callback

# OAuth2 scopes to request
Scope=openid profile email

[Application]
# (Optional) Window dimensions
WindowWidth=1000
WindowHeight=700

# (Optional) Local server port for redirects
ListenPort=8080
```

### Configuration Precedence

1. **Command-line arguments** (not currently implemented)
2. **config.ini file** (in app directory or parent)
3. **Hardcoded defaults** (in initializeOIDCConfig)
4. **Environment variables** (not currently implemented)

---

## Authentication Flow

### OAuth 2.0 Authorization Code Flow (Implemented)

```
┌─────────┐                                  ┌──────────────┐
│         │                                  │              │
│ Client  │                                  │ OIDC         │
│ App     │                                  │ Provider     │
│         │                                  │              │
└────┬────┘                                  └──────┬───────┘
     │                                              │
     │  (1) Direct user to                         │
     │      authorization endpoint                 │
     ├─────────────────────────────────────────────►
     │     (response_type=code&client_id=...       │
     │      &redirect_uri=...&scope=...&state=...) │
     │                                              │
     │                                    ┌─────────▼────────┐
     │                                    │ User logs in     │
     │                                    │ Grants consent   │
     │                                    └─────────┬────────┘
     │                                              │
     │  (2) Redirect back with code                │
     │◄─────────────────────────────────────────────
     │     (code=...&state=...)                    │
     │                                              │
     ├──► Parse redirect URL                       │
     │    Extract code and state                   │
     │    Validate state                           │
     │    Store auth code                          │
     │                                              │
     │  (3) [Future] Exchange code for tokens      │
     ├─────────────────────────────────────────────►
     │     (code=...&client_id=...                 │
     │      &client_secret=...&grant_type=...)     │
     │                                              │
     │  (4) [Future] Return tokens                 │
     │◄─────────────────────────────────────────────
     │     (access_token, id_token,                │
     │      refresh_token, expires_in)             │
     │                                              │
     └─────────────────────────────────────────────►
           (5) [Future] Use access_token for
               API requests with Authorization
               header
```

### Currently Implemented Steps
- ✅ Step (1) & (2): Authorization and redirect handling
- ✅ Code extraction and state validation
- ❌ Step (3) & (4): Token exchange (future enhancement)
- ❌ Step (5): API requests with tokens (future enhancement)

---

## Build System

### CMake Architecture

```
CMakeLists.txt
├── Qt5 Configuration
│   ├── Find Qt6/Qt5 required components
│   ├── Widgets
│   ├── WebEngineWidgets
│   └── Network
│
├── C++ Standard
│   └── C++17
│
├── Automoc/Autouic/Autorcc
│   ├── Automatic MOC for Q_OBJECT classes
│   ├── Automatic UIC for .ui files
│   └── Automatic RCC for resources
│
├── Source Files
│   ├── main.cpp
│   ├── mainwindow.cpp/h
│   ├── loginwindow.cpp/h
│   ├── oidcconfig.cpp/h
│   └── mainwindow.ui (optional)
│
├── Target Configuration
│   ├── Executable: oidc-client-qt
│   └── Link Qt libraries
│
└── Installation Rules
    └── Copy to standard locations
```

### Build Output Structure

```
build/
├── CMakeFiles/
│   ├── oidc-client-qt.dir/
│   │   ├── main.cpp.o
│   │   ├── mainwindow.cpp.o
│   │   ├── logindialog.cpp.o
│   │   ├── oidcconfig.cpp.o
│   │   └── oidc-client-qt_autogen/
│   │       └── mocs_compilation.cpp
│   └── [CMake infrastructure]
│
├── oidc-client-qt                    # Final executable
│
├── CMakeCache.txt
├── cmake_install.cmake
└── build.ninja (or Makefile)
```

---

## Deployment Architecture

### Runtime Environment Requirements

```
Application Runtime
│
├── System Dependencies
│   ├── Qt5 Core Library
│   ├── Qt5 GUI Library
│   ├── Qt5 Widgets Library
│   ├── Qt5 WebEngineWidgets Library
│   ├── Qt5 Network Library
│   └── System Libraries
│       ├── libstdc++
│       ├── libm (math)
│       └── libc
│
├── Assets & Configuration
│   ├── config.ini (or ENV defaults)
│   ├── Qt plugins (in Qt plugin path)
│   └── WebEngine resources
│
└── Runtime Requirements
    ├── DISPLAY (X11/Wayland)
    ├── Network connectivity
    └── Browser capability (Chromium)
```

### Installation Paths

```
Standard Locations:
├── Binary: /usr/local/bin/oidc-client-qt
├── Config: ~/.config/oidc-client-qt/
│   └── config.ini
└── Resources: /usr/local/share/oidc-client-qt/
```

### Runtime Search Paths

```
Config File Search Order:
1. ./config.ini                           (app directory)
2. ../config.ini                          (parent directory)
3. Fallback to compiled defaults

Library Search Order:
1. LD_LIBRARY_PATH environment variable
2. /usr/local/lib
3. /usr/lib
4. Qt installation paths
```

---

## Security Architecture

### Security Features Implemented

#### 1. **State Parameter for CSRF Protection**
```cpp
// In OIDCConfig::getAuthorizationEndpoint()
QString stateValue = OIDCConfig::generateState();  // UUID
query.addQueryItem("state", stateValue);

// In LoginDialog::parseRedirectUrl()
QString returnedState = query.queryItemValue("state");
// Validate matches expected state
```

#### 2. **Authorization Code Flow (Most Secure)**
- Uses authorization code (not access token) in redirect
- Code must be exchanged server-side with client secret
- Prevents token exposure in browser history

#### 3. **Secure Redirect Detection**
```cpp
// LoginDialog monitors URL changes
if (url.toString().startsWith(m_config.redirectUri())) {
    // Only process redirects to registered URI
    parseRedirectUrl(url);
}
```

#### 4. **Error Handling**
```cpp
if (query.hasQueryItem("error")) {
    QString error = query.queryItemValue("error");
    emit loginFailed(error);
    reject();
}
```

### Security Considerations & Recommendations

#### ⚠️ For Development Use (Current State)
- Hardcoded secrets in config file
- No encrypted storage of tokens
- HTTP redirect URIs allowed (localhost only)

#### 🔒 For Production Deployment
**Required Enhancements**:

1. **Token Storage**
   - Use OS keychain (Keychain on macOS, Credential Manager on Windows, Secret Service on Linux)
   - Never store tokens in plain text config files

2. **PKCE (Proof Key for Code Exchange)**
   - Add code_challenge and code_verifier parameters
   - Enhanced security for native applications

3. **HTTPS Enforcement**
   - All communication with OIDC provider must use HTTPS
   - Validate SSL certificates

4. **Token Refresh & Revocation**
   - Implement refresh token flow for long-lived access
   - Add logout with token revocation

5. **Secure Configuration**
   - Client secret should never be stored in config file
   - Use environment variables or secure storage
   - Consider using mTLS for server communication

6. **Input Validation**
   - Validate all URL parameters
   - Sanitize error messages before display

7. **Logging**
   - Log authentication attempts without exposing sensitive data
   - Monitor for suspicious patterns

---

## Extension Points

### 1. Token Exchange Implementation

**Current**: App receives authorization code  
**Next Step**: Exchange code for tokens

```cpp
// In mainwindow.cpp
void MainWindow::onLoginSucceeded(const QString &authCode, const QString &state)
{
    // TODO: Implement token exchange
    // POST /token endpoint with:
    // - code
    // - client_id
    // - client_secret  (server-side only!)
    // - redirect_uri
    // - grant_type=authorization_code
}
```

### 2. User Profile Fetching

```cpp
// Fetch user info from OIDC provider's userinfo endpoint
// Use access token in Authorization header
// Store user data (name, email, picture, etc.)
```

### 3. Token Storage

```cpp
// Implement secure token storage
// Options:
// - Qt Keychain (cross-platform library)
// - OS-specific keychains
// - Encrypted local database
```

### 4. Session Management

```cpp
// Add features like:
// - Token expiration tracking
// - Automatic token refresh
// - Logout with token revocation
// - Session persistence
```

### 5. Multi-OIDC Provider Support

```cpp
// Extend OIDCConfig to support multiple providers
// Allow user selection during login
// Store provider configs in config.ini
```

### 6. Local HTTP Server for Redirects

```cpp
// Replace redirect URI with local server
// Allows custom redirect handling
// Better error page presentation
// Could be extended with:
// - Token display
// - Error recovery UI
// - Logging
```

### 7. Advanced Features

- **Single Sign-Off (SLO)**
- **Federation Support**
- **Backchannel Authentication**
- **Device Flow**
- **Authorization Grant**
- **Refresh Token Rotation**

---

## Testing & Quality

### Recommended Test Strategy

```
Unit Tests
├── OIDCConfig
│   ├── URL generation
│   ├── State parameter generation
│   └── Configuration loading
│
└── URL Parsing
    ├── Authorization code extraction
    ├── Error parameter handling
    └── State validation

Integration Tests
├── LoginDialog with mock OIDC provider
├── Config file loading and validation
└── End-to-end authentication flow

Manual Testing
├── Real OIDC provider (Google, Keycloak, etc.)
├── Error scenarios
├── Network failures
└── Redirect handling
```

### Code Quality

```
Coverage Areas:
├── All public methods documented
├── Key algorithms explained
├── Configuration options clear
└── Error paths documented

Tools:
├── CMake static analysis
├── Clang-tidy for code quality
├── Clazy for Qt-specific issues
└── Memory leak detection (Valgrind)
```

---

## File Structure

```
oidc-client-qt/
├── CMakeLists.txt                   # Build configuration
├── main.cpp                         # Entry point
│
├── mainwindow.h / cpp               # Main application window
├── oidcconfig.h / cpp               # OIDC configuration
├── logindialog.h / cpp              # Login dialog UI
├── mainwindow.ui                    # Qt Designer UI file (optional)
│
├── config.ini                       # Runtime configuration
├── config.ini.example               # Configuration template
│
├── run.sh                           # Environment setup script
│
├── OIDC_IMPLEMENTATION.md           # Implementation guide
├── ARCHITECTURE.md                  # This file
│
├── build/                           # CMake build directory
│   ├── oidc-client-qt              # Executable
│   ├── CMakeCache.txt
│   └── ...
│
└── .git/                            # Version control
```

---

## Glossary

| Term | Definition |
|------|-----------|
| **OIDC** | OpenID Connect - authentication protocol built on OAuth 2.0 |
| **OAuth 2.0** | Authorization framework for delegated access |
| **Authorization Code** | Temporary code exchanged for tokens after user login |
| **Access Token** | Token used to access protected resources (APIs) |
| **ID Token** | JWT containing user identity information |
| **Refresh Token** | Long-lived token used to obtain new access tokens |
| **State Parameter** | Random value for CSRF protection in OAuth flow |
| **PKCE** | Proof Key for Code Exchange - enhanced security for native apps |
| **Redirect URI** | URL where provider sends user after authentication |
| **Client ID** | Public identifier for the application |
| **Client Secret** | Confidential key shared between app and provider |
| **Scope** | Defines what permissions the application requests |

---

## References

- [OpenID Connect Core 1.0 Specification](https://openid.net/specs/openid-connect-core-1_0.html)
- [OAuth 2.0 Authorization Code Flow RFC 6749](https://tools.ietf.org/html/rfc6749)
- [PKCE RFC 7636](https://tools.ietf.org/html/rfc7636)
- [Qt 5 Documentation](https://doc.qt.io)
- [QWebEngineView Documentation](https://doc.qt.io/qt-5/qwebengineview.html)

---

## Document History

| Date | Version | Changes |
|------|---------|---------|
| 2026-04-12 | 1.0 | Initial architecture documentation |


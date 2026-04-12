# OIDC Qt Client - System Design & Data Flow Diagrams

## Table of Contents
1. [System Architecture Diagrams](#system-architecture-diagrams)
2. [Sequence Diagrams](#sequence-diagrams)
3. [State Machines](#state-machines)
4. [Data Structures](#data-structures)

---

## System Architecture Diagrams

### Complete Application Architecture

```

       ┌────────────────────────────────────────────────────────────┐
       │                                                              │
       │  User's Computer / Desktop Environment                      │
       │                                                              │
       │  ┌──────────────────────────────────────────────────────┐  │
       │  │                                                       │  │
       │  │  OIDC Qt Client Application                          │  │
       │  │                                                       │  │
       │  │  ┌───────────────────────────────────────────────┐  │  │
       │  │  │  MainWindow                                   │  │  │
       │  │  │                                               │  │  │
       │  │  │  ┌─────────────────────────────────────────┐ │  │  │
       │  │  │  │ showLoginDialog()                       │ │  │  │
       │  │  │  └──────────┬──────────────────────────────┘ │  │  │
       │  │  │             │                                │  │  │
       │  │  │  ┌──────────▼──────────────────────────────┐ │  │  │
       │  │  │  │  LoginDialog (Modal)                   │ │  │  │
       │  │  │  │                                        │ │  │  │
       │  │  │  │  ┌────────────────────────────────┐   │ │  │  │
       │  │  │  │  │  QWebEngineView                │   │ │  │  │
       │  │  │  │  │  ┌──────────────────────────┐ │   │ │  │  │
       │  │  │  │  │  │ Renders Login Page       │ │   │ │  │  │
       │  │  │  │  │  │ (OIDC Provider)          │ │   │ │  │  │
       │  │  │  │  │  └──────────────────────────┘ │   │ │  │  │
       │  │  │  │  │                                │   │ │  │  │
       │  │  │  │  │ onUrlChanged()                 │   │ │  │  │
       │  │  │  │  │ onLoadFinished()               │   │ │  │  │
       │  │  │  │  └────────────────────────────────┘   │ │  │  │
       │  │  │  │                                        │ │  │  │
       │  │  │  └────────────────────────────────────────┘ │  │  │
       │  │  │                                             │  │  │
       │  │  └─────────────────────────────────────────────┘  │  │
       │  │                                                   │  │
       │  │  ┌──────────────────────────────────────────────┐  │  │
       │  │  │  OIDCConfig                                  │  │  │
       │  │  │  • Stores provider URLs                      │  │  │
       │  │  │  • Generates auth endpoints                  │  │  │
       │  │  │  • Manages state parameters                  │  │  │
       │  │  └──────────────────────────────────────────────┘  │  │
       │  │                                                   │  │
       │  │  ┌──────────────────────────────────────────────┐  │  │
       │  │  │  Configuration                              │  │  │
       │  │  │  • config.ini (runtime)                     │  │  │
       │  │  │  • Defaults (fallback)                      │  │  │
       │  │  │  • Environment variables (future)           │  │  │
       │  │  └──────────────────────────────────────────────┘  │  │
       │  │                                                   │  │
       │  └──────────────────────────────────────────────────┘  │
       │                                                        │
       │  ┌──────────────────────────────────────────────────┐  │
       │  │  Qt5 Runtime                                     │  │
       │  │  • QApplication, QMainWindow, QDialog           │  │
       │  │  • QWebEngineView (Chromium-based)             │  │
       │  │  • QSettings (Configuration)                    │  │
       │  │  • Signal-Slot mechanism                       │  │
       │  └──────────────────────────────────────────────────┘  │
       │                                                        │
       └────────────────────────────────────────────────────────┘
                           │
                           │ HTTPS
                           │
                           ▼
       ┌────────────────────────────────────────────────────────────┐
       │                                                              │
       │  OIDC Provider (Authentication Server)                     │
       │  (Google, Azure AD, Keycloak, etc.)                        │
       │                                                              │
       │  ┌──────────────────────────────────────────────────────┐  │
       │  │ Step 1: Authorization Endpoint                       │  │
       │  │ GET /oauth/authorize?response_type=code&...         │  │
       │  └──────────────────────────────────────────────────────┘  │
       │                                                              │
       │  ┌──────────────────────────────────────────────────────┐  │
       │  │ Step 2: User Authentication & Consent              │  │
       │  │ • Login form                                        │  │
       │  │ • Credential validation                            │  │
       │  │ • Permission confirmation                          │  │
       │  └──────────────────────────────────────────────────────┘  │
       │                                                              │
       │  ┌──────────────────────────────────────────────────────┐  │
       │  │ Step 3: Generate & Send Authorization Code          │  │
       │  │ Redirect to: http://localhost:8080/callback?code=...   │
       │  └──────────────────────────────────────────────────────┘  │
       │                                                              │
       │  ┌──────────────────────────────────────────────────────┐  │
       │  │ [Future] Step 4: Token Endpoint                      │  │
       │  │ POST /oauth/token (server-to-server)               │  │
       │  │ Exchange code for tokens                           │  │
       │  └──────────────────────────────────────────────────────┘  │
       │                                                              │
       └────────────────────────────────────────────────────────────┘
```

### Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Qt5 Framework                             │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ QApplication: Manages application lifecycle             │  │
│  └──────────────┬───────────────────────────────────────────┘  │
│                 │                                                │
│  ┌──────────────▼───────────────────────────────────────────┐  │
│  │ MainWindow: Application container                        │  │
│  │   • showEvent() - triggered on first show               │  │
│  │   • initializeOIDCConfig() - load config                │  │
│  │   • onLoginSucceeded() - handle auth success            │  │
│  │   • onLoginFailed() - handle auth failure               │  │
│  └────────┬─────────────────────────────────────────────────┘  │
│           │                                                      │
│  ┌────────▼─────────────────────────────────────────────────┐  │
│  │ OIDCConfig: Configuration management                    │  │
│  │   • Stores provider settings                            │  │
│  │   • Generates authorization URLs                        │  │
│  │   • Creates state parameters (CSRF protection)          │  │
│  └────────┬─────────────────────────────────────────────────┘  │
│           │                                                      │
│  ┌────────▼─────────────────────────────────────────────────┐  │
│  │ LoginDialog: Authentication UI                          │  │
│  │   • Displays login form via QWebEngineView             │  │
│  │   • Monitors URL changes                               │  │
│  │   • Extracts authorization code                        │  │
│  │   • Emits loginSucceeded() / loginFailed() signals    │  │
│  └────────┬─────────────────────────────────────────────────┘  │
│           │                                                      │
│  ┌────────▼─────────────────────────────────────────────────┐  │
│  │ QWebEngineView: Web rendering engine                    │  │
│  │   • Renders OIDC provider login page                   │  │
│  │   • Handles user interactions                          │  │
│  │   • Emits urlChanged() signal on redirects            │  │
│  │   • Emits loadFinished() signal                       │  │
│  └────────┬─────────────────────────────────────────────────┘  │
│           │                                                      │
│  ┌────────▼─────────────────────────────────────────────────┐  │
│  │ QSettings: Configuration file handler                   │  │
│  │   • Parses config.ini file                             │  │
│  │   • Provides key-value access to config               │  │
│  │   • Supports fallback defaults                        │  │
│  └─────────────────────────────────────────────────────────┘  │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

---

## Sequence Diagrams

### Application Startup Sequence

```
User
  │
  ├─► Application.exe
  │   │
  │   ├─► main()
  │   │   │
  │   │   ├─► QApplication app(argc, argv)
  │   │   │
  │   │   ├─► MainWindow window
  │   │   │   │
  │   │   │   ├─► MainWindow::MainWindow()
  │   │   │   │   │
  │   │   │   │   ├─► initializeOIDCConfig()
  │   │   │   │   │   │
  │   │   │   │   │   ├─► new OIDCConfig()
  │   │   │   │   │   │
  │   │   │   │   │   ├─► loadConfigFromFile()
  │   │   │   │   │   │   │
  │   │   │   │   │   │   ├─► QFileInfo::exists(./config.ini)
  │   │   │   │   │   │   │
  │   │   │   │   │   │   └─► QSettings(config.ini)
  │   │   │   │   │   │       └─► return(true)
  │   │   │   │   │   │
  │   │   │   │   │   ├─► config->setAuthorizationUrl(...)
  │   │   │   │   │   ├─► config->setTokenUrl(...)
  │   │   │   │   │   ├─► config->setClientId(...)
  │   │   │   │   │   ├─► config->setRedirectUri(...)
  │   │   │   │   │   └─► config->setScope(...)
  │   │   │   │   │
  │   │   │   │   └─► Create UI (label, layout)
  │   │   │   │
  │   │   │   └─► return
  │   │   │
  │   │   ├─► window.show()
  │   │   │   │
  │   │   │   └─► schedules showEvent()
  │   │   │
  │   │   └─► [Event loop running]
  │   │       │
  │   │       ├─► showEvent() triggered
  │   │       │   │
  │   │       │   ├─► m_loginAttempted = true
  │   │       │   │
  │   │       │   └─► QTimer::singleShot(0, showLoginDialog)
  │   │       │       │
  │   │       │       └─► showLoginDialog()
  │   │       │           │
  │   │       │           ├─► new LoginDialog(config)
  │   │       │           │   │
  │   │       │           │   ├─► new QWebEngineView()
  │   │       │           │   │
  │   │       │           │   ├─► config.getAuthorizationEndpoint()
  │   │       │           │   │   Returns: https://auth.provider.com/...?code=...&state=...
  │   │       │           │   │
  │   │       │           │   ├─► m_webView->load(authUrl)
  │   │       │           │   │
  │   │       │           │   ├─► connect(m_webView, &QWebEngineView::urlChanged,
  │   │       │           │   │            this, &LoginDialog::onUrlChanged)
  │   │       │           │   │
  │   │       │           │   └─► connect(m_webView, &QWebEngineView::loadFinished,
  │   │       │           │            this, &LoginDialog::onLoadFinished)
  │   │       │           │
  │   │       │           ├─► dialog.exec()  [Modal - blocks until accepted/rejected]
  │   │       │           │
  │   │       │           └─► [Waiting for user...]
  │   │       │
  │   └─► app.exec()  [Start Qt event loop]

```

### Authentication Success Sequence

```
User (in Browser)                      LoginDialog                         QWebEngineView
        │                                   │                                   │
        │ Clicks Login Button               │                                   │
        ├──────────────────────────────────►│                                   │
        │                                   │ load(authorizationUrl)            │
        │                                   ├──────────────────────────────────►│
        │                                   │                                   │
        │                                   │                                   │ loads page
        │                                   │                                   │
        ├───────────────────────────────────────────────────[ User enters credentials ]
        │                                   │                                   │
        │ Submits Credentials               │                                   │
        ├──────────────────────────────────────────────────────────────────────►│
        │                                   │                                   │
        │                                   │                                   │ sends to provider
        │                                   │                                   │
        │◄───────────────────────────────────────────────────[ Provider validates ]
        │                                   │                                   │
        │                                   │                                   │
        │ Gets Redirected                   │                                   │
        ├──────────────────────────────────────────────────────────────────────►│
        │                                   │                                   │ navigate to redirect
        │                                   │                                   │
        │                                   │◄──────────────────────────────────┤ urlChanged signal
        │                                   │ onUrlChanged(redirect_url)        │
        │                                   │ http://localhost:8080/callback?code=...&state=...
        │                                   │
        │                                   ├─► parseRedirectUrl()
        │                                   │   ├─► QUrlQuery query(url)
        │                                   │   ├─► extract "code" parameter
        │                                   │   ├─► extract "state" parameter
        │                                   │   └─► emit loginSucceeded(code, state)
        │                                   │
        │                                   ├─► dialog.accept()
        │                                   │
        MainWindow                          │
           │◄─ signal: loginSucceeded()─────┤
           │
           ├─► onLoginSucceeded(authCode, state)
           │   ├─► m_isAuthenticated = true
           │   ├─► QMessageBox::information() [Show success]
           │   └─► grantApplicationAccess()
           │
           └─► [Application continues with authenticated user]
```

### Authentication Failure Sequence

```
User (in Browser)              LoginDialog                    MainWindow
        │                               │                         │
        │                               │                         │
        ├─── Denies Permission ────────►│                         │
        │                               │                         │
        │                               ├─ redirect with error───►│
        │                               │                         │
        │                               ├─ onUrlChanged()         │
        │                               │ parseRedirectUrl()       │
        │                               │ detects error parameter  │
        │                               │                         │
        │                               ├─ emit loginFailed()─────►
        │                               │                         │
        │◄─ Redirect back ───────────────┤                         │
        │                               │                         │
        │                               │ reject()                │
        │                               │                         │ onLoginFailed(error)
        │                               │                         │ ├─ QMessageBox::critical()
        │                               └───────────────────────────┤ └─ close()
        │                                                           │
        └─[Dialog closed, application exits]
```

---

## State Machines

### MainWindow State Machine

```
         ┌──────────────────┐
         │  INITIALIZING    │  (Constructor called)
         │  • Load config   │
         │  • Create UI     │
         └────────┬─────────┘
                  │
                  ▼
         ┌──────────────────────────────────┐
         │  WAITING_FOR_DISPLAY             │  (showEvent not yet called)
         │  • Window exists                 │
         │  • Not visible yet               │
         └────────┬─────────────────────────┘
                  │
                  │ window.show() called
                  ▼
         ┌──────────────────────────────────┐
         │  SHOWING_LOGIN                   │  (showEvent triggered)
         │  • Display LoginDialog modal     │
         │  • Wait for user authentication  │
         │  • m_loginAttempted = true       │
         └────────┬──────┬──────────────────┘
                  │      │
        ┌─────────┘      └─────────┐
        │                          │
        ▼                          ▼
   [Success]                  [Failure/Cancel]
        │                          │
        │                          ▼
        │              ┌──────────────────────┐
        │              │ AUTHENTICATION_FAILED│
        │              │ • Show error dialog  │
        │              │ • Close application  │
        │              └──────────────────────┘
        │
        ▼
   ┌──────────────────────────────┐
   │ AUTHENTICATED                │
   │ • m_isAuthenticated = true   │
   │ • Show success message       │
   │ • Grant application access   │
   └─────────┬────────────────────┘
             │
             ▼
   ┌──────────────────────────────┐
   │ APPLICATION_RUNNING          │
   │ • User can interact with app │
   │ • Main functionality enabled │
   └──────────────────────────────┘
```

### LoginDialog State Machine

```
         ┌──────────────────────┐
         │  INITIALIZING        │
         │  • Create WebEngine  │
         │  • Setup signals     │
         └────────┬─────────────┘
                  │
                  ▼
         ┌──────────────────────┐
         │  LOADING             │
         │  • Load auth URL     │
         │  • Wait for page     │
         └────────┬─────────────┘
                  │
                  ▼
         ┌──────────────────────────────┐
         │  WAITING_FOR_REDIRECT        │
         │  • Monitor URL changes       │
         │  • User fills form           │
         │  • Awaits provider response  │
         └────────┬────────┬────────────┘
                  │        │
         ┌────────┘        └──────────┐
         │                            │
         ▼                            ▼
    [Redirect]                   [Error or Cancel]
         │                            │
         ▼                            ▼
   ┌───────────────┐         ┌──────────────┐
   │REDIRECT_      │         │ERROR         │
   │DETECTED       │         │emit failed() │
   │parseRedirect()│         │reject()      │
   └───────┬───────┘         └──────────────┘
           │
           ├─► Check for "error" param
           │   ├─ Yes -> emit loginFailed()
           │   │         reject()
           │   │
           │   └─ No -> Continue
           │
           ├─► Extract "code" param
           │   ├─ Found -> Continue
           │   │
           │   └─ Not found -> emit loginFailed()
           │                   reject()
           │
           └─► emit loginSucceeded(code, state)
               accept()
                   │
                   ▼
              ┌──────────────┐
              │CLOSED        │
              │Dialog done   │
              └──────────────┘
```

---

## Data Structures

### OIDCConfig Private Members

```
┌─────────────────────────────────────────────┐
│ OIDCConfig                                   │
└─────────────────────────────────────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
        ▼                       ▼
   AuthProvider           Application
   Settings                Settings
        │                       │
        ├─ m_authorizationUrl   ├─ m_clientId
        ├─ m_tokenUrl           ├─ m_clientSecret
        └─ m_scope              ├─ m_redirectUri
                                └─ ...

Typical Values:
{
  m_authorizationUrl = "https://accounts.google.com/o/oauth2/v2/auth",
  m_tokenUrl = "https://oauth2.googleapis.com/token",
  m_clientId = "1234567890-abcdef.apps.googleusercontent.com",
  m_clientSecret = "GOCSPX-abc...",
  m_redirectUri = "http://localhost:8080/callback",
  m_scope = "openid profile email"
}
```

### Authorization Request URL Structure

```
GET /oauth/authorize?
    response_type=code
    &client_id=YOUR_CLIENT_ID
    &redirect_uri=http%3A%2F%2Flocalhost%3A8080%2Fcallback
    &scope=openid+profile+email
    &state=550e8400-e29b-41d4-a716-446655440000
    &// ... other provider-specific params

Example Full URL:
https://accounts.google.com/o/oauth2/v2/auth?
response_type=code&
client_id=1234567890.apps.googleusercontent.com&
redirect_uri=http%3A%2F%2Flocalhost%3A8080%2Fcallback&
scope=openid+profile+email&
state=550e8400-e29b-41d4-a716-446655440000
```

### Authorization Response (Redirect URL)

```
Success:
http://localhost:8080/callback?
  code=4/0AY8e8bL8h...
  &scope=openid+profile+email+openid
  &state=550e8400-e29b-41d4-a716-446655440000
  &expires_in=3599

Error:
http://localhost:8080/callback?
  error=access_denied
  &error_description=User+denied+access
  &state=550e8400-e29b-41d4-a716-446655440000
```

### Configuration File Structure

```
config.ini (INI format)

[OIDC]                          ← Section identifier
├─ AuthorizationUrl=...         ← Key = Value
├─ TokenUrl=...
├─ ClientId=...
├─ ClientSecret=...
├─ RedirectUri=...
└─ Scope=...

[Application]
├─ WindowWidth=1000
├─ WindowHeight=700
└─ ListenPort=8080

Parsed into memory as:
┌─────────────────────┐
│  QSettings object   │
│  (key-value pairs)  │
├─────────────────────┤
│ OIDC/AuthorizationUrl
│ OIDC/TokenUrl
│ OIDC/ClientId
│ OIDC/ClientSecret
│ OIDC/RedirectUri
│ OIDC/Scope
│ Application/WindowWidth
│ Application/WindowHeight
│ Application/ListenPort
└─────────────────────┘
```

### Signal-Slot Connection Map

```
Signals Emitted:
├─ LoginDialog::loginSucceeded(authCode, state)
│  └─► Connected to: MainWindow::onLoginSucceeded(authCode, state)
│
└─ LoginDialog::loginFailed(error)
   └─► Connected to: MainWindow::onLoginFailed(error)

Qt Signals:
├─ QWebEngineView::urlChanged(QUrl)
│  └─► Connected to: LoginDialog::onUrlChanged(QUrl)
│
├─ QWebEngineView::loadFinished(bool)
│  └─► Connected to: LoginDialog::onLoadFinished(bool)
│
└─ MainWindow::showEvent(QShowEvent*)
   ├─► Schedules: QTimer::singleShot(0, showLoginDialog)
   │
   └─► Prevents re-showing via: m_loginAttempted flag
```

---

## Interaction Patterns

### Observer Pattern (Qt Signals)
The application uses Qt's Signal-Slot mechanism which implements the Observer pattern:

```
LoginDialog (Subject)
    │
    ├─ Signal: loginSucceeded()
    │   └─► Slot: MainWindow::onLoginSucceeded()
    │
    └─ Signal: loginFailed()
        └─► Slot: MainWindow::onLoginFailed()

QWebEngineView (Subject)
    │
    ├─ Signal: urlChanged()
    │   └─► Slot: LoginDialog::onUrlChanged()
    │
    └─ Signal: loadFinished()
        └─► Slot: LoginDialog::onLoadFinished()
```

### Strategy Pattern (Configuration)
OIDCConfig encapsulates OIDC provider configuration:

```
Different OIDC Providers (Strategy):
├─ Google OAuth
├─ Microsoft Azure AD
├─ Keycloak
├─ Okta
└─ Generic OIDC

All use same interface but different:
├─ Authorization URLs
├─ Token Endpoints
├─ Additional Parameters
└─ Scope Names
```

### Factory Pattern (Potential)
Future token exchange could use factory pattern:

```
TokenFactory
    │
    ├─ createAuthorizationCodeToken()
    ├─ createRefreshToken()  
    ├─ createAccessToken()
    └─ createIdToken()
```

---


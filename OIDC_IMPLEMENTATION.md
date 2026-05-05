# OIDC Qt Client

An OpenID Connect (OIDC) login client built with Qt5 supporting multiple authentication methods:

1. **LoginDialog** - QWebEngineView browser-based login (interactive OAuth2 Authorization Code Flow)
2. **SimpleLoginDialog** - Direct username/password authentication (Resource Owner Password Credentials)
3. **AuthCodeDialog** - Browser + manual code entry (OAuth2 Authorization Code Flow with manual redirect handling)

## Architecture

The OIDC login system consists of these components:

### 1. **OIDCConfig** (`oidcconfig.h/cpp`)
- Configuration holder for OIDC parameters
- Generates authorization endpoint URL with proper query parameters
- Generates secure state parameter for CSRF protection
- Supports customizable:
  - Authorization URL
  - Token endpoint URL
  - Client ID and secret
  - Redirect URI
  - OAuth scopes

### 2. **LoginDialog** (`logindialog.h/cpp`) - QWebEngineView Method
- Modal dialog displaying the OIDC login page using QWebEngineView
- Monitors URL changes to detect redirect back to your application
- Parses the authorization code from the redirect URL
- Handles error responses from the OIDC provider
- Emits signals for successful/failed login attempts
- **Note**: Not recommended for WSL 2 due to GPU rendering issues

### 3. **SimpleLoginDialog** (`simplelogindialog.h/cpp`) - Direct Credentials Method
- Modal dialog with username and password input fields
- Sends credentials directly to OIDC provider's token endpoint
- Uses OAuth2 Resource Owner Password Credentials flow (password grant)
- Parses JSON token response
- **Requirements**: OIDC provider must support password grant flow
- **WSL 2 Compatible**: Yes (includes SSL error handling)

### 4. **AuthCodeDialog** (`authcodedialog.h/cpp`) - Browser + Manual Code Method
- Modal dialog for the most secure Authorization Code flow
- Automatically copies authorization URL to clipboard
- Opens system default browser for user authorization
- Accepts manual authorization code entry
- Exchanges code for tokens
- **Recommended for WSL 2**: Yes (no rendering requirements)

### 5. **MainWindow** (Updated)
- Shows appropriate login dialog automatically on application startup
- Handles successful/failed login responses
- **Exchanges authorization code for tokens** (NEW)
- Parses token response and stores tokens securely
- Grants application access after successful authentication
- Sets up main UI for authenticated users
- Supports switching between login methods

## Configuration

### Configuration via Code
Edit `mainwindow.cpp` in the `initializeOIDCConfig()` method to set your OIDC parameters:

```cpp
m_oidcConfig->setAuthorizationUrl("https://your-auth-provider.com/oauth/authorize");
m_oidcConfig->setTokenUrl("https://your-auth-provider.com/oauth/token");
m_oidcConfig->setClientId("your-client-id");
m_oidcConfig->setClientSecret("your-client-secret");
m_oidcConfig->setRedirectUri("http://localhost:8080/callback");
m_oidcConfig->setScope("openid profile email");
```

### Configuration via File (Future Enhancement)
A configuration file template is provided in `config.ini.example`. You can extend the code to load from this file.

## Building

```bash
cd /home/coonal/code/oidc-client-qt
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Running

```bash
# Using the run script (handles environment setup)
cd /home/coonal/code/oidc-client-qt
chmod +x run.sh
./run.sh

# Or directly
./build/oidc-client-qt
```

## Switching Between Login Methods

To use different login methods, update the `showEvent()` method in `mainwindow.cpp`:

```cpp
// In MainWindow::showEvent()

// Option 1: Browser-based (not recommended for WSL 2)
QTimer::singleShot(0, this, &MainWindow::showLoginDialog);

// Option 2: Direct credentials (if provider supports password grant)
QTimer::singleShot(0, this, &MainWindow::showSimpleLoginDialog);

// Option 3: Browser + manual code (recommended for WSL 2)
QTimer::singleShot(0, this, &MainWindow::showAuthCodeDialog);
```

## WSL 2 Compatibility

### Issue
QWebEngineView (Chromium-based) has GPU rendering problems in WSL 2, resulting in blank white windows.

### Solutions

**Option 1: Use AuthCodeDialog (Recommended)**
```cpp
void MainWindow::showEvent(QShowEvent *event)
{
    if (!m_loginAttempted) {
        m_loginAttempted = true;
        QTimer::singleShot(0, this, &MainWindow::showAuthCodeDialog);  // ← Browser + code entry
    }
    QMainWindow::showEvent(event);
}
```

**Option 2: Use SimpleLoginDialog (if provider supports password grant)**
```cpp
QTimer::singleShot(0, this, &MainWindow::showSimpleLoginDialog);
```

**Option 3: Fix GPU Issues (Advanced)**
```bash
# In main.cpp before QApplication creation:
qputenv("QT_XCB_GL_INTEGRATION", "none");
qputenv("LIBGL_ALWAYS_INDIRECT", "1");
qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");

# Then rely on OpenSSL being properly configured
sudo apt install libssl-dev libnss3-dev
```

## SSL Certificate Errors in WSL 2

### Issue
Qt Network reports SSL errors like:
```
qt.network.ssl: QSslSocket: cannot resolve EVP_PKEY_base_id
qt.network.ssl: QSslSocket: cannot resolve SSL_get_peer_certificate
Network error: "SSL handshake failed"
```

### Solution
Both SimpleLoginDialog and AuthCodeDialog include SSL error handling for development:

```cpp
// In authenticateWithCredentials() and exchangeAuthorizationCode()
connect(m_currentReply, QOverload<const QList<QSslError>&>::of(&QNetworkReply::sslErrors),
        [this](const QList<QSslError> &errors) {
            qWarning() << "SSL errors encountered:" << errors.count();
            m_currentReply->ignoreSslErrors();  // Ignore for development
        });
```

⚠️ **Warning**: This disables SSL verification for development only. **Never use in production**.

## OIDC Flow Comparison

| Feature | LoginDialog | SimpleLoginDialog | AuthCodeDialog |
|---------|---|---|---|
| **Flow** | Authorization Code | Resource Owner Password Credentials | Authorization Code |
| **Browser** | QWebEngineView | None | System default |
| **User Input** | Interactive logon page | Username + Password | Authorization code |
| **WSL 2** | ❌ GPU issues | ✅ Works | ✅ Recommended |
| **Security** | ✅ High (no credentials shared) | ⚠️ Medium (credentials sent) | ✅ High (no credentials shared) |
| **Provider Support** | ✅ Standard | ⚠️ Optional (password grant) | ✅ Standard |
| **Code complexity** | Medium | Low | Medium |

## Build Environment Issue

If you encounter linker errors about missing NSS or ALSA libraries, this is due to system library dependencies:
- The Qt5 WebEngine library requires NSS (Mozilla Network Security Services)
- The Qt5 WebEngine library requires ALSA (Advanced Linux Sound Architecture)

**Solution**: Install the required development packages:

```bash
sudo apt update
sudo apt install libasound2-dev libssl-dev libnss3-dev libdbus-1-dev libxcomposite-dev
```

## Token Exchange Implementation (✓ Completed)

The authorization code is now automatically exchanged for tokens after successful login.

### Implementation Details

**Flow**:
1. User completes login → `onLoginSucceeded()` is called with authorization code
2. `exchangeAuthCodeForTokens()` sends POST request to token endpoint
3. Token endpoint returns access_token, refresh_token, and id_token
4. Tokens are parsed and stored in member variables
5. Success message displays token information
6. `grantApplicationAccess()` is called to proceed with application

**Token Exchange Request** (`OIDCConfig::getTokenExchangeBody()`):
```cpp
// POST to token endpoint with form data:
grant_type=authorization_code
code=<authorization_code>
redirect_uri=<redirect_uri>
client_id=<client_id>
client_secret=<client_secret>
```

**Token Exchange Response**:
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refresh_token": "abc123def456...",
  "id_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "token_type": "Bearer",
  "expires_in": 3600
}
```

**Tokens Stored In**:
- `MainWindow::m_accessToken` - For API requests
- `MainWindow::m_refreshToken` - For token renewal
- `MainWindow::m_idToken` - Contains user identity information

### Next Steps

After receiving tokens (either via SimpleLoginDialog, AuthCodeDialog, or after LoginDialog exchanges code):

1. **Store Tokens Securely** (in `mainwindow.cpp`'s `onLoginSucceeded`):
   ```cpp
   // Store access_token, id_token, refresh_token securely
   // Use OS keychain for secure storage
   ```
   
   - Implement secure storage using OS keychain
   - Never log tokens to console in production
   - Encrypt tokens if storing to disk
   
2. **Use Access Token for API Requests**:
   ```cpp
   QNetworkRequest request(apiUrl);
   request.setRawHeader("Authorization", 
       QString("Bearer %1").arg(m_accessToken).toUtf8());
   m_networkManager->get(request);
   ```

3. **Implement Token Refresh**:
   - Track token expiration
   - Refresh tokens before they expire
   - Handle 401 Unauthorized responses

4. **Token Refresh**:
   - Monitor token expiration (expires_in field)
   - Use refresh_token to obtain new access_token before expiry
   - Implement automatic refresh logic

5. **Logout Functionality**:
   - Send token revocation request to provider
   - Clear stored tokens
   - Close application or show login dialog again

## Security Considerations

✓ **Implemented**:
- State parameter generation for CSRF protection
- Authorization code flow (most secure for native apps)
- Redirect URI validation
- SSL error handling for development

⚠️ **To Implement**:
- Secure token storage (keychain/secure enclave)
- Token refresh token handling
- Code verifier/challenge (PKCE) for enhanced security
- Logout with token revocation
- HTTPS enforcement for production

## Classes Overview

### OIDCConfig
```cpp
// Set configuration
OIDCConfig config;
config.setAuthorizationUrl("...");
config.setClientId("...");
config.setTokenUrl("...");
config.setClientSecret("...");

// Get authorization URL
QUrl authUrl = config.getAuthorizationEndpoint();

// Prepare token exchange request body
QByteArray tokenBody = config.getTokenExchangeBody(authCode);
```

### AuthCodeDialog
```cpp
// Create and show dialog
AuthCodeDialog dialog(config);
connect(&dialog, &AuthCodeDialog::loginSucceeded, 
        this, [](const QString &token) { /* use token */ });

int result = dialog.exec(); // Modal dialog
```

### SimpleLoginDialog (If password grant supported)
```cpp
// Create and show dialog
SimpleLoginDialog dialog(config);
connect(&dialog, &SimpleLoginDialog::loginSucceeded, 
        this, [](const QString &token) { /* use token */ });

int result = dialog.exec(); // Modal dialog
```

### LoginDialog (Browser method)
```cpp
// Create and show dialog  
LoginDialog dialog(config);
connect(&dialog, &LoginDialog::loginSucceeded, 
        this, &MainWindow::onLoginSucceeded);

int result = dialog.exec(); // Modal dialog
```

### MainWindow
```cpp
// Automatically handles login flow:
// - Shows appropriate login dialog on startup
// - Handles authentication responses
// - Grants access to main application
```

## Troubleshooting

### LoginDialog: Blank white window
1. Verify OIDC URLs are correct and reachable
2. Check browser console for JavaScript errors
3. Use AuthCodeDialog instead (WSL 2 recommended)

### SimpleLoginDialog: 400 unsupported_grant_type Error
- OIDC provider doesn't support Resource Owner Password Credentials flow
- Use AuthCodeDialog instead

### AuthCodeDialog: Code exchange fails
- Verify redirect URI matches provider configuration
- Ensure authorization code is valid and not expired
- Check client_id and client_secret are correct

### SSL/Network errors in WSL 2
- Both SimpleLoginDialog and AuthCodeDialog include SSL error recovery
- SSL verification is disabled for development by default
- For production, properly configure OpenSSL in WSL

### Authorization code not received
- Verify redirect URI matches OIDC provider configuration
- Ensure you can access your OIDC provider's website
- Check that callback page handling is correct

### Build fails
- Ensure Qt5 is installed with required components
- Check CMakeLists.txt paths match your Qt installation
- Install required system development libraries

## References

- [Qt QWebEngineView Documentation](https://doc.qt.io/qt-5/qwebengineview.html)
- [OpenID Connect Specification](https://openid.net/specs/openid-connect-core-1_0.html)
- [OAuth 2.0 Authorization Code Flow](https://tools.ietf.org/html/rfc6749#section-1.3.1)
- [OAuth 2.0 Resource Owner Password Credentials](https://tools.ietf.org/html/rfc6749#section-1.3.3)

# OIDC Qt Client

An OpenID Connect (OIDC) login client built with Qt5 using QWebEngineView for displaying the login page.

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

### 2. **LoginDialog** (`logindialog.h/cpp`)
- Modal dialog displaying the OIDC login page using QWebEngineView
- Monitors URL changes to detect redirect back to your application
- Parses the authorization code from the redirect URL
- Handles error responses from the OIDC provider
- Emits signals for successful/failed login attempts

### 3. **MainWindow** (Updated)
- Shows login dialog automatically on application startup
- Handles successful/failed login responses
- **Exchanges authorization code for tokens** (NEW)
- Parses token response and stores tokens securely
- Grants application access after successful authentication
- Sets up main UI for authenticated users

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

## Build Environment Issue

If you encounter linker errors about missing NSS or ALSA libraries, this is due to system library dependencies:
- The Qt5 WebEngine library requires NSS (Mozilla Network Security Services)
- The Qt5 WebEngine library requires ALSA (Advanced Linux Sound Architecture)

**Solution**: Install the required development packages or use the pre-built application if available.

The most direct fix is to install the ALSA development package in your WSL Ubuntu terminal. This provides the symbols the linker is currently failing to find:

```bash
sudo apt update
sudo apt install libasound2-dev
```

Qt WebEngine is based on Chromium and has a massive list of system dependencies. If you fixed ALSA and started seeing errors for NSS, DBUS, or X11, you should install the full set of required libraries:

```bash
sudo apt install libnss3-dev libdbus-1-dev libxcomposite-dev libxcursor-dev \
libxi-dev libxtst-dev libxrandr-dev libfontconfig1-dev libxss-dev libpci-dev
```

## OIDC Flow Overview

1. **Application Startup**: MainWindow shows a login dialog
2. **Authorization Request**: LoginDialog loads the OIDC authorization endpoint in QWebEngineView
3. **User Authentication**: User logs in with their credentials in the browser
4. **Authorization Grant**: OIDC provider redirects back with authorization code
5. **Code Extraction**: LoginDialog detects redirect and extracts the authorization code
6. **Token Exchange**: Application should exchange the code for tokens (implementation-dependent on backend)
7. **Access Granted**: User gains access to the application

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

1. **Use Access Token for API Requests**:
   ```cpp
   QNetworkRequest request(apiUrl);
   request.setRawHeader("Authorization", 
       QString("Bearer %1").arg(m_accessToken).toUtf8());
   m_networkManager->get(request);
   ```

2. **Secure Token Storage**:
   - Implement secure storage using OS keychain
   - Never log tokens to console in production
   - Encrypt tokens if storing to disk

3. **Token Refresh**:
   - Monitor token expiration (expires_in field)
   - Use refresh_token to obtain new access_token before expiry
   - Implement automatic refresh logic

4. **Logout Functionality**:
   - Send token revocation request to provider
   - Clear stored tokens
   - Close application or show login dialog again

## Security Considerations

✓ **Implemented**:
- State parameter generation for CSRF protection
- Authorization code flow (most secure for native apps)
- Redirect URI validation

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

### LoginDialog
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
// - Shows login dialog on startup
// - Handles successful/failed authentication
// - Grants access to main application
```

## Troubleshooting

### Dialog doesn't show
- Check that QWebEngineView is properly initialized
- Verify OIDC URLs are correct
- Check browser console for JavaScript errors

### Authorization code not received
- Verify redirect URI matches OIDC provider configuration
- Check that LoginDialog is correctly monitoring URL changes
- Ensure browser allows redirects

### Build fails
- Ensure Qt5 is installed with WebEngine component
- Check CMakeLists.txt paths match your Qt installation
- Install required system development libraries

## References

- [Qt QWebEngineView Documentation](https://doc.qt.io/qt-5/qwebengineview.html)
- [OpenID Connect Specification](https://openid.net/specs/openid-connect-core-1_0.html)
- [OAuth 2.0 Authorization Code Flow](https://tools.ietf.org/html/rfc6749#section-1.3.1)

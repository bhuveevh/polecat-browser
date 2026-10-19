#pragma once

#include <QAuthenticator>
#include <QWebEngineCertificateError>
#include <QWebEngineFullScreenRequest>
#include <QWebEnginePage>
#include <QUrl>
#include <functional>

class QWebEngineProfile;

namespace Polecat {

class SettingsManager;

// WebPage centralises all page-level *policy* so that BrowserTab and the
// window chrome stay thin:
//   * new-window requests            -> createWindowRequested()
//   * certificate errors             -> explicit, never silently bypassed
//   * feature permissions            -> permissionRequested()
//   * full-screen requests           -> fullScreenRequested()
//   * HTTP authentication            -> authenticationRequired()
//   * optional HTTPS-only upgrades   -> upgradeRequested()
//   * JavaScript console messages    -> forwarded to the log category
class WebPage : public QWebEnginePage
{
    Q_OBJECT

public:
    WebPage(QWebEngineProfile *profile,
            SettingsManager *settings,
            QObject *parent = nullptr);

    // Set to true for pages hosted in a private (off-the-record) window. Used
    // by callers to decide whether history/downloads may be persisted.
    void setPrivate(bool isPrivate);
    bool isPrivate() const { return m_private; }

    // Allow the owner to answer an earlier permissionRequested() asynchronously.
    void answerPermission(const QUrl &origin, Feature feature, bool granted);

    // Installs a factory used to service createWindow() synchronously. The
    // owner (TabManager) creates a new tab and returns its page; Qt WebEngine
    // then retargets the pending navigation into it. Returning nullptr makes
    // the popup be blocked.
    using WindowFactory = std::function<QWebEnginePage *(WebWindowType)>;
    void setWindowFactory(WindowFactory factory) { m_windowFactory = std::move(factory); }

signals:
    // Emitted when a page asks for a sensitive feature. The owner answers
    // asynchronously with answerPermission().
    void permissionRequested(const QUrl &origin, QWebEnginePage::Feature feature);
    // Emitted when HTTPS-only mode rewrites an http navigation.
    void upgradeRequested(const QUrl &from, const QUrl &to);
    // Forwarded JavaScript console output.
    void consoleMessage(const QString &message, int line, const QString &source);

    // Note: full-screen requests and HTTP authentication use Qt WebEngine's own
    // QWebEnginePage::fullScreenRequested / authenticationRequired signals;
    // we deliberately do not redeclare them here to avoid signal shadowing.

protected:
    QWebEnginePage *createWindow(WebWindowType type) override;
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override;

private:
    void handleFeaturePermission(const QUrl &securityOrigin, Feature feature);
    // Qt 6 delivers certificate errors through the QWebEnginePage::certificateError
    // signal (not a virtual method); the decision is applied on the error object.
    void handleCertificateError(QWebEngineCertificateError error);
    void applySettingsFromManager();

    SettingsManager *m_settings = nullptr;
    bool m_private = false;
    bool m_handlingPermission = false;
    WindowFactory m_windowFactory;
};

} // namespace Polecat

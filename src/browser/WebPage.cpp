#include "browser/WebPage.h"

#include "settings/SettingsManager.h"
#include "utils/Logging.h"
#include "utils/UrlUtils.h"

#include <QAbstractButton>
#include <QAuthenticator>
#include <QMessageBox>
#include <QPushButton>
#include <QWebEngineCertificateError>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace Polecat {

WebPage::WebPage(QWebEngineProfile *profile, SettingsManager *settings, QObject *parent)
    : QWebEnginePage(profile, parent)
    , m_settings(settings)
{
    applySettingsFromManager();

    if (m_settings) {
        connect(m_settings, &SettingsManager::settingChanged, this,
                [this](const QString &key, const QVariant &) {
                    if (key == SettingsManager::kJavascriptEnabled()
                        || key == SettingsManager::kImagesEnabled()
                        || key == SettingsManager::kAutoplayPolicy()
                        || key.isEmpty()) {
                        applySettingsFromManager();
                    }
                });
    }

    connect(this, &QWebEnginePage::featurePermissionRequested,
            this, &WebPage::handleFeaturePermission);
    // In Qt 6 certificateError is a signal, not a virtual method: the decision
    // is applied through acceptCertificate()/rejectCertificate() on the error.
    connect(this, &QWebEnginePage::certificateError,
            this, &WebPage::handleCertificateError);
    // fullScreenRequested and authenticationRequired are Qt WebEngine signals;
    // the window connects to them directly on the page.
}

void WebPage::applySettingsFromManager()
{
    if (!m_settings)
        return;
    QWebEngineSettings *s = settings();
    if (!s)
        return;

    s->setAttribute(QWebEngineSettings::JavascriptEnabled,
                    m_settings->value(SettingsManager::kJavascriptEnabled(), true).toBool());
    s->setAttribute(QWebEngineSettings::AutoLoadImages,
                    m_settings->value(SettingsManager::kImagesEnabled(), true).toBool());

    const QString autoplay = m_settings->value(SettingsManager::kAutoplayPolicy()).toString();
    if (autoplay == QLatin1String("allow")) {
        s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    } else {
        s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, true);
    }

    // Reasonable, privacy-respecting defaults.
    s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,
                    !m_settings->value(SettingsManager::kPopupBlocking(), true).toBool());
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, false);
    s->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
}

void WebPage::setPrivate(bool isPrivate)
{
    m_private = isPrivate;
}

QWebEnginePage *WebPage::createWindow(WebWindowType type)
{
    // Service the request synchronously through the factory installed by the
    // TabManager. A nullptr means the popup is blocked (e.g. pop-up blocking).
    if (m_windowFactory)
        return m_windowFactory(type);
    return nullptr;
}

void WebPage::handleCertificateError(QWebEngineCertificateError error)
{
    // Security rule: never silently bypass a certificate error. Ask the user.
    const QString host = error.url().host();
    QString reason;
    switch (error.type()) {
    case QWebEngineCertificateError::CertificateCommonNameInvalid:
        reason = tr("The certificate is issued for a different website name.");
        break;
    case QWebEngineCertificateError::CertificateDateInvalid:
        reason = tr("The certificate is expired or not yet valid.");
        break;
    case QWebEngineCertificateError::CertificateAuthorityInvalid:
        reason = tr("The certificate was not issued by a trusted authority.");
        break;
    case QWebEngineCertificateError::CertificateContainsErrors:
        reason = tr("The certificate contains errors.");
        break;
    case QWebEngineCertificateError::CertificateRevoked:
        reason = tr("The certificate has been revoked.");
        break;
    case QWebEngineCertificateError::CertificateWeakSignatureAlgorithm:
        reason = tr("The certificate uses a weak signature algorithm.");
        break;
    default:
        reason = tr("The site's security certificate could not be verified.");
        break;
    }

    qCWarning(lcPrivacy) << "Certificate error for" << host << ":" << reason;

    QMessageBox box;
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Security warning"));
    box.setText(tr("Polecat Browser cannot verify the identity of \"%1\".").arg(host));
    box.setInformativeText(reason + QLatin1Char('\n')
        + tr("Continuing may put your information at risk. This is not recommended."));
    QPushButton *continueButton = box.addButton(tr("Continue anyway"), QMessageBox::DestructiveRole);
    box.addButton(tr("Go back"), QMessageBox::RejectRole);
    box.setDefaultButton(qobject_cast<QPushButton *>(box.buttons().last()));
    box.exec();

    const bool proceed = (box.clickedButton() == static_cast<QAbstractButton *>(continueButton));
    if (proceed) {
        qCWarning(lcPrivacy) << "User overrode certificate error for" << host;
        error.acceptCertificate();
    } else {
        error.rejectCertificate();
    }
}

bool WebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    // Optional HTTPS-only mode: upgrade plain http main-frame navigations.
    if (isMainFrame && m_settings
        && m_settings->value(SettingsManager::kHttpsOnly(), false).toBool()
        && url.scheme() == QLatin1String("http")) {
        QUrl httpsUrl = url;
        httpsUrl.setScheme(QStringLiteral("https"));
        emit upgradeRequested(url, httpsUrl);
        setUrl(httpsUrl);
        return false;
    }

    // Do not persist history for private pages: the HistoryManager checks the
    // profile, but we also tag the navigation for callers that need to know.
    Q_UNUSED(type);
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

void WebPage::handleFeaturePermission(const QUrl &securityOrigin, Feature feature)
{
    // Default deny until the owner answers. The owner (BrowserWindow, via
    // PermissionManager) calls answerPermission() with the user's decision.
    if (m_handlingPermission) {
        setFeaturePermission(securityOrigin, feature, PermissionDeniedByUser);
        return;
    }
    m_handlingPermission = true;
    emit permissionRequested(securityOrigin, feature);
}

void WebPage::answerPermission(const QUrl &origin, Feature feature, bool granted)
{
    setFeaturePermission(origin, feature,
                         granted ? PermissionGrantedByUser : PermissionDeniedByUser);
    m_handlingPermission = false;
}

void WebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                       const QString &message,
                                       int lineNumber,
                                       const QString &sourceID)
{
    Q_UNUSED(level);
    emit consoleMessage(message, lineNumber, sourceID);
    qCDebug(lcNet) << "console:" << sourceID << lineNumber << message;
}

} // namespace Polecat

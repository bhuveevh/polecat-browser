#include "app/Application.h"

#include "app/AppContext.h"
#include "browser/BrowserWindow.h"
#include "bookmarks/BookmarkManager.h"
#include "history/HistoryManager.h"
#include "downloads/DownloadsManager.h"
#include "privacy/PrivacyManager.h"
#include "privacy/PermissionManager.h"
#include "privacy/ContentBlocker.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsManager.h"
#include "settings/SettingsDialog.h"
#include "ui/AboutDialog.h"
#include "ui/ThemeManager.h"
#include "database/DatabaseManager.h"
#include "utils/Logging.h"
#include "utils/Paths.h"
#include "utils/UrlUtils.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QTimer>
#include <utility>

namespace Polecat {

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setApplicationName(QStringLiteral("Polecat Browser"));
    setApplicationDisplayName(QStringLiteral("Polecat Browser"));
    setOrganizationName(QStringLiteral("OpentiaOS"));
    setOrganizationDomain(QStringLiteral("opentiaos"));
    setApplicationVersion(QStringLiteral(POLECAT_VERSION));
    setDesktopFileName(QStringLiteral("org.opentiaos.PolecatBrowser"));

    Logging::install();
    parseArguments();
}

Application::~Application() = default;

void Application::parseArguments()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Polecat Browser — a modern, privacy-focused web browser for Debian Linux."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption privateOpt(QStringList{QStringLiteral("p"), QStringLiteral("private")},
                                  QStringLiteral("Start in a private window."));
    QCommandLineOption newWindowOpt(QStringList{QStringLiteral("n"), QStringLiteral("new-window")},
                                    QStringLiteral("Open links in a new window."));
    QCommandLineOption profileOpt(QStringLiteral("profile"),
                                  QStringLiteral("Use the named profile."),
                                  QStringLiteral("name"));
    QCommandLineOption verboseOpt(QStringLiteral("verbose"),
                                  QStringLiteral("Enable verbose logging."));
    parser.addOption(privateOpt);
    parser.addOption(newWindowOpt);
    parser.addOption(profileOpt);
    parser.addOption(verboseOpt);
    parser.addPositionalArgument(QStringLiteral("urls"),
                                 QStringLiteral("URLs to open."), QStringLiteral("[urls...]"));

    parser.process(*this);

    m_forcePrivate = parser.isSet(privateOpt);
    m_forceNewWindow = parser.isSet(newWindowOpt);
    if (parser.isSet(profileOpt))
        m_profileName = parser.value(profileOpt);
    if (parser.isSet(verboseOpt)) {
        Logging::setVerbose(true);
        QLoggingCategory::setFilterRules(QStringLiteral("polecat.*=true"));
    }

    for (const QString &arg : parser.positionalArguments()) {
        const QUrl url = UrlUtils::toUrl(arg, QStringLiteral("https://duckduckgo.com/?q=%1"));
        if (url.isValid())
            m_startupUrls.append(url);
    }
}

void Application::setupServices()
{
    Paths::ensureDir(Paths::configDir());
    Paths::ensureDir(Paths::dataDir());
    Paths::ensureDir(Paths::cacheDir());

    auto *settings = new SettingsManager(this);

    auto *db = new DatabaseManager(this);
    if (!db->open()) {
        QMessageBox::critical(nullptr, tr("Polecat Browser"),
            tr("Polecat Browser could not open its database. Bookmarks, history "
               "and downloads will not be saved this session.\n\n%1").arg(db->lastError()));
    }

    auto *profiles = new ProfileManager(settings, this);
    auto *blocker = new ContentBlocker(settings, this);
    profiles->setContentBlocker(blocker);

    auto *permissions = new PermissionManager(db, this);
    auto *bookmarks = new BookmarkManager(db, this);
    auto *history = new HistoryManager(db, this);
    auto *downloads = new DownloadsManager(settings, db, this);
    auto *privacy = new PrivacyManager(settings, profiles, history, downloads, permissions, db, this);
    auto *theme = new ThemeManager(settings, this);

    m_context.settings = settings;
    m_context.profiles = profiles;
    m_context.bookmarks = bookmarks;
    m_context.history = history;
    m_context.downloads = downloads;
    m_context.privacy = privacy;
    m_context.permissions = permissions;
    m_context.blocker = blocker;
    m_context.theme = theme;
    m_context.database = db;

    // Observe downloads on both the default and private profiles.
    downloads->registerProfile(profiles->persistentProfile(QStringLiteral("default")), true);
    downloads->registerProfile(profiles->privateProfile(), false);

    theme->applyFromSettings();
    applyProxySettings();

    connect(settings, &SettingsManager::settingChanged, this, [this](const QString &key, const QVariant &) {
        if (key == SettingsManager::kTheme() || key.isEmpty())
            m_context.theme->applyFromSettings();
        if (key == SettingsManager::kProxyMode() || key == SettingsManager::kProxyUrl() || key.isEmpty())
            applyProxySettings();
        if (key == SettingsManager::kBlockAdsTrackers() || key.isEmpty()) {
            m_context.blocker->setEnabled(
                m_context.settings->value(SettingsManager::kBlockAdsTrackers(), false).toBool());
        }
        if (key == SettingsManager::kBlockThirdPartyCookies() || key.isEmpty())
            m_context.profiles->refreshPolicy();
    });

    // Apply the initial content-blocker state.
    m_context.blocker->setEnabled(
        settings->value(SettingsManager::kBlockAdsTrackers(), false).toBool());
}

void Application::applyProxySettings()
{
    const QString mode = m_context.settings->value(SettingsManager::kProxyMode(),
                                                   QStringLiteral("system")).toString();
    if (mode == QLatin1String("none")) {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    } else if (mode == QLatin1String("manual")) {
        const QString url = m_context.settings->value(SettingsManager::kProxyUrl()).toString();
        QUrl proxyUrl(url);
        if (proxyUrl.isValid() && !proxyUrl.host().isEmpty()) {
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyUrl.port(8080));
            QNetworkProxy::setApplicationProxy(proxy);
        } else {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));
        }
    } else {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));
    }
}

bool Application::start()
{
    setupServices();

    if (!m_context.isValid()) {
        qCCritical(lcApp) << "Service initialisation incomplete";
        return false;
    }

    restoreSessionIfNeeded();

    // Always ensure at least one window.
    if (m_windows.isEmpty()) {
        BrowserWindow *window = createWindow(m_forcePrivate, m_profileName);
        if (!m_startupUrls.isEmpty())
            window->openUrl(m_startupUrls.first());
    } else if (!m_startupUrls.isEmpty()) {
        openExternalUrls(m_startupUrls, m_forceNewWindow);
    }

    return true;
}

BrowserWindow *Application::createWindow(bool isPrivate, const QString &profileName)
{
    auto *window = new BrowserWindow(m_context, profileName, isPrivate);

    connect(window, &BrowserWindow::newWindowRequested, this, &Application::onNewWindowRequested);
    connect(window, &BrowserWindow::windowClosed, this, &Application::onWindowClosed);
    connect(window, &BrowserWindow::openSettingsRequested, this, &Application::onOpenSettings);
    connect(window, &BrowserWindow::openAboutRequested, this, &Application::onOpenAbout);
    connect(window, &BrowserWindow::privacyNoteRequested, this, &Application::onPrivacyNote);
    connect(window, &BrowserWindow::quitRequested, this, &Application::onQuitRequested);

    m_windows.append(window);
    window->show();
    emit windowCountChanged(m_windows.size());
    return window;
}

void Application::onNewWindowRequested(bool isPrivate)
{
    createWindow(isPrivate, m_profileName);
}

void Application::onWindowClosed(BrowserWindow *window)
{
    m_windows.removeAll(QPointer<BrowserWindow>(window));
    emit windowCountChanged(m_windows.size());

    if (m_windows.isEmpty() && !m_quitting) {
        // Last window closed: on Linux it is conventional to quit, unless the
        // user asked to keep running. We quit.
        quit();
    }
}

void Application::openExternalUrls(const QList<QUrl> &urls, bool newWindow)
{
    if (urls.isEmpty())
        return;
    BrowserWindow *target = nullptr;
    if (!newWindow) {
        for (const auto &w : std::as_const(m_windows)) {
            if (w && !w->isPrivate()) {
                target = w;
                break;
            }
        }
    }
    if (!target)
        target = createWindow(false, m_profileName);

    for (int i = 0; i < urls.size(); ++i)
        target->openUrl(urls.at(i), i > 0);
    target->raise();
    target->activateWindow();
}

void Application::onOpenSettings()
{
    auto *dialog = new SettingsDialog(m_context.settings, m_context.privacy, m_context.profiles);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(dialog, &SettingsDialog::settingsChanged, this, [this]() {
        m_context.theme->applyFromSettings();
        m_context.profiles->refreshPolicy();
        for (const auto &w : std::as_const(m_windows)) {
            if (w)
                w->tabManager()->applyDefaultZoom();
        }
    });
    dialog->show();
}

void Application::onOpenAbout()
{
    auto *dialog = new AboutDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->show();
}

void Application::onPrivacyNote()
{
    QMessageBox::information(nullptr, tr("Private browsing"),
        tr("Private windows do not save your history, cookies, cache or form "
           "data on this device.\n\n"
           "Private browsing does not make you anonymous online. Your network, "
           "your employer, and the websites you visit may still be able to see "
           "your activity."));
}

void Application::onQuitRequested()
{
    m_quitting = true;
    saveSession();
    closeAllWindows();
    quit();
}

QString Application::sessionFilePath() const
{
    return Paths::dataDir() + QStringLiteral("/session.json");
}

void Application::saveSession()
{
    if (!m_context.settings->value(SettingsManager::kRestoreSession(), true).toBool())
        return;

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    QJsonArray windows;
    for (const auto &w : std::as_const(m_windows)) {
        if (!w || w->isPrivate())
            continue; // never persist private windows
        QJsonObject win = w->sessionState();
        win.insert(QStringLiteral("profile"), w->profileName());
        windows.append(win);
    }
    root.insert(QStringLiteral("windows"), windows);

    const QString path = sessionFilePath();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(lcApp) << "Cannot write session file:" << path;
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
    qCInfo(lcApp) << "Session saved to" << path;
}

void Application::restoreSessionIfNeeded()
{
    const QString startup = m_context.settings->value(SettingsManager::kStartupBehaviour()).toString();
    const bool restore = m_context.settings->value(SettingsManager::kRestoreSession(), true).toBool();

    // A URL on the command line overrides session restore.
    if (!m_startupUrls.isEmpty() || m_forcePrivate)
        return;

    if (startup != QLatin1String("restore") && !restore)
        return;

    const QString path = sessionFilePath();
    if (!QFile::exists(path))
        return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(lcApp) << "Cannot read session file:" << path;
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();
    const QJsonArray windows = root.value(QStringLiteral("windows")).toArray();
    if (windows.isEmpty())
        return;

    for (const QJsonValue &v : windows) {
        const QJsonObject win = v.toObject();
        auto *window = createWindow(false, win.value(QStringLiteral("profile")).toString(QStringLiteral("default")));
        window->restoreSession(win);
    }
    qCInfo(lcApp) << "Restored" << windows.size() << "window(s) from session";
}

} // namespace Polecat

#include "settings/SettingsManager.h"

#include "utils/Paths.h"
#include "utils/Logging.h"

#include <QDir>

namespace Polecat {

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    Paths::ensureDir(Paths::configDir());
    const QString file = Paths::configDir() + QStringLiteral("/polecat-browser.conf");
    m_settings = new QSettings(file, QSettings::IniFormat, this);
    applyDefaults();
}

SettingsManager::~SettingsManager()
{
    if (m_settings) {
        m_settings->sync();
    }
}

void SettingsManager::applyDefaults()
{
    struct Def { QString key; QVariant value; };
    const QList<Def> defaults = {
        { kStartupBehaviour(),       QStringLiteral("newtab") },
        { kHomePage(),               QStringLiteral("about:newtab") },
        { kSearchEngine(),           QStringLiteral("duckduckgo") },
        { kLanguage(),               QStringLiteral("system") },
        { kPromptDefaultBrowser(),   false },
        { kTheme(),                  QStringLiteral("system") },
        { kDensity(),                QStringLiteral("comfortable") },
        { kTabsPosition(),           QStringLiteral("top") },
        { kSidebarVisible(),         false },
        { kSidebarSection(),         QStringLiteral("bookmarks") },
        { kDefaultZoom(),            100 },
        { kHardwareAcceleration(),   true },
        { kBlockThirdPartyCookies(), true },
        { kHttpsOnly(),              false },
        { kDoNotTrack(),             false },
        { kJavascriptEnabled(),      true },
        { kImagesEnabled(),          true },
        { kPopupBlocking(),          true },
        { kBlockAdsTrackers(),       false },
        { kWebRtcPolicy(),           QStringLiteral("default") },
        { kReferrerPolicy(),         QStringLiteral("default") },
        { kAutoplayPolicy(),         QStringLiteral("userGesture") },
        { kSearchSuggestions(),      false },
        { kDownloadDir(),            Paths::downloadsDir() },
        { kAskWhereToSave(),         false },
        { kDownloadNotifications(),  true },
        { kAutoOpenSafe(),           false },
        { kRestoreSession(),         true },
        { kOpenLinksInNewTab(),      false },
        { kCloseToLastTab(),         true },
        { kShowTabBar(),             true },
        { kStudyEnabled(),           true },
        { kReaderModeEnabled(),      true },
        { kDictionaryEnabled(),      true },
        { kTranslationEnabled(),     true },
        { kStudyTimerMinutes(),      25 },
        { kDevToolsEnabled(),        true },
        { kProxyMode(),              QStringLiteral("system") },
        { kProxyUrl(),               QString() },
        { kVerboseLogging(),         false },
        { kExperimental(),           false },
        { kSessionState(),           QString() },
    };

    for (const Def &d : defaults) {
        if (!m_settings->contains(d.key))
            m_settings->setValue(d.key, d.value);
    }
}

QVariant SettingsManager::value(const QString &key, const QVariant &def) const
{
    return m_settings->value(key, def);
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    if (m_settings->value(key) == value)
        return;
    m_settings->setValue(key, value);
    m_settings->sync();
    emit settingChanged(key, value);
}

void SettingsManager::reset()
{
    m_settings->clear();
    applyDefaults();
    m_settings->sync();
    emit settingChanged(QString(), QVariant());
}

void SettingsManager::sync()
{
    m_settings->sync();
}

QString SettingsManager::searchTemplate() const
{
    const QString id = value(kSearchEngine()).toString();
    // Ordered lookup of built-in engines. Unknown ids fall back to DuckDuckGo.
    if (id == QLatin1String("google"))
        return QStringLiteral("https://www.google.com/search?q=%1");
    if (id == QLatin1String("bing"))
        return QStringLiteral("https://www.bing.com/search?q=%1");
    if (id == QLatin1String("wikipedia"))
        return QStringLiteral("https://en.wikipedia.org/w/index.php?search=%1");
    if (id == QLatin1String("ddg-lite"))
        return QStringLiteral("https://lite.duckduckgo.com/lite/?q=%1");
    return QStringLiteral("https://duckduckgo.com/?q=%1");
}

QString SettingsManager::downloadDir() const
{
    QString dir = value(kDownloadDir()).toString();
    if (dir.isEmpty() || !Paths::isWritableDir(dir)) {
        dir = Paths::downloadsDir();
        Paths::ensureDir(dir);
    }
    return dir;
}

} // namespace Polecat

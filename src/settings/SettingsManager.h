#pragma once

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QString>

namespace Polecat {

// SettingsManager centralises every persisted user preference in a QSettings
// INI file at ~/.config/polecat-browser/polecat-browser.conf.
//
// All access goes through typed accessors or value()/setValue() with defaults,
// so a missing or corrupt key never crashes the browser — it falls back to the
// documented default.
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager() override;

    // --- raw access -----------------------------------------------------
    QVariant value(const QString &key, const QVariant &def = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    void reset();                    // restore all defaults
    void sync();

    // --- well-known keys, as string constants ---------------------------
    // General
    static QString kStartupBehaviour()  { return QStringLiteral("general/startup"); }        // newtab|home|restore
    static QString kHomePage()          { return QStringLiteral("general/homePage"); }
    static QString kSearchEngine()      { return QStringLiteral("general/searchEngine"); }
    static QString kLanguage()          { return QStringLiteral("general/language"); }
    static QString kPromptDefaultBrowser() { return QStringLiteral("general/promptDefaultBrowser"); }
    // Appearance
    static QString kTheme()             { return QStringLiteral("appearance/theme"); }       // system|light|dark
    static QString kDensity()           { return QStringLiteral("appearance/density"); }     // compact|comfortable
    static QString kTabsPosition()      { return QStringLiteral("appearance/tabsPosition"); } // top|bottom
    static QString kSidebarVisible()    { return QStringLiteral("appearance/sidebarVisible"); }
    static QString kSidebarSection()    { return QStringLiteral("appearance/sidebarSection"); }
    static QString kDefaultZoom()       { return QStringLiteral("appearance/defaultZoom"); }  // percent
    static QString kHardwareAcceleration() { return QStringLiteral("appearance/hardwareAcceleration"); }
    // Use the desktop's own title bar instead of the built-in (frameless) one.
    static QString kNativeTitleBar()    { return QStringLiteral("appearance/nativeTitleBar"); }
    // Privacy
    static QString kBlockThirdPartyCookies() { return QStringLiteral("privacy/blockThirdPartyCookies"); }
    static QString kHttpsOnly()         { return QStringLiteral("privacy/httpsOnly"); }
    static QString kDoNotTrack()        { return QStringLiteral("privacy/doNotTrack"); }
    static QString kJavascriptEnabled() { return QStringLiteral("privacy/javascriptEnabled"); }
    static QString kImagesEnabled()     { return QStringLiteral("privacy/imagesEnabled"); }
    static QString kPopupBlocking()     { return QStringLiteral("privacy/popupBlocking"); }
    static QString kBlockAdsTrackers()  { return QStringLiteral("privacy/blockAdsTrackers"); }
    static QString kWebRtcPolicy()      { return QStringLiteral("privacy/webrtcPolicy"); }
    static QString kReferrerPolicy()    { return QStringLiteral("privacy/referrerPolicy"); }
    static QString kAutoplayPolicy()    { return QStringLiteral("privacy/autoplayPolicy"); }
    // Search
    static QString kSearchSuggestions() { return QStringLiteral("search/suggestions"); }
    // Downloads
    static QString kDownloadDir()       { return QStringLiteral("downloads/defaultDir"); }
    static QString kAskWhereToSave()    { return QStringLiteral("downloads/askWhereToSave"); }
    static QString kDownloadNotifications() { return QStringLiteral("downloads/notify"); }
    static QString kAutoOpenSafe()      { return QStringLiteral("downloads/autoOpenSafe"); }
    // Tabs
    static QString kRestoreSession()    { return QStringLiteral("tabs/restoreSession"); }
    static QString kOpenLinksInNewTab() { return QStringLiteral("tabs/openLinksInNewTab"); }
    static QString kCloseToLastTab()    { return QStringLiteral("tabs/closeToLastTab"); }
    static QString kShowTabBar()        { return QStringLiteral("tabs/showTabBar"); }
    // Study
    static QString kStudyEnabled()      { return QStringLiteral("study/enabled"); }
    static QString kReaderModeEnabled() { return QStringLiteral("study/readerMode"); }
    static QString kDictionaryEnabled() { return QStringLiteral("study/dictionary"); }
    static QString kTranslationEnabled(){ return QStringLiteral("study/translation"); }
    static QString kStudyTimerMinutes() { return QStringLiteral("study/timerMinutes"); }
    // Advanced
    static QString kDevToolsEnabled()   { return QStringLiteral("advanced/devTools"); }
    static QString kProxyMode()         { return QStringLiteral("advanced/proxyMode"); }      // none|system|manual
    static QString kProxyUrl()          { return QStringLiteral("advanced/proxyUrl"); }
    static QString kVerboseLogging()    { return QStringLiteral("advanced/verboseLogging"); }
    static QString kExperimental()      { return QStringLiteral("advanced/experimental"); }
    // Session (opaque JSON blob, written on exit)
    static QString kSessionState()      { return QStringLiteral("session/state"); }

    // Convenience typed getters used across the app.
    QString searchTemplate() const;
    QString downloadDir() const;

signals:
    void settingChanged(const QString &key, const QVariant &value);

private:
    void applyDefaults();

    QSettings *m_settings = nullptr;
};

} // namespace Polecat

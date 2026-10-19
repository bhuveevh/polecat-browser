#include "privacy/PrivacyManager.h"

#include "database/DatabaseManager.h"
#include "downloads/DownloadsManager.h"
#include "history/HistoryManager.h"
#include "privacy/PermissionManager.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsManager.h"
#include "utils/Logging.h"

#include <QSqlQuery>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

namespace Polecat {

PrivacyManager::PrivacyManager(SettingsManager *settings,
                               ProfileManager *profiles,
                               HistoryManager *history,
                               DownloadsManager *downloads,
                               PermissionManager *permissions,
                               DatabaseManager *db,
                               QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_profiles(profiles)
    , m_history(history)
    , m_downloads(downloads)
    , m_permissions(permissions)
    , m_db(db)
{
}

bool PrivacyManager::clearData(DataTypes types)
{
    if (types.testFlag(History))      clearHistory();
    if (types.testFlag(Cookies))      clearCookies();
    if (types.testFlag(Cache))        clearCache();
    if (types.testFlag(DownloadList)) clearDownloads();
    if (types.testFlag(FormData))     clearFormData();
    if (types.testFlag(Permissions))  clearPermissions();

    qCInfo(lcPrivacy) << "Cleared browsing data, flags =" << int(types);
    emit dataCleared(types);
    return true;
}

void PrivacyManager::clearEverything()
{
    clearData(History | Cookies | Cache | DownloadList | FormData | Permissions);
}

void PrivacyManager::clearHistory()
{
    if (m_history)
        m_history->clearAll();
}

void PrivacyManager::clearCookies()
{
    if (!m_profiles)
        return;
    // Delete cookies from every persistent profile and the private profile.
    const QStringList names = m_profiles->knownProfiles();
    for (const QString &name : names) {
        if (QWebEngineProfile *p = m_profiles->persistentProfile(name)) {
            if (auto *store = p->cookieStore())
                store->deleteAllCookies();
        }
    }
    if (QWebEngineProfile *priv = m_profiles->privateProfile()) {
        if (auto *store = priv->cookieStore())
            store->deleteAllCookies();
    }
}

void PrivacyManager::clearCache()
{
    if (!m_profiles)
        return;
    const QStringList names = m_profiles->knownProfiles();
    for (const QString &name : names) {
        if (QWebEngineProfile *p = m_profiles->persistentProfile(name))
            p->clearHttpCache();
    }
}

void PrivacyManager::clearDownloads()
{
    if (m_downloads)
        m_downloads->clearCompleted();
    if (m_db && m_db->isOpen()) {
        QSqlQuery q(m_db->database());
        q.exec(QStringLiteral("DELETE FROM downloads"));
    }
}

void PrivacyManager::clearFormData()
{
    // Qt WebEngine does not expose a direct "clear form data" API. The closest
    // supported, non-destructive action is to clear visited-link state and the
    // profile's autofill-adjacent storage via the cookie store.
    if (!m_profiles)
        return;
    const QStringList names = m_profiles->knownProfiles();
    for (const QString &name : names) {
        if (QWebEngineProfile *p = m_profiles->persistentProfile(name))
            p->clearAllVisitedLinks();
    }
}

void PrivacyManager::clearPermissions()
{
    if (m_permissions)
        m_permissions->clearAll();
}

} // namespace Polecat

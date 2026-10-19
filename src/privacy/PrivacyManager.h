#pragma once

#include <QObject>
#include <QString>

namespace Polecat {

class SettingsManager;
class ProfileManager;
class HistoryManager;
class DownloadsManager;
class PermissionManager;
class DatabaseManager;

// PrivacyManager coordinates "clear browsing data" and privacy-policy changes
// across the profile, history, downloads and permission subsystems.
class PrivacyManager : public QObject
{
    Q_OBJECT

public:
    enum DataType {
        History        = 1 << 0,
        Cookies        = 1 << 1,
        Cache          = 1 << 2,
        DownloadList   = 1 << 3,
        FormData       = 1 << 4,
        Permissions    = 1 << 5,
    };
    Q_DECLARE_FLAGS(DataTypes, DataType)

    PrivacyManager(SettingsManager *settings,
                   ProfileManager *profiles,
                   HistoryManager *history,
                   DownloadsManager *downloads,
                   PermissionManager *permissions,
                   DatabaseManager *db,
                   QObject *parent = nullptr);

    // Clears the selected data types across all profiles. Returns true if the
    // request was dispatched without error.
    bool clearData(DataTypes types);

    // Convenience: everything except passwords (which we never store).
    void clearEverything();

signals:
    void dataCleared(PrivacyManager::DataTypes types);

private:
    void clearHistory();
    void clearCookies();
    void clearCache();
    void clearDownloads();
    void clearFormData();
    void clearPermissions();

    SettingsManager *m_settings = nullptr;
    ProfileManager *m_profiles = nullptr;
    HistoryManager *m_history = nullptr;
    DownloadsManager *m_downloads = nullptr;
    PermissionManager *m_permissions = nullptr;
    DatabaseManager *m_db = nullptr;
};

} // namespace Polecat

Q_DECLARE_OPERATORS_FOR_FLAGS(Polecat::PrivacyManager::DataTypes)

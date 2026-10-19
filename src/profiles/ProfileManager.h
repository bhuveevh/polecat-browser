#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>

class QWebEngineProfile;

namespace Polecat {

class SettingsManager;
class ContentBlocker;

// ProfileManager is the single owner of every QWebEngineProfile in the process.
// Nothing else may construct a profile, which prevents the classic WebEngine
// bug of accidentally creating many profiles and multiplying cache/memory.
//
//  * Named, persistent profiles live under
//      <data>/profiles/<name>/         (cookies, storage)
//      <cache>/profiles/<name>/cache/  (HTTP cache)
//  * One shared off-the-record profile serves all private windows in a session
//    and is torn down when the last private window closes.
class ProfileManager : public QObject
{
    Q_OBJECT

public:
    explicit ProfileManager(SettingsManager *settings, QObject *parent = nullptr);
    ~ProfileManager() override;

    // Returns (creating on first use) the persistent profile called `name`.
    QWebEngineProfile *persistentProfile(const QString &name = QStringLiteral("default"));

    // Returns the shared off-the-record profile used by private windows.
    QWebEngineProfile *privateProfile();

    // Installs the request interceptor (content blocker) on all profiles.
    void setContentBlocker(ContentBlocker *blocker);

    QStringList knownProfiles() const;

    // Deletes a profile's on-disk data. Refuses to delete "default".
    bool deleteProfileData(const QString &name);

    // Re-applies cookie/cache/privacy policy after a settings change.
    void refreshPolicy();

signals:
    void profilesChanged();

private:
    QWebEngineProfile *createPersistent(const QString &name);
    void applyCommonPolicy(QWebEngineProfile *profile, bool persistent);

    SettingsManager *m_settings = nullptr;
    ContentBlocker *m_blocker = nullptr;
    QHash<QString, QWebEngineProfile *> m_persistent;
    QWebEngineProfile *m_private = nullptr;
};

} // namespace Polecat

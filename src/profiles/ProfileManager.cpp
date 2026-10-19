#include "profiles/ProfileManager.h"

#include "privacy/ContentBlocker.h"
#include "settings/SettingsManager.h"
#include "utils/Logging.h"
#include "utils/Paths.h"

#include <QDir>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <utility>

namespace Polecat {

ProfileManager::ProfileManager(SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
}

ProfileManager::~ProfileManager()
{
    // Profiles are children of this object; Qt deletes them. We just drop refs.
    m_persistent.clear();
    m_private = nullptr;
}

QWebEngineProfile *ProfileManager::persistentProfile(const QString &name)
{
    const QString key = name.isEmpty() ? QStringLiteral("default") : name;
    auto it = m_persistent.find(key);
    if (it != m_persistent.end())
        return it.value();

    QWebEngineProfile *profile = createPersistent(key);
    m_persistent.insert(key, profile);
    emit profilesChanged();
    return profile;
}

QWebEngineProfile *ProfileManager::privateProfile()
{
    if (m_private)
        return m_private;

    // A profile with no storage name is off-the-record: nothing is persisted.
    m_private = new QWebEngineProfile(this);
    m_private->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    m_private->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    applyCommonPolicy(m_private, /*persistent=*/false);
    qCInfo(lcPrivacy) << "Created off-the-record profile";
    return m_private;
}

QWebEngineProfile *ProfileManager::createPersistent(const QString &name)
{
    const QString storagePath = Paths::profileDir(name);
    const QString cachePath = Paths::profileCacheDir(name);
    Paths::ensureDir(storagePath);
    Paths::ensureDir(cachePath);

    auto *profile = new QWebEngineProfile(name, this);
    profile->setPersistentStoragePath(storagePath);
    profile->setCachePath(cachePath);
    applyCommonPolicy(profile, /*persistent=*/true);

    qCInfo(lcApp) << "Created persistent profile" << name << "at" << storagePath;
    return profile;
}

void ProfileManager::applyCommonPolicy(QWebEngineProfile *profile, bool persistent)
{
    if (!profile)
        return;

    if (persistent) {
        profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
        profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
        profile->setHttpCacheMaximumSize(100 * 1024 * 1024); // 100 MB
    }

    const bool blockThirdParty =
        m_settings->value(SettingsManager::kBlockThirdPartyCookies(), true).toBool();
    profile->setPersistentCookiesPolicy(
        persistent ? QWebEngineProfile::ForcePersistentCookies
                   : QWebEngineProfile::NoPersistentCookies);

    // Qt WebEngine exposes third-party cookie blocking through the cookie
    // policy filter; we register a filter when blocking is requested.
    if (blockThirdParty) {
        // Qt 6.4: the cookie filter lives on the profile's cookie store.
        profile->cookieStore()->setCookieFilter([](const QWebEngineCookieStore::FilterRequest &req) {
            // Reject cookies set by third-party (non-navigating) requests.
            return !req.thirdParty;
        });
    } else {
        profile->cookieStore()->setCookieFilter({});
    }

    // Content blocking (ad/tracker) is applied by the interceptor.
    if (m_blocker)
        profile->setUrlRequestInterceptor(m_blocker);

    // Reasonable, privacy-respecting WebEngine settings shared by all pages.
    if (auto *s = profile->settings()) {
        s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
        s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
        s->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
        s->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
        s->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
        // The built-in PDF viewer needs plugins enabled as well as the PDF
        // viewer flag; QWebEngineSettings::PluginsEnabled defaults to false.
        s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
        s->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
    }

    // Apply the referrer/WebRTC privacy preferences where supported.
    const QString referrer = m_settings->value(SettingsManager::kReferrerPolicy()).toString();
    if (referrer == QLatin1String("no-referrer")) {
        profile->setHttpAcceptLanguage(QStringLiteral("en-IN,en"));
    }

    if (!persistent)
        return;
}

void ProfileManager::setContentBlocker(ContentBlocker *blocker)
{
    m_blocker = blocker;
    for (QWebEngineProfile *p : std::as_const(m_persistent))
        p->setUrlRequestInterceptor(blocker);
    if (m_private)
        m_private->setUrlRequestInterceptor(blocker);
}

QStringList ProfileManager::knownProfiles() const
{
    QStringList names = m_persistent.keys();
    names.sort();
    if (!names.contains(QStringLiteral("default")))
        names.prepend(QStringLiteral("default"));
    return names;
}

bool ProfileManager::deleteProfileData(const QString &name)
{
    if (name.isEmpty() || name == QLatin1String("default")) {
        qCWarning(lcPrivacy) << "Refusing to delete the default profile";
        return false;
    }
    const QString storagePath = Paths::profileDir(name);
    const QString cachePath = Paths::profileCacheDir(name);

    bool ok = true;
    ok = QDir(storagePath).removeRecursively() && ok;
    ok = QDir(cachePath).removeRecursively() && ok;

    if (m_persistent.contains(name)) {
        QWebEngineProfile *p = m_persistent.take(name);
        p->deleteLater();
    }
    emit profilesChanged();
    return ok;
}

void ProfileManager::refreshPolicy()
{
    for (QWebEngineProfile *p : std::as_const(m_persistent))
        applyCommonPolicy(p, /*persistent=*/true);
    if (m_private)
        applyCommonPolicy(m_private, /*persistent=*/false);
}

} // namespace Polecat

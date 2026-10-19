#include "privacy/ContentBlocker.h"

#include "settings/SettingsManager.h"
#include "utils/Logging.h"
#include "utils/Paths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include <QWebEngineUrlRequestInfo>

namespace Polecat {

ContentBlocker::ContentBlocker(SettingsManager *settings, QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_settings(settings)
{
    m_enabled = m_settings->value(SettingsManager::kBlockAdsTrackers(), false).toBool();
    reloadFilters();
}

void ContentBlocker::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

int ContentBlocker::reloadFilters()
{
    m_blockRules.clear();
    m_plainRules.clear();
    m_allowRules.clear();
    m_lastError.clear();

    QStringList files;
    // Bundled lists.
    const QDir bundled(Paths::resourcePath(QStringLiteral("filters")));
    for (const QString &entry : bundled.entryList({QStringLiteral("*.txt")}, QDir::Files))
        files << bundled.absoluteFilePath(entry);

    // User list.
    const QString userList = Paths::configDir() + QStringLiteral("/filters/user.txt");
    if (QFileInfo::exists(userList))
        files << userList;

    int loadedFiles = 0;
    for (const QString &path : files) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(lcPrivacy) << "Cannot open filter list" << path;
            continue;
        }
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('!')))
                continue;

            if (line.startsWith(QStringLiteral("@@"))) {
                QString rule = line.mid(2);
                rule.remove(QStringLiteral("||"));
                rule.remove(QLatin1Char('^'));
                if (!rule.isEmpty())
                    m_allowRules.insert(rule.toLower());
                continue;
            }

            if (line.startsWith(QStringLiteral("||"))) {
                QString domain = line.mid(2);
                domain.remove(QLatin1Char('^'));
                domain.remove(QLatin1Char('*'));
                domain = domain.section(QLatin1Char('/'), 0, 0).toLower();
                if (!domain.isEmpty())
                    m_blockRules.append(domain);
                continue;
            }

            // Plain substring rule; ignore anything with filter-only syntax we
            // do not support, to avoid false positives.
            if (line.contains(QLatin1Char('$')) || line.contains(QLatin1Char('#')))
                continue;
            if (line.length() >= 4)
                m_plainRules.append(line.toLower());
        }
        ++loadedFiles;
    }

    m_sourceInfo = QStringLiteral("Bundled lists: %1 file(s). User list: %2. Updated: %3")
                       .arg(loadedFiles - (QFileInfo::exists(userList) ? 1 : 0))
                       .arg(QFileInfo::exists(userList) ? QStringLiteral("yes") : QStringLiteral("no"),
                            QDateTime::currentDateTime().toString(Qt::ISODate));

    const int total = m_blockRules.size() + m_plainRules.size();
    qCInfo(lcPrivacy) << "Loaded" << total << "filter rules from" << loadedFiles << "file(s)";
    emit filtersReloaded(total);
    return total;
}

void ContentBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    // Do Not Track is applied through this interceptor because Qt WebEngine
    // offers no direct DNT switch. This runs even when ad blocking is off.
    if (m_settings->value(SettingsManager::kDoNotTrack(), false).toBool())
        info.setHttpHeader(QByteArrayLiteral("DNT"), QByteArrayLiteral("1"));

    if (!m_enabled)
        return;

    const QUrl url = info.requestUrl();
    const QString urlString = url.toString().toLower();
    const QString host = url.host().toLower();

    // Never block the main frame's own top-level document from a user action;
    // only block subresources. This keeps ordinary browsing working.
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame)
        return;

    if (isAllowed(urlString, host))
        return;

    if (matchesBlockRule(urlString) || (!host.isEmpty() && m_blockRules.contains(host))) {
        info.block(true);
        ++m_blockedCount;
        emit blockedCountChanged(m_blockedCount);
    }
}

bool ContentBlocker::isAllowed(const QString &urlString, const QString &host) const
{
    Q_UNUSED(urlString);
    if (m_allowHosts.contains(host))
        return true;
    for (const QString &allowed : m_allowRules) {
        if (host == allowed || host.endsWith(QLatin1Char('.') + allowed))
            return true;
    }
    return false;
}

bool ContentBlocker::matchesBlockRule(const QString &urlString) const
{
    for (const QString &rule : m_plainRules) {
        if (urlString.contains(rule))
            return true;
    }
    // Domain-suffix matching for "||domain" rules.
    const QUrl url(urlString);
    const QString host = url.host().toLower();
    for (const QString &domain : m_blockRules) {
        if (host == domain || host.endsWith(QLatin1Char('.') + domain))
            return true;
    }
    return false;
}

void ContentBlocker::allowHost(const QString &host)
{
    if (!host.isEmpty())
        m_allowHosts.insert(host.toLower());
}

void ContentBlocker::removeAllowedHost(const QString &host)
{
    m_allowHosts.remove(host.toLower());
}

void ContentBlocker::resetBlockedCount()
{
    m_blockedCount = 0;
    emit blockedCountChanged(0);
}

} // namespace Polecat

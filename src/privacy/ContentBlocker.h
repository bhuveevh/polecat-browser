#pragma once

#include <QWebEngineUrlRequestInterceptor>
#include <QStringList>
#include <QSet>
#include <QString>

namespace Polecat {

class SettingsManager;

// ContentBlocker is an optional ad/tracker request interceptor.
//
// Rules are loaded from bundled filter lists (resources/filters/*.txt) plus an
// optional user list at <config>/filters/user.txt. Supported syntax is a small,
// well-defined subset of common filter lists:
//   ! comment
//   ||example.com^        domain (and subdomain) block
//   substring             plain substring match
//   @@||example.com^      exception (allowlist) rule
//
// The blocker is off by default and only active when the user enables it in
// Privacy settings. It never claims perfect blocking.
class ContentBlocker : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    explicit ContentBlocker(SettingsManager *settings, QObject *parent = nullptr);

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // (Re)loads bundled and user filter lists. Returns the number of rules.
    int reloadFilters();

    int ruleCount() const { return m_blockRules.size(); }
    int blockedCount() const { return m_blockedCount; }
    void resetBlockedCount();

    // Allowlist management (host names that are never blocked).
    void allowHost(const QString &host);
    void removeAllowedHost(const QString &host);
    QStringList allowedHosts() const { return m_allowHosts.values(); }

    QString lastError() const { return m_lastError; }
    QString filtersSourceInfo() const { return m_sourceInfo; }

signals:
    void blockedCountChanged(int count);
    void filtersReloaded(int ruleCount);

private:
    bool isAllowed(const QString &urlString, const QString &host) const;
    bool matchesBlockRule(const QString &urlString) const;

    SettingsManager *m_settings = nullptr;
    bool m_enabled = false;

    QStringList m_blockRules;   // "||domain" forms stored without the leading "||"
    QStringList m_plainRules;   // substring rules
    QSet<QString> m_allowRules; // exception domains
    QSet<QString> m_allowHosts; // user allowlist

    int m_blockedCount = 0;
    QString m_lastError;
    QString m_sourceInfo;
};

} // namespace Polecat

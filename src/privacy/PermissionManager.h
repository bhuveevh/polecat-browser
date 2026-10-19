#pragma once

#include <QObject>
#include <QString>
#include <QWebEnginePage>

namespace Polecat {

class DatabaseManager;

// PermissionManager stores per-origin decisions for sensitive WebEngine
// features (camera, microphone, location, notifications, clipboard, ...).
//
// A decision is one of: Ask (default), Allow, Deny. Decisions are reversible
// and re-askable, and are stored in the `permissions` table.
class PermissionManager : public QObject
{
    Q_OBJECT

public:
    enum Decision { Ask = 0, Allow = 1, Deny = 2 };

    explicit PermissionManager(DatabaseManager *db, QObject *parent = nullptr);

    Decision decisionFor(const QString &host, QWebEnginePage::Feature feature) const;
    void setDecision(const QString &host, QWebEnginePage::Feature feature, Decision decision);
    void clearAll();
    void clearHost(const QString &host);

    // Human-readable feature names for prompts and settings.
    static QString featureName(QWebEnginePage::Feature feature);
    static QString featureKey(QWebEnginePage::Feature feature);

signals:
    void changed();

private:
    DatabaseManager *m_db = nullptr;
};

} // namespace Polecat

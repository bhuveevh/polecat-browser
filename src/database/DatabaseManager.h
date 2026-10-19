#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

namespace Polecat {

// DatabaseManager owns the single SQLite connection used by the whole
// application and applies versioned schema migrations.
//
// Robustness:
//  * Migrations run inside a transaction; a failure rolls back and leaves
//    PRAGMA user_version untouched, so a later run can retry cleanly.
//  * On open failure or a failed integrity check, the corrupt file is moved
//    aside (polecat.db.corrupt-<timestamp>) and a fresh database is created,
//    so the browser still starts instead of refusing to run.
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    // Opens (creating if needed) and migrates the database. Returns false on
    // unrecoverable failure; lastError() then holds a human-readable reason.
    bool open();
    bool isOpen() const;

    QSqlDatabase database() const { return m_db; }
    int schemaVersion() const;
    QString lastError() const { return m_lastError; }

    // Transaction helpers for managers that issue several statements.
    bool beginTransaction();
    bool commit();
    void rollback();

    // Runs "PRAGMA integrity_check" and returns true when the DB is healthy.
    bool integrityCheck();

signals:
    void databaseRecovered(const QString &backupPath);

private:
    bool migrate();
    bool applyMigration(int version, const QStringList &statements);
    void moveAsideCorruptFile();

    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastError;
    bool m_opened = false;
};

} // namespace Polecat

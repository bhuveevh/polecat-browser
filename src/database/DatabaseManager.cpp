#include "database/DatabaseManager.h"

#include "utils/Paths.h"
#include "utils/Logging.h"

#include <QDateTime>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <vector>

namespace Polecat {

namespace {

struct Migration {
    int version;
    QStringList statements;
};

// ---------------------------------------------------------------------------
// Schema migrations. Append new entries; never edit a shipped migration.
// ---------------------------------------------------------------------------
std::vector<Migration> migrations()
{
    std::vector<Migration> list;

    // v1 — initial schema.
    list.push_back({1, {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bookmarks ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " parent_id INTEGER REFERENCES bookmarks(id) ON DELETE CASCADE,"
            " title TEXT NOT NULL DEFAULT '',"
            " url TEXT,"                       // NULL => this row is a folder
            " favicon TEXT,"
            " created INTEGER NOT NULL,"
            " modified INTEGER NOT NULL,"
            " position INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_bookmarks_parent ON bookmarks(parent_id)"),

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS history ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " title TEXT NOT NULL DEFAULT '',"
            " url TEXT NOT NULL,"
            " visit_time INTEGER NOT NULL,"
            " visit_count INTEGER NOT NULL DEFAULT 1,"
            " last_visit INTEGER NOT NULL,"
            " title_norm TEXT NOT NULL DEFAULT '',"
            " url_norm TEXT NOT NULL DEFAULT '')"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_history_url_norm ON history(url_norm)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_history_last_visit ON history(last_visit)"),

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS downloads ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " file_name TEXT NOT NULL,"
            " source_url TEXT NOT NULL DEFAULT '',"
            " destination TEXT NOT NULL DEFAULT '',"
            " total_bytes INTEGER NOT NULL DEFAULT 0,"
            " received_bytes INTEGER NOT NULL DEFAULT 0,"
            " mime TEXT NOT NULL DEFAULT '',"
            " start_time INTEGER NOT NULL,"
            " end_time INTEGER NOT NULL DEFAULT 0,"
            " state INTEGER NOT NULL DEFAULT 0)"),

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS permissions ("
            " host TEXT NOT NULL,"
            " feature TEXT NOT NULL,"
            " decision INTEGER NOT NULL DEFAULT 0,"   // 0 ask,1 allow,2 deny
            " updated INTEGER NOT NULL,"
            " PRIMARY KEY(host, feature))"),

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS notes ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " url TEXT NOT NULL DEFAULT '',"
            " title TEXT NOT NULL DEFAULT '',"
            " body TEXT NOT NULL DEFAULT '',"
            " created INTEGER NOT NULL,"
            " modified INTEGER NOT NULL)"),

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS vocabulary ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " word TEXT NOT NULL,"
            " meaning TEXT NOT NULL DEFAULT '',"
            " language TEXT NOT NULL DEFAULT '',"
            " created INTEGER NOT NULL)"),

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS reading_list ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " title TEXT NOT NULL DEFAULT '',"
            " url TEXT NOT NULL,"
            " added INTEGER NOT NULL,"
            " read INTEGER NOT NULL DEFAULT 0)"),
    }});

    // v2 — reserved for future use. Kept here so the migration machinery is
    // exercised by tests and version bumps are trivial.
    list.push_back({2, {
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_notes_url ON notes(url)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_reading_list_added ON reading_list(added)"),
    }});

    return list;
}

} // namespace

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    m_connectionName = QStringLiteral("polecat-main");
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen())
        m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DatabaseManager::open()
{
    Paths::ensureDir(Paths::dataDir());

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(Paths::databaseFile());

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qCWarning(lcDb) << "Failed to open database:" << m_lastError;
        moveAsideCorruptFile();
        // Try once more with a fresh file.
        m_db.close();
        if (!m_db.open()) {
            m_lastError = m_db.lastError().text();
            qCCritical(lcDb) << "Database still unopenable:" << m_lastError;
            return false;
        }
    }

    // Pragmas: foreign keys on, WAL for better concurrency, sensible sync.
    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));

    if (!integrityCheck()) {
        qCWarning(lcDb) << "Integrity check failed; rebuilding database";
        moveAsideCorruptFile();
        m_db.close();
        if (!m_db.open()) {
            m_lastError = m_db.lastError().text();
            return false;
        }
    }

    if (!migrate()) {
        qCCritical(lcDb) << "Migration failed:" << m_lastError;
        return false;
    }

    m_opened = true;
    return true;
}

bool DatabaseManager::isOpen() const
{
    return m_opened && m_db.isOpen();
}

int DatabaseManager::schemaVersion() const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next())
        return q.value(0).toInt();
    return 0;
}

bool DatabaseManager::beginTransaction()
{
    return m_db.transaction();
}

bool DatabaseManager::commit()
{
    return m_db.commit();
}

void DatabaseManager::rollback()
{
    m_db.rollback();
}

bool DatabaseManager::integrityCheck()
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("PRAGMA integrity_check")))
        return false;
    if (q.next())
        return q.value(0).toString().compare(QLatin1String("ok"), Qt::CaseInsensitive) == 0;
    return false;
}

void DatabaseManager::moveAsideCorruptFile()
{
    const QString file = Paths::databaseFile();
    if (!QFile::exists(file))
        return;
    const QString backup = file + QStringLiteral(".corrupt-")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
    if (QFile::rename(file, backup)) {
        qCWarning(lcDb) << "Moved aside database to" << backup;
        emit databaseRecovered(backup);
    } else {
        QFile::remove(file); // best effort
    }
}

bool DatabaseManager::migrate()
{
    const int current = schemaVersion();
    for (const Migration &m : migrations()) {
        if (m.version <= current)
            continue;
        if (!applyMigration(m.version, m.statements)) {
            qCCritical(lcDb) << "Migration to version" << m.version << "failed";
            return false;
        }
        qCInfo(lcDb) << "Applied migration to version" << m.version;
    }
    return true;
}

bool DatabaseManager::applyMigration(int version, const QStringList &statements)
{
    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    QSqlQuery q(m_db);
    for (const QString &stmt : statements) {
        if (!q.exec(stmt)) {
            m_lastError = q.lastError().text() + QStringLiteral(" [") + stmt.left(60) + QStringLiteral("]");
            qCWarning(lcDb) << "Statement failed:" << m_lastError;
            m_db.rollback();
            return false;
        }
    }

    // user_version cannot be parameterised; the value is an int we control.
    if (!q.exec(QStringLiteral("PRAGMA user_version = %1").arg(version))) {
        m_lastError = q.lastError().text();
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    return true;
}

} // namespace Polecat

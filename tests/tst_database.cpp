// Tests for the database open path, schema version and migration idempotency.

#include "database/DatabaseManager.h"
#include "utils/Paths.h"

#include <QtTest>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSqlQuery>

using namespace Polecat;

class TestDatabase : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QDir(Paths::dataDir()).removeRecursively();
    }

    void opensAndCreatesSchema()
    {
        DatabaseManager db;
        QVERIFY2(db.open(), qPrintable(db.lastError()));
        QVERIFY(db.isOpen());
        QVERIFY(db.schemaVersion() >= 1);
        QVERIFY(db.integrityCheck());
    }

    void createsExpectedTables()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        QSqlQuery q(db.database());
        QVERIFY(q.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='table'")));
        QStringList tables;
        while (q.next())
            tables << q.value(0).toString();
        for (const QString &expected : {QStringLiteral("bookmarks"),
                                        QStringLiteral("history"),
                                        QStringLiteral("downloads"),
                                        QStringLiteral("permissions"),
                                        QStringLiteral("notes"),
                                        QStringLiteral("vocabulary"),
                                        QStringLiteral("reading_list")}) {
            QVERIFY2(tables.contains(expected), qPrintable("missing table " + expected));
        }
    }

    void migrationsAreIdempotent()
    {
        int version = 0;
        {
            DatabaseManager db;
            QVERIFY(db.open());
            version = db.schemaVersion();
        }
        {
            DatabaseManager db;
            QVERIFY(db.open());
            // Reopening must not bump the version or fail.
            QCOMPARE(db.schemaVersion(), version);
        }
    }

    void recoversFromCorruptFile()
    {
        // Write garbage into the database file, then reopen.
        const QString file = Paths::databaseFile();
        QFile f(file);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("this is not a sqlite database");
        f.close();

        DatabaseManager db;
        const bool opened = db.open();
        QVERIFY(opened);
        QVERIFY(db.isOpen());
        // A backup of the corrupt file should have been made.
        const QDir dir(Paths::dataDir());
        const QStringList backups = dir.entryList({QStringLiteral("polecat.db.corrupt-*")}, QDir::Files);
        QVERIFY(!backups.isEmpty());
    }
};

QTEST_MAIN(TestDatabase)
#include "tst_database.moc"

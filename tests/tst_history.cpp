// Tests for history recording, merging, search and clearing — including the
// rule that internal pages and non-web schemes are never recorded.

#include "history/HistoryManager.h"
#include "database/DatabaseManager.h"
#include "utils/Paths.h"

#include <QtTest>
#include <QStandardPaths>
#include <QDir>

using namespace Polecat;

class TestHistory : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QDir(Paths::dataDir()).removeRecursively();
    }

    void recordsAndMergesVisits()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        HistoryManager hm(&db);

        const QUrl url(QStringLiteral("https://example.com/page"));
        QVERIFY(hm.recordVisit(QStringLiteral("Page One"), url));
        QVERIFY(hm.recordVisit(QStringLiteral("Page One renamed"), url));

        const QList<HistoryEntry> entries = hm.recent(10);
        int matches = 0;
        for (const HistoryEntry &e : entries) {
            if (e.url == url) {
                ++matches;
                QCOMPARE(e.visitCount, 2);
            }
        }
        QCOMPARE(matches, 1); // merged into a single row
    }

    void refusesInternalAndNonWebSchemes()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        HistoryManager hm(&db);

        QVERIFY(!hm.recordVisit(QStringLiteral("New Tab"), QUrl(QStringLiteral("about:newtab"))));
        QVERIFY(!hm.recordVisit(QStringLiteral("Local"), QUrl(QStringLiteral("file:///tmp/x.html"))));
        QVERIFY(!hm.recordVisit(QStringLiteral("Data"), QUrl(QStringLiteral("data:text/plain,hi"))));
    }

    void searchesTitleAndUrl()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        HistoryManager hm(&db);
        hm.recordVisit(QStringLiteral("Qt Project"), QUrl(QStringLiteral("https://qt.io")));
        hm.recordVisit(QStringLiteral("Debian"), QUrl(QStringLiteral("https://debian.org")));

        QCOMPARE(hm.search(QStringLiteral("qt")).size(), 1);
        QCOMPARE(hm.search(QStringLiteral("debian")).size(), 1);
    }

    void clearsRangeAndAll()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        HistoryManager hm(&db);
        hm.recordVisit(QStringLiteral("A"), QUrl(QStringLiteral("https://a.example")));
        hm.recordVisit(QStringLiteral("B"), QUrl(QStringLiteral("https://b.example")));
        QVERIFY(hm.count() >= 2);

        QVERIFY(hm.clearAll());
        QCOMPARE(hm.count(), 0);
    }

    void removeSingleEntry()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        HistoryManager hm(&db);
        hm.recordVisit(QStringLiteral("Solo"), QUrl(QStringLiteral("https://solo.example")));
        const QList<HistoryEntry> entries = hm.recent(5);
        QVERIFY(!entries.isEmpty());
        QVERIFY(hm.removeEntry(entries.first().id));
    }
};

QTEST_MAIN(TestHistory)
#include "tst_history.moc"

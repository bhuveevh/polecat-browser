// Tests for bookmark CRUD, folders, search and HTML import/export.

#include "bookmarks/BookmarkManager.h"
#include "database/DatabaseManager.h"
#include "utils/Paths.h"

#include <QtTest>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>

using namespace Polecat;

class TestBookmarks : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QDir(Paths::dataDir()).removeRecursively();
    }

    void addAndFind()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        BookmarkManager bm(&db);

        const QUrl url(QStringLiteral("https://example.com"));
        const qint64 id = bm.addBookmark(-1, QStringLiteral("Example"), url);
        QVERIFY(id > 0);
        QVERIFY(bm.isBookmarked(url));

        const Bookmark found = bm.findByUrl(url);
        QCOMPARE(found.title, QStringLiteral("Example"));
        QCOMPARE(found.id, id);
    }

    void searchFindsByTitleAndUrl()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        BookmarkManager bm(&db);
        bm.addBookmark(-1, QStringLiteral("Qt Documentation"),
                       QUrl(QStringLiteral("https://doc.qt.io")));
        bm.addBookmark(-1, QStringLiteral("Debian"),
                       QUrl(QStringLiteral("https://www.debian.org")));

        QCOMPARE(bm.search(QStringLiteral("qt")).size(), 1);
        QCOMPARE(bm.search(QStringLiteral("debian")).size(), 1);
        QCOMPARE(bm.search(QStringLiteral("nonexistent-xyz")).size(), 0);
    }

    void foldersHoldChildren()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        BookmarkManager bm(&db);

        const qint64 folder = bm.addFolder(-1, QStringLiteral("Study"));
        QVERIFY(folder > 0);
        bm.addBookmark(folder, QStringLiteral("Khan Academy"),
                       QUrl(QStringLiteral("https://khanacademy.org")));

        const QList<Bookmark> kids = bm.children(folder);
        QCOMPARE(kids.size(), 1);
        QVERIFY(kids.first().isFolder() == false);
    }

    void removeDeletesBookmark()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        BookmarkManager bm(&db);
        const QUrl url(QStringLiteral("https://remove-me.example"));
        const qint64 id = bm.addBookmark(-1, QStringLiteral("Temp"), url);
        QVERIFY(bm.removeBookmark(id));
        QVERIFY(!bm.isBookmarked(url));
    }

    void exportThenImportRoundTrips()
    {
        DatabaseManager db;
        QVERIFY(db.open());
        BookmarkManager bm(&db);
        bm.addBookmark(-1, QStringLiteral("RoundTrip"),
                       QUrl(QStringLiteral("https://roundtrip.example/page")));

        QTemporaryFile out(QDir::tempPath() + QStringLiteral("/polecat-XXXXXX.html"));
        QVERIFY(out.open());
        const QString path = out.fileName();
        out.close();
        QVERIFY(bm.exportHtml(path));

        // Remove everything, then import from the exported file.
        for (const Bookmark &b : bm.all())
            bm.removeBookmark(b.id);
        QVERIFY(bm.importHtml(path));
        QVERIFY(bm.isBookmarked(QUrl(QStringLiteral("https://roundtrip.example/page"))));
    }
};

QTEST_MAIN(TestBookmarks)
#include "tst_bookmarks.moc"

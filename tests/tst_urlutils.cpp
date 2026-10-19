// Tests for the address-bar URL/query heuristics and download-safety helpers.

#include "utils/UrlUtils.h"

#include <QtTest>
#include <QStandardPaths>

using namespace Polecat;

class TestUrlUtils : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Keep any incidental filesystem use out of the user's real config.
        QStandardPaths::setTestModeEnabled(true);
    }

    void detectsSchemes()
    {
        QVERIFY(UrlUtils::isLikelyUrl(QStringLiteral("https://example.com")));
        QVERIFY(UrlUtils::isLikelyUrl(QStringLiteral("http://example.com/x?y=1")));
        QVERIFY(UrlUtils::isLikelyUrl(QStringLiteral("about:blank")));
    }

    void detectsBareHosts()
    {
        QVERIFY(UrlUtils::isLikelyUrl(QStringLiteral("example.com")));
        QVERIFY(UrlUtils::isLikelyUrl(QStringLiteral("localhost:8080")));
        QVERIFY(UrlUtils::isLikelyUrl(QStringLiteral("127.0.0.1")));
    }

    void treatsPlainWordsAsSearch()
    {
        QVERIFY(!UrlUtils::isLikelyUrl(QStringLiteral("hello world")));
        QVERIFY(!UrlUtils::isLikelyUrl(QStringLiteral("qt webengine")));
        QVERIFY(!UrlUtils::isLikelyUrl(QString()));
    }

    void searchTemplateIsUsed()
    {
        const QUrl url = UrlUtils::toUrl(QStringLiteral("hello world"),
                                         QStringLiteral("https://duckduckgo.com/?q=%1"));
        QCOMPARE(url.host(), QStringLiteral("duckduckgo.com"));
        QVERIFY(url.query().contains(QStringLiteral("hello")));
    }

    void bareHostGetsHttps()
    {
        const QUrl url = UrlUtils::toUrl(QStringLiteral("example.com"),
                                         QStringLiteral("https://duckduckgo.com/?q=%1"));
        QCOMPARE(url.scheme(), QStringLiteral("https"));
        QCOMPARE(url.host(), QStringLiteral("example.com"));
    }

    void missingPlaceholderFallsBackToDuckDuckGo()
    {
        const QUrl url = UrlUtils::toUrl(QStringLiteral("qt tutorial"),
                                         QStringLiteral("https://example.com/search"));
        QCOMPARE(url.host(), QStringLiteral("duckduckgo.com"));
    }

    void sanitisesTraversalFilenames()
    {
        QCOMPARE(UrlUtils::sanitizeFileName(QStringLiteral("../../etc/passwd")),
                 QStringLiteral("passwd"));
        QCOMPARE(UrlUtils::sanitizeFileName(QStringLiteral("/tmp/evil.sh")),
                 QStringLiteral("evil.sh"));
        QCOMPARE(UrlUtils::sanitizeFileName(QStringLiteral("..")),
                 QStringLiteral("download"));
        QCOMPARE(UrlUtils::sanitizeFileName(QString()),
                 QStringLiteral("download"));
        // Control characters are stripped.
        QCOMPARE(UrlUtils::sanitizeFileName(QStringLiteral("a\x01b.txt")),
                 QStringLiteral("ab.txt"));
    }

    void flagsDangerousExtensions()
    {
        QVERIFY(UrlUtils::isDangerousExtension(QStringLiteral("setup.exe")));
        QVERIFY(UrlUtils::isDangerousExtension(QStringLiteral("script.sh")));
        QVERIFY(UrlUtils::isDangerousExtension(QStringLiteral("app.desktop")));
        QVERIFY(!UrlUtils::isDangerousExtension(QStringLiteral("photo.png")));
        QVERIFY(!UrlUtils::isDangerousExtension(QStringLiteral("document.pdf")));
    }

    void normalisesForSearch()
    {
        QCOMPARE(UrlUtils::normalizeForSearch(QStringLiteral("  Hello   World  ")),
                 QStringLiteral("hello world"));
    }

    void stripsQueryAndFragment()
    {
        const QUrl url(QStringLiteral("https://example.com/p?secret=1#frag"));
        QCOMPARE(UrlUtils::stripQueryAndFragment(url), QStringLiteral("https://example.com/p"));
    }
};

QTEST_MAIN(TestUrlUtils)
#include "tst_urlutils.moc"

// Tests for settings defaults, persistence and reset.

#include "settings/SettingsManager.h"

#include <QtTest>
#include <QStandardPaths>
#include <QDir>

using namespace Polecat;

class TestSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        // Start from a clean configuration directory.
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                            + QStringLiteral("/polecat-browser");
        QDir(dir).removeRecursively();
    }

    void defaultsAreSensible()
    {
        SettingsManager s;
        QCOMPARE(s.value(SettingsManager::kStartupBehaviour()).toString(), QStringLiteral("newtab"));
        QCOMPARE(s.value(SettingsManager::kSearchEngine()).toString(), QStringLiteral("duckduckgo"));
        QCOMPARE(s.value(SettingsManager::kTheme()).toString(), QStringLiteral("system"));
        QVERIFY(s.value(SettingsManager::kBlockThirdPartyCookies()).toBool());
        QVERIFY(!s.value(SettingsManager::kDoNotTrack()).toBool());
        QVERIFY(!s.value(SettingsManager::kBlockAdsTrackers()).toBool());
    }

    void persistsAcrossInstances()
    {
        {
            SettingsManager s;
            s.setValue(SettingsManager::kHomePage(), QStringLiteral("https://example.org"));
            s.setValue(SettingsManager::kDefaultZoom(), 120);
            s.sync();
        }
        SettingsManager s2;
        QCOMPARE(s2.value(SettingsManager::kHomePage()).toString(), QStringLiteral("https://example.org"));
        QCOMPARE(s2.value(SettingsManager::kDefaultZoom()).toInt(), 120);
    }

    void emitsChangeSignal()
    {
        SettingsManager s;
        QSignalSpy spy(&s, &SettingsManager::settingChanged);
        s.setValue(SettingsManager::kTheme(), QStringLiteral("dark"));
        QCOMPARE(spy.count(), 1);
        // Setting the same value again does not emit.
        s.setValue(SettingsManager::kTheme(), QStringLiteral("dark"));
        QCOMPARE(spy.count(), 1);
    }

    void resetRestoresDefaults()
    {
        SettingsManager s;
        s.setValue(SettingsManager::kTheme(), QStringLiteral("dark"));
        s.reset();
        QCOMPARE(s.value(SettingsManager::kTheme()).toString(), QStringLiteral("system"));
    }

    void searchTemplateMapsEngines()
    {
        SettingsManager s;
        s.setValue(SettingsManager::kSearchEngine(), QStringLiteral("wikipedia"));
        QVERIFY(s.searchTemplate().contains(QStringLiteral("wikipedia.org")));
        s.setValue(SettingsManager::kSearchEngine(), QStringLiteral("unknown-engine"));
        QVERIFY(s.searchTemplate().contains(QStringLiteral("duckduckgo.com")));
    }
};

QTEST_MAIN(TestSettings)
#include "tst_settings.moc"

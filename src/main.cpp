// Polecat Browser — application entry point.
//
// A modern, privacy-focused web browser for Debian Linux.
// Developer: OpentiaOS Developer <opentiaos@gmail.com>

#include "app/Application.h"
#include "utils/Logging.h"
#include "utils/Paths.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QWebEngineUrlScheme>

namespace {

// Reads a single boolean setting without constructing a QApplication, so that
// WebEngine/Chromium environment flags can be decided before the process starts.
bool readBoolSetting(const QString &key, bool fallback)
{
    const QString file = Polecat::Paths::configDir() + QStringLiteral("/polecat-browser.conf");
    if (!QFile::exists(file))
        return fallback;
    QSettings settings(file, QSettings::IniFormat);
    return settings.value(key, fallback).toBool();
}

} // namespace

int main(int argc, char *argv[])
{
    // Register Polecat's internal command scheme (used by the built-in pages,
    // e.g. the Settings page) before the WebEngine initialises.
    QWebEngineUrlScheme polecatScheme(QByteArrayLiteral("polecat"));
    polecatScheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    polecatScheme.setFlags(QWebEngineUrlScheme::LocalScheme
                           | QWebEngineUrlScheme::LocalAccessAllowed);
    QWebEngineUrlScheme::registerScheme(polecatScheme);

    // These attributes must be set before QApplication is constructed.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // Hardware acceleration can be turned off from Settings → Appearance. The
    // choice is read here, before Qt WebEngine initialises Chromium.
    if (!readBoolSetting(QStringLiteral("appearance/hardwareAcceleration"), true)) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
                qgetenv("QTWEBENGINE_CHROMIUM_FLAGS") + QByteArray(" --disable-gpu"));
    }

    // Respect the user's locale for Qt's own strings; translations live in the
    // resources and are loaded per-language when available.
    QLocale::setDefault(QLocale::system());

    Polecat::Application app(argc, argv);

    if (!app.start()) {
        qCCritical(lcApp) << "Polecat Browser failed to start";
        return 1;
    }

    return app.exec();
}

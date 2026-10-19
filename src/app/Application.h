#pragma once

#include <QApplication>
#include <QList>
#include <QPointer>
#include <QUrl>

#include "app/AppContext.h"

namespace Polecat {

class BrowserWindow;
class SettingsManager;
class ProfileManager;
class BookmarkManager;
class HistoryManager;
class DownloadsManager;
class PrivacyManager;
class PermissionManager;
class ContentBlocker;
class ThemeManager;
class DatabaseManager;

// Application owns every application-wide service and the set of open windows.
// It parses the command line, restores a saved session, and writes the session
// back on exit.
class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application() override;

    // Creates a normal or private window. Returns the new window.
    BrowserWindow *createWindow(bool isPrivate, const QString &profileName = QStringLiteral("default"));

    const AppContext &context() const { return m_context; }

    // Handles a URL passed on the command line or by another application.
    void openExternalUrls(const QList<QUrl> &urls, bool newWindow);

    bool start();

signals:
    void windowCountChanged(int count);

private slots:
    void onNewWindowRequested(bool isPrivate);
    void onWindowClosed(Polecat::BrowserWindow *window);
    void onOpenSettings();
    void onOpenAbout();
    void onPrivacyNote();
    void onQuitRequested();

private:
    void parseArguments();
    void setupServices();
    void applyProxySettings();
    void saveSession();
    void restoreSessionIfNeeded();
    QString sessionFilePath() const;

    AppContext m_context;
    QList<QPointer<BrowserWindow>> m_windows;

    // Command-line options.
    QList<QUrl> m_startupUrls;
    bool m_forcePrivate = false;
    bool m_forceNewWindow = false;
    QString m_profileName = QStringLiteral("default");

    bool m_quitting = false;
};

} // namespace Polecat

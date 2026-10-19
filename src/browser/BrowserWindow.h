#pragma once

#include <QMainWindow>
#include <QUrl>
#include <QPointer>
#include <QJsonObject>
#include <QWebEngineFullScreenRequest>
#include <QWebEnginePage>

#include "app/AppContext.h"

class QLabel;
class QMenu;
class QToolBar;

namespace Polecat {

class TabManager;
class BrowserTab;
class HeaderBar;
class NavigationToolBar;
class TabBar;
class Sidebar;
class FindBar;
class WebView;
class WebPage;

// BrowserWindow is a single top-level browser window (normal or private). It
// composes the header bar, navigation toolbar, tab bar, optional sidebar, the
// web content area and transient overlays (find bar, status strip).
class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    BrowserWindow(AppContext context,
                  const QString &profileName,
                  bool isPrivate,
                  QWidget *parent = nullptr);
    ~BrowserWindow() override;

    TabManager *tabManager() const { return m_tabs; }
    bool isPrivate() const { return m_private; }
    QString profileName() const { return m_profileName; }

    // Opens `url` in a new tab (or the current tab if none is open).
    BrowserTab *openUrl(const QUrl &url, bool background = false);

    // Session support.
    QJsonObject sessionState() const;
    void restoreSession(const QJsonObject &state);

    // Called by the Application when it wants this window to close for quit.
    void prepareToClose();

signals:
    void newWindowRequested(bool isPrivate);
    void windowClosed(Polecat::BrowserWindow *window);
    void openSettingsRequested();
    void openAboutRequested();
    void openBookmarksRequested();
    void openHistoryRequested();
    void openDownloadsRequested();
    void quitRequested();
    void privacyNoteRequested();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onCurrentTabChanged(BrowserTab *tab);
    void onTabUpdated(BrowserTab *tab);
    void onSubmitAddress(const QString &text);
    void onNewTabRequested();
    void onCloseTabRequested(int index);
    void onBackRequested();
    void onForwardRequested();
    void onReloadRequested();
    void onStopRequested();
    void onHomeRequested();
    void onBookmarkRequested();
    void onReaderRequested();
    void showMainMenu();
    void showProfileMenu();
    void showContextMenu(const QPoint &globalPos, const QPoint &viewPos);
    void onVisitRecorded(const QString &title, const QUrl &url);
    void onPermissionRequested(const QUrl &origin, QWebEnginePage::Feature feature);
    void onFullScreenRequested(QWebEngineFullScreenRequest request);
    void toggleSidebar();
    void toggleFindBar();
    void onLoadProgress(int progress);

private:
    void buildLayout();
    void buildMenus();
    void buildShortcuts();
    void wireTabManager();
    void updateWindowTitle();
    void updateAddressBar(BrowserTab *tab);
    void updateNavigationState(BrowserTab *tab);
    void applySettings();
    void openFindInPage();
    void setFullScreenInternal(bool on);
    void printPage();
    void savePageAsPdf();
    void viewSource();
    void openDevTools();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void saveCurrentPage();
    void copyCurrentUrl();
    void showPermissionPrompt(const QUrl &origin, QWebEnginePage::Feature feature);
    QUrl resolveAddress(const QString &text) const;

    AppContext m_context;
    QString m_profileName;
    bool m_private = false;

    TabManager *m_tabs = nullptr;
    NavigationToolBar *m_nav = nullptr;
    TabBar *m_tabBar = nullptr;
    Sidebar *m_sidebar = nullptr;
    FindBar *m_findBar = nullptr;
    class StudyPanel *m_studyPanel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QToolBar *m_statusBar = nullptr;

    QMenu *m_mainMenu = nullptr;
    bool m_fullScreen = false;
    bool m_readerActive = false;
};

} // namespace Polecat

#pragma once

#include <QObject>
#include <QList>
#include <QUrl>
#include <QJsonObject>
#include <QPointer>
#include <QWebEnginePage>

class QStackedWidget;
class QWebEngineProfile;

namespace Polecat {

class BrowserTab;
class SettingsManager;

// TabManager owns the tabs of a single window and the QStackedWidget that
// displays their web views. It is deliberately free of windowing concerns so
// it can be unit-tested.
//
// Responsibilities:
//  * create / close / select / reorder / duplicate / pin / mute tabs
//  * keep the recently-closed stack for "reopen closed tab"
//  * serialise and restore the session (used for normal restart and for
//    crash-safe recovery)
class TabManager : public QObject
{
    Q_OBJECT

public:
    TabManager(QWebEngineProfile *normalProfile,
               QWebEngineProfile *privateProfile,
               SettingsManager *settings,
               bool isPrivateWindow,
               QObject *parent = nullptr);
    ~TabManager() override;

    QWidget *contentWidget() const;

    bool isPrivateWindow() const { return m_privateWindow; }
    QWebEngineProfile *profileFor(bool wantPrivate) const;

    // Creates a tab. If `url` is empty the configured new-tab page is used.
    // When `background` is true the tab is added without stealing focus.
    BrowserTab *createTab(const QUrl &url = QUrl(), bool background = false, int insertIndex = -1);

    // Closes the tab (with the "close to last tab" policy). Returns true if
    // the tab was closed, false if the request was vetoed by policy.
    bool closeTab(BrowserTab *tab);
    void closeCurrentTab();
    void closeOtherTabs(BrowserTab *keep);

    void selectTab(BrowserTab *tab);
    void selectNextTab();
    void selectPreviousTab();
    void selectIndex(int index);

    BrowserTab *duplicateTab(BrowserTab *tab);
    void moveTab(int from, int to);

    BrowserTab *currentTab() const { return m_current; }
    const QList<BrowserTab *> &tabs() const { return m_tabs; }
    int indexOf(BrowserTab *tab) const;
    int count() const { return m_tabs.size(); }

    bool canReopenClosed() const { return !m_closedStack.isEmpty(); }
    BrowserTab *reopenClosedTab();

    void setTabPinned(BrowserTab *tab, bool pinned);
    void setTabMuted(BrowserTab *tab, bool muted);

    // Apply the default zoom (from settings) to every tab.
    void applyDefaultZoom();

    // Session serialisation.
    QJsonObject sessionState() const;
    void restoreSession(const QJsonObject &state);
    QUrl newTabUrl() const;

signals:
    void tabCreated(Polecat::BrowserTab *tab);
    void tabRemoved(Polecat::BrowserTab *tab);
    void currentTabChanged(Polecat::BrowserTab *tab);
    void tabUpdated(Polecat::BrowserTab *tab);
    void visitRecorded(const QString &title, const QUrl &url);
    void allTabsClosed();

private slots:
    void onTabTitleChanged();
    void onTabUrlChanged();
    void onTabFaviconChanged();
    void onTabLoadingChanged();
    void onTabLoadFinished(bool ok);
    void onTabMutedChanged();
    void onTabPinnedChanged();

private:
    void wireTab(BrowserTab *tab);
    void rememberClosed(BrowserTab *tab);
    void updateCurrentAfterRemoval(BrowserTab *removed);

    QWebEngineProfile *m_normalProfile = nullptr;
    QWebEngineProfile *m_privateProfile = nullptr;
    SettingsManager *m_settings = nullptr;
    bool m_privateWindow = false;

    QStackedWidget *m_stack = nullptr;
    QList<BrowserTab *> m_tabs;
    QPointer<BrowserTab> m_current;
    QList<QPair<QString, QUrl>> m_closedStack;  // title, url
    int m_nextId = 1;
};

} // namespace Polecat

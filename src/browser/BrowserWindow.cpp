#include "browser/BrowserWindow.h"

#include "browser/NavigationToolBar.h"
#include "browser/AddressBar.h"
#include "browser/WebView.h"
#include "browser/WebPage.h"
#include "tabs/TabManager.h"
#include "tabs/BrowserTab.h"
#include "tabs/TabBar.h"
#include "ui/HeaderBar.h"
#include "ui/TitleBar.h"
#include "ui/Sidebar.h"
#include "ui/FindBar.h"
#include "bookmarks/BookmarkManager.h"
#include "bookmarks/BookmarksPanel.h"
#include "history/HistoryManager.h"
#include "history/HistoryPanel.h"
#include "downloads/DownloadsManager.h"
#include "downloads/DownloadsPanel.h"
#include "study/StudyPanel.h"
#include "database/DatabaseManager.h"
#include "privacy/PermissionManager.h"
#include "settings/SettingsManager.h"
#include "profiles/ProfileManager.h"
#include "utils/IconLoader.h"
#include "utils/Logging.h"
#include "utils/Paths.h"
#include "utils/UrlUtils.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QClipboard>
#include <QCloseEvent>
#include <QCursor>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QPrintDialog>
#include <QPrinter>
#include <QShortcut>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QWebEngineDownloadRequest>
#include <QWindow>

namespace Polecat {

BrowserWindow::BrowserWindow(AppContext context,
                             const QString &profileName,
                             bool isPrivate,
                             QWidget *parent)
    : QMainWindow(parent)
    , m_context(context)
    , m_profileName(profileName.isEmpty() ? QStringLiteral("default") : profileName)
    , m_private(isPrivate)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowIcon(IconLoader::appIcon());

    QWebEngineProfile *normal = m_context.profiles->persistentProfile(m_profileName);
    QWebEngineProfile *priv = m_context.profiles->privateProfile();

    m_tabs = new TabManager(normal, priv, m_context.settings, m_private, this);

    // Handle downloads from this window's profile too (covers named profiles,
    // not just "default"). registerProfile() is idempotent.
    if (m_context.downloads)
        m_context.downloads->registerProfile(normal, !m_private);

    // Chrome-style client-side decoration: draw our own title bar and go
    // frameless. Moving and resizing are delegated to the compositor
    // (startSystemMove/startSystemResize), so this works on Wayland and X11.
    // Can be turned off with appearance/nativeTitleBar=true if a desktop
    // misbehaves.
    m_customTitleBar =
        !m_context.settings->value(SettingsManager::kNativeTitleBar(), false).toBool();
    if (m_customTitleBar) {
        setWindowFlag(Qt::FramelessWindowHint, true);
        qApp->installEventFilter(this);
    }

    buildLayout();
    buildMenus();
    buildShortcuts();
    wireTabManager();
    applySettings();

    // Start with a tab.
    m_tabs->createTab();
    updateWindowTitle();
}

BrowserWindow::~BrowserWindow() = default;

void BrowserWindow::buildLayout()
{
    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_nav = new NavigationToolBar(central);

    // Chrome-style profile chip: the system user's name for the default
    // profile, or the named profile otherwise.
    const QString profileLabel = (m_profileName == QLatin1String("default"))
                                     ? Paths::userDisplayName()
                                     : m_profileName;
    m_nav->setProfileName(profileLabel);
    m_nav->setPrivate(m_private);

    m_tabBar = new TabBar(central);
    m_tabBar->setTabManager(m_tabs);
    m_tabBar->setNewTabButtonVisible(true);

    if (m_customTitleBar) {
        m_titleBar = new TitleBar(central);
        m_titleBar->setTabBar(m_tabBar);
        m_titleBar->setMaximized(isMaximized());
        connect(m_titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
        connect(m_titleBar, &TitleBar::toggleMaximizeRequested, this, &BrowserWindow::toggleMaximize);
        connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);
    }

    // Content row: optional sidebar + the stacked web views.
    auto *contentRow = new QWidget(central);
    auto *contentLayout = new QHBoxLayout(contentRow);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_sidebar = new Sidebar(contentRow);
    m_sidebar->hide();

    // Sidebar panels.
    auto *bookmarksPanel = new BookmarksPanel(m_context.bookmarks, m_sidebar);
    auto *historyPanel = new HistoryPanel(m_context.history, m_sidebar);
    auto *downloadsPanel = new DownloadsPanel(m_context.downloads, m_sidebar);
    m_studyPanel = new StudyPanel(m_context.database, m_context.settings, m_sidebar);
    m_sidebar->addSection(QStringLiteral("bookmarks"), tr("Bookmarks"),
                          QStringLiteral("bookmarks"), bookmarksPanel);
    m_sidebar->addSection(QStringLiteral("history"), tr("History"),
                          QStringLiteral("history"), historyPanel);
    m_sidebar->addSection(QStringLiteral("downloads"), tr("Downloads"),
                          QStringLiteral("downloads"), downloadsPanel);
    m_sidebar->addSection(QStringLiteral("study"), tr("Study Tools"),
                          QStringLiteral("reader"), m_studyPanel);

    connect(bookmarksPanel, &BookmarksPanel::openUrl, this, [this](const QUrl &url) {
        openUrl(url);
    });
    connect(historyPanel, &HistoryPanel::openUrl, this, [this](const QUrl &url) {
        openUrl(url);
    });
    connect(m_studyPanel, &StudyPanel::openUrl, this, [this](const QUrl &url) {
        openUrl(url);
    });

    contentLayout->addWidget(m_sidebar, 0);
    contentLayout->addWidget(m_tabs->contentWidget(), 1);

    // Status strip (non-intrusive).
    m_statusLabel = new QLabel(this);
    m_statusBar = new QToolBar(this);
    m_statusBar->setObjectName(QStringLiteral("statusStrip"));
    m_statusBar->setMovable(false);
    m_statusBar->addWidget(m_statusLabel);
    m_statusBar->hide();

    // Chrome-style order: the title bar (tabs + window controls) on top, then
    // the navigation toolbar. Without the custom title bar the tab strip sits
    // there instead.
    if (m_customTitleBar)
        rootLayout->addWidget(m_titleBar);
    else
        rootLayout->addWidget(m_tabBar);
    rootLayout->addWidget(m_nav);
    rootLayout->addWidget(contentRow, 1);

    setCentralWidget(central);

    // Find bar as a transient overlay anchored under the content.
    m_findBar = new FindBar(this);
    m_findBar->hide();
    connect(m_findBar, &FindBar::closeRequested, this, [this]() {
        m_findBar->deactivate();
    });

    connect(m_nav, &NavigationToolBar::profileRequested, this, &BrowserWindow::showProfileMenu);

    connect(m_nav, &NavigationToolBar::backRequested, this, &BrowserWindow::onBackRequested);
    connect(m_nav, &NavigationToolBar::forwardRequested, this, &BrowserWindow::onForwardRequested);
    connect(m_nav, &NavigationToolBar::reloadRequested, this, &BrowserWindow::onReloadRequested);
    connect(m_nav, &NavigationToolBar::stopRequested, this, &BrowserWindow::onStopRequested);
    connect(m_nav, &NavigationToolBar::homeRequested, this, &BrowserWindow::onHomeRequested);
    connect(m_nav, &NavigationToolBar::bookmarkRequested, this, &BrowserWindow::onBookmarkRequested);
    connect(m_nav, &NavigationToolBar::readerRequested, this, &BrowserWindow::onReaderRequested);
    connect(m_nav, &NavigationToolBar::menuRequested, this, &BrowserWindow::showMainMenu);
    connect(m_nav, &NavigationToolBar::submitRequested, this, &BrowserWindow::onSubmitAddress);

    connect(m_tabBar, &TabBar::newTabRequested, this, &BrowserWindow::onNewTabRequested);
    connect(m_tabBar, &TabBar::closeTabRequested, this, &BrowserWindow::onCloseTabRequested);
    connect(m_tabBar, &TabBar::duplicateTabRequested, this, [this](int index) {
        if (auto *tab = m_tabs->tabs().value(index))
            m_tabs->duplicateTab(tab);
    });
    connect(m_tabBar, &TabBar::pinTabRequested, this, [this](int index, bool pinned) {
        if (auto *tab = m_tabs->tabs().value(index))
            m_tabs->setTabPinned(tab, pinned);
    });
    connect(m_tabBar, &TabBar::muteTabRequested, this, [this](int index, bool muted) {
        if (auto *tab = m_tabs->tabs().value(index))
            m_tabs->setTabMuted(tab, muted);
    });
    connect(m_tabBar, &TabBar::closeOthersRequested, this, [this](int index) {
        if (auto *tab = m_tabs->tabs().value(index))
            m_tabs->closeOtherTabs(tab);
    });
    connect(m_tabBar, &TabBar::moveRequested, this, [this](int from, int to) {
        m_tabs->moveTab(from, to);
    });

    connect(m_sidebar, &Sidebar::closeRequested, this, &BrowserWindow::toggleSidebar);
}

void BrowserWindow::wireTabManager()
{
    connect(m_tabs, &TabManager::currentTabChanged, this, &BrowserWindow::onCurrentTabChanged);
    connect(m_tabs, &TabManager::tabUpdated, this, &BrowserWindow::onTabUpdated);
    connect(m_tabs, &TabManager::visitRecorded, this, &BrowserWindow::onVisitRecorded);
    connect(m_tabs, &TabManager::allTabsClosed, this, [this]() { close(); });

    // Per-tab wiring: context menu, permissions, full-screen and load progress.
    connect(m_tabs, &TabManager::tabCreated, this, [this](BrowserTab *tab) {
        if (!tab)
            return;
        connect(tab->view(), &WebView::contextMenuRequested,
                this, &BrowserWindow::showContextMenu);
        connect(tab->view(), &QWebEngineView::loadProgress,
                this, &BrowserWindow::onLoadProgress);
        if (auto *page = tab->page()) {
            connect(page, &WebPage::permissionRequested,
                    this, &BrowserWindow::onPermissionRequested);
            connect(page, &QWebEnginePage::fullScreenRequested,
                    this, &BrowserWindow::onFullScreenRequested);
            connect(page, &WebPage::upgradeRequested, this, [this](const QUrl &from, const QUrl &to) {
                qCInfo(lcPrivacy) << "Upgraded to HTTPS:" << from.toString() << "->" << to.toString();
            });
        }
    });
}

void BrowserWindow::buildShortcuts()
{
    auto add = [this](const QKeySequence &seq, const std::function<void()> &fn) {
        auto *sc = new QShortcut(seq, this);
        connect(sc, &QShortcut::activated, this, fn);
    };

    add(QKeySequence(QStringLiteral("Ctrl+L")), [this]() { m_nav->addressBar()->selectAllForEditing(); });
    add(QKeySequence(QStringLiteral("Ctrl+T")), [this]() { onNewTabRequested(); });
    add(QKeySequence(QStringLiteral("Ctrl+W")), [this]() { m_tabs->closeCurrentTab(); });
    add(QKeySequence(QStringLiteral("Ctrl+Shift+T")), [this]() { m_tabs->reopenClosedTab(); });
    add(QKeySequence(QStringLiteral("Ctrl+N")), [this]() { emit newWindowRequested(false); });
    add(QKeySequence(QStringLiteral("Ctrl+Shift+P")), [this]() { emit newWindowRequested(true); });
    add(QKeySequence(QStringLiteral("Ctrl+R")), [this]() { onReloadRequested(); });
    add(QKeySequence(Qt::Key_F5), [this]() { onReloadRequested(); });
    add(QKeySequence(QStringLiteral("Ctrl+F")), [this]() { openFindInPage(); });
    add(QKeySequence(QStringLiteral("Ctrl+D")), [this]() { onBookmarkRequested(); });
    add(QKeySequence(QStringLiteral("Ctrl+H")), [this]() { toggleSidebar(); m_sidebar->setCurrentSection(QStringLiteral("history")); });
    add(QKeySequence(QStringLiteral("Ctrl+J")), [this]() { toggleSidebar(); m_sidebar->setCurrentSection(QStringLiteral("downloads")); });
    add(QKeySequence(QStringLiteral("Ctrl+Plus")), [this]() { zoomIn(); });
    add(QKeySequence(QStringLiteral("Ctrl+=")), [this]() { zoomIn(); });
    add(QKeySequence(QStringLiteral("Ctrl+Minus")), [this]() { zoomOut(); });
    add(QKeySequence(QStringLiteral("Ctrl+0")), [this]() { zoomReset(); });
    add(QKeySequence(QStringLiteral("F11")), [this]() { setFullScreenInternal(!m_fullScreen); });
    add(QKeySequence(QStringLiteral("Ctrl+Shift+I")), [this]() { openDevTools(); });
    add(QKeySequence(QStringLiteral("Ctrl+Tab")), [this]() { m_tabs->selectNextTab(); });
    add(QKeySequence(QStringLiteral("Ctrl+Shift+Tab")), [this]() { m_tabs->selectPreviousTab(); });
    add(QKeySequence(QStringLiteral("Ctrl+B")), [this]() { toggleSidebar(); });
}

void BrowserWindow::buildMenus()
{
    m_mainMenu = new QMenu(this);

    auto *newTab = m_mainMenu->addAction(IconLoader::icon(QStringLiteral("new-tab")), tr("New Tab"));
    newTab->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    connect(newTab, &QAction::triggered, this, &BrowserWindow::onNewTabRequested);

    auto *newWindow = m_mainMenu->addAction(tr("New Window"));
    newWindow->setShortcut(QKeySequence(QStringLiteral("Ctrl+N")));
    connect(newWindow, &QAction::triggered, this, [this]() { emit newWindowRequested(false); });

    auto *newPrivate = m_mainMenu->addAction(IconLoader::icon(QStringLiteral("private")), tr("New Private Window"));
    newPrivate->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    connect(newPrivate, &QAction::triggered, this, [this]() { emit newWindowRequested(true); });

    m_mainMenu->addSeparator();

    auto *bookmarks = m_mainMenu->addAction(IconLoader::icon(QStringLiteral("bookmarks")), tr("Bookmarks"));
    connect(bookmarks, &QAction::triggered, this, [this]() {
        m_sidebar->show();
        m_sidebar->setCurrentSection(QStringLiteral("bookmarks"));
        emit openBookmarksRequested();
    });

    auto *history = m_mainMenu->addAction(IconLoader::icon(QStringLiteral("history")), tr("History"));
    connect(history, &QAction::triggered, this, [this]() {
        m_sidebar->show();
        m_sidebar->setCurrentSection(QStringLiteral("history"));
        emit openHistoryRequested();
    });

    auto *downloads = m_mainMenu->addAction(IconLoader::icon(QStringLiteral("downloads")), tr("Downloads"));
    connect(downloads, &QAction::triggered, this, [this]() {
        m_sidebar->show();
        m_sidebar->setCurrentSection(QStringLiteral("downloads"));
        emit openDownloadsRequested();
    });

    m_mainMenu->addSeparator();

    auto *find = m_mainMenu->addAction(IconLoader::icon(QStringLiteral("find")), tr("Find in Page"));
    find->setShortcut(QKeySequence(QStringLiteral("Ctrl+F")));
    connect(find, &QAction::triggered, this, &BrowserWindow::openFindInPage);

    auto *zoomMenu = m_mainMenu->addMenu(tr("Zoom"));
    zoomMenu->addAction(tr("Zoom In"), this, &BrowserWindow::zoomIn);
    zoomMenu->addAction(tr("Zoom Out"), this, &BrowserWindow::zoomOut);
    zoomMenu->addAction(tr("Reset Zoom"), this, &BrowserWindow::zoomReset);

    auto *fullScreen = m_mainMenu->addAction(tr("Full Screen"));
    fullScreen->setShortcut(QKeySequence(Qt::Key_F11));
    connect(fullScreen, &QAction::triggered, this, [this]() { setFullScreenInternal(!m_fullScreen); });

    m_mainMenu->addSeparator();

    auto *print = m_mainMenu->addAction(tr("Print…"), this, &BrowserWindow::printPage);
    auto *savePdf = m_mainMenu->addAction(tr("Save as PDF…"), this, &BrowserWindow::savePageAsPdf);
    auto *savePage = m_mainMenu->addAction(tr("Save Page…"), this, &BrowserWindow::saveCurrentPage);
    auto *screenshot = m_mainMenu->addAction(tr("Take Screenshot…"), this, [this]() {
        if (auto *tab = m_tabs->currentTab()) {
            const QPixmap pix = tab->view()->grab();
            const QString path = QFileDialog::getSaveFileName(
                this, tr("Save screenshot"), QStringLiteral("polecat-screenshot.png"),
                tr("PNG image (*.png)"));
            if (!path.isEmpty())
                pix.save(path);
        }
    });

    m_mainMenu->addSeparator();

    auto *reader = m_mainMenu->addAction(tr("Reader Mode"), this, &BrowserWindow::onReaderRequested);
    reader->setCheckable(true);

    auto *readingList = m_mainMenu->addAction(tr("Add to Reading List"), this, [this]() {
        if (auto *tab = m_tabs->currentTab()) {
            m_sidebar->show();
            m_sidebar->setCurrentSection(QStringLiteral("study"));
            if (m_studyPanel)
                m_studyPanel->addReadingListItem(tab->displayTitle(), tab->url());
        }
    });
    Q_UNUSED(readingList);

    auto *viewSrc = m_mainMenu->addAction(tr("View Page Source"), this, &BrowserWindow::viewSource);
    auto *devTools = m_mainMenu->addAction(tr("Developer Tools"), this, &BrowserWindow::openDevTools);
    devTools->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+I")));

    m_mainMenu->addSeparator();

    auto *settings = m_mainMenu->addAction(IconLoader::icon(QStringLiteral("settings")), tr("Settings"));
    connect(settings, &QAction::triggered, this, &BrowserWindow::openSettingsRequested);

    auto *privacyNote = m_mainMenu->addAction(tr("Private Browsing Info"), this, [this]() {
        emit privacyNoteRequested();
    });
    Q_UNUSED(privacyNote);

    auto *about = m_mainMenu->addAction(tr("About Polecat Browser"), this, [this]() {
        emit openAboutRequested();
    });
    Q_UNUSED(about);

    m_mainMenu->addSeparator();
    auto *quit = m_mainMenu->addAction(tr("Quit"), this, [this]() { emit quitRequested(); });
    quit->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
}

void BrowserWindow::applySettings()
{
    const bool showTabBar = m_context.settings->value(SettingsManager::kShowTabBar(), true).toBool();
    m_tabBar->setVisible(showTabBar);

    const bool sidebarVisible = m_context.settings->value(SettingsManager::kSidebarVisible(), false).toBool();
    m_sidebar->setVisible(sidebarVisible);

    const QString position = m_context.settings->value(SettingsManager::kTabsPosition()).toString();
    // Tab position is fixed to top in this layout; the setting is stored for
    // forward compatibility with a bottom-tab layout.
    Q_UNUSED(position);

    m_tabs->applyDefaultZoom();
}

void BrowserWindow::onCurrentTabChanged(BrowserTab *tab)
{
    if (tab) {
        m_findBar->setTarget(tab->view());
        if (m_findBar->isVisible())
            m_findBar->deactivate();
    }
    updateWindowTitle();
    updateAddressBar(tab);
    updateNavigationState(tab);
}

void BrowserWindow::onTabUpdated(BrowserTab *tab)
{
    if (tab == m_tabs->currentTab()) {
        updateWindowTitle();
        updateAddressBar(tab);
        updateNavigationState(tab);
    }
}

void BrowserWindow::updateWindowTitle()
{
    BrowserTab *tab = m_tabs->currentTab();
    const QString title = tab ? tab->displayTitle() : tr("New Tab");
    const QString suffix = m_private ? tr(" — Private") : QString();
    setWindowTitle(QStringLiteral("%1%2 — Polecat Browser").arg(title, suffix));
}

void BrowserWindow::updateAddressBar(BrowserTab *tab)
{
    if (!tab)
        return;
    const QUrl url = tab->url();
    m_nav->addressBar()->setCurrentUrl(url);
    const bool secure = url.scheme() == QLatin1String("https");
    const bool internal = url.scheme() == QLatin1String("about");
    m_nav->addressBar()->setSecurityState(secure, internal);
    m_nav->setBookmarked(m_context.bookmarks->isBookmarked(url));

    // Reader mode is only offered for ordinary web pages.
    m_nav->setReaderAvailable(url.scheme() == QLatin1String("http")
                              || url.scheme() == QLatin1String("https"));
}

void BrowserWindow::updateNavigationState(BrowserTab *tab)
{
    if (!tab) {
        m_nav->setBackEnabled(false);
        m_nav->setForwardEnabled(false);
        m_nav->setLoading(false);
        return;
    }
    m_nav->setBackEnabled(tab->view()->history()->canGoBack());
    m_nav->setForwardEnabled(tab->view()->history()->canGoForward());
    m_nav->setLoading(tab->isLoading());
}

void BrowserWindow::onSubmitAddress(const QString &text)
{
    const QUrl url = resolveAddress(text);
    if (!url.isValid())
        return;
    openUrl(url);
}

QUrl BrowserWindow::resolveAddress(const QString &text) const
{
    return UrlUtils::toUrl(text, m_context.settings->searchTemplate());
}

BrowserTab *BrowserWindow::openUrl(const QUrl &url, bool background)
{
    BrowserTab *current = m_tabs->currentTab();
    if (current && !background) {
        current->view()->loadUrl(url);
        return current;
    }
    return m_tabs->createTab(url, background);
}

void BrowserWindow::onNewTabRequested()
{
    m_tabs->createTab();
}

void BrowserWindow::onCloseTabRequested(int index)
{
    if (auto *tab = m_tabs->tabs().value(index))
        m_tabs->closeTab(tab);
}

void BrowserWindow::onBackRequested()
{
    if (auto *tab = m_tabs->currentTab())
        tab->view()->back();
}

void BrowserWindow::onForwardRequested()
{
    if (auto *tab = m_tabs->currentTab())
        tab->view()->forward();
}

void BrowserWindow::onReloadRequested()
{
    if (auto *tab = m_tabs->currentTab())
        tab->view()->reload();
}

void BrowserWindow::onStopRequested()
{
    if (auto *tab = m_tabs->currentTab())
        tab->view()->stop();
}

void BrowserWindow::onHomeRequested()
{
    const QString home = m_context.settings->value(SettingsManager::kHomePage()).toString();
    openUrl(QUrl(home.isEmpty() ? QStringLiteral("about:newtab") : home));
}

void BrowserWindow::onBookmarkRequested()
{
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab)
        return;
    const QUrl url = tab->url();
    if (url.isEmpty() || url.scheme() == QLatin1String("about"))
        return;

    if (m_context.bookmarks->isBookmarked(url)) {
        const Bookmark b = m_context.bookmarks->findByUrl(url);
        if (b.id > 0)
            m_context.bookmarks->removeBookmark(b.id);
    } else {
        m_context.bookmarks->ensureRootFolder();
        m_context.bookmarks->addBookmark(-1, tab->displayTitle(), url);
    }
    m_nav->setBookmarked(m_context.bookmarks->isBookmarked(url));
}

void BrowserWindow::onReaderRequested()
{
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab)
        return;
    if (!m_readerActive) {
        // A deliberately simple reader mode: extract the readable text and
        // render it with comfortable typography. It is not a full article
        // extractor, and is clearly documented as such.
        const QString js = QStringLiteral(R"(
(function(){
  var text = document.body ? document.body.innerText : '';
  var title = document.title || '';
  var html = '<!DOCTYPE html><html><head><meta charset="utf-8">'
    + '<style>body{max-width:42em;margin:3em auto;padding:0 1.2em;'
    + 'font-family:Georgia,serif;font-size:19px;line-height:1.7;color:#202124;background:#fafafa}'
    + 'h1{font-size:1.6em;line-height:1.3}p{white-space:pre-wrap}</style></head><body>'
    + '<h1>' + title.replace(/</g,'&lt;') + '</h1><p>'
    + text.replace(/</g,'&lt;') + '</p></body></html>';
  document.open(); document.write(html); document.close();
})();
)");
        tab->view()->page()->runJavaScript(js);
        m_readerActive = true;
        m_nav->setReaderAvailable(true);
    } else {
        tab->view()->reload();
        m_readerActive = false;
    }
}

void BrowserWindow::showMainMenu()
{
    if (!m_mainMenu)
        return;
    m_mainMenu->exec(QCursor::pos());
}

void BrowserWindow::showProfileMenu()
{
    QMenu menu(this);
    const QStringList profiles = m_context.profiles->knownProfiles();
    for (const QString &name : profiles) {
        QAction *a = menu.addAction(name);
        a->setCheckable(true);
        a->setChecked(name == m_profileName);
        connect(a, &QAction::triggered, this, [this, name]() {
            if (name != m_profileName)
                emit newWindowRequested(false);
            Q_UNUSED(name);
        });
    }
    menu.addSeparator();
    QAction *priv = menu.addAction(IconLoader::icon(QStringLiteral("private")), tr("New Private Window"));
    connect(priv, &QAction::triggered, this, [this]() { emit newWindowRequested(true); });
    menu.exec(QCursor::pos());
}

void BrowserWindow::showContextMenu(const QPoint &globalPos, const QPoint &viewPos)
{
    Q_UNUSED(viewPos);
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab)
        return;
    WebPage *page = tab->page();
    if (!page)
        return;

    QMenu menu(this);
    QAction *back = menu.addAction(tr("Back"));
    back->setEnabled(tab->view()->history()->canGoBack());
    QAction *forward = menu.addAction(tr("Forward"));
    forward->setEnabled(tab->view()->history()->canGoForward());
    QAction *reload = menu.addAction(tr("Reload"));
    menu.addSeparator();

    QAction *copy = menu.addAction(tr("Copy"));
    copy->setEnabled(page->action(QWebEnginePage::Copy)->isEnabled());
    QAction *selectAll = menu.addAction(tr("Select All"));
    QAction *saveImage = menu.addAction(tr("Save Image…"));
    saveImage->setVisible(false);

    menu.addSeparator();
    QAction *searchText = menu.addAction(tr("Search for selected text"));
    QAction *translateText = menu.addAction(tr("Translate selected text"));
    QAction *addVocab = menu.addAction(tr("Add selected word to vocabulary"));
    QAction *addNote = menu.addAction(tr("Add to notes"));
    QAction *addReading = menu.addAction(tr("Add to reading list"));

    const QString selected = page->selectedText().trimmed();
    const bool hasSelection = !selected.isEmpty();
    searchText->setEnabled(hasSelection);
    translateText->setEnabled(hasSelection);
    addVocab->setEnabled(hasSelection && !selected.contains(QLatin1Char(' ')));

    menu.addSeparator();
    QAction *viewSrc = menu.addAction(tr("View Page Source"));
    QAction *inspect = menu.addAction(tr("Inspect Element"));

    QAction *chosen = menu.exec(globalPos);
    if (!chosen)
        return;

    if (chosen == back)        tab->view()->back();
    else if (chosen == forward) tab->view()->forward();
    else if (chosen == reload)  tab->view()->reload();
    else if (chosen == copy)    page->triggerAction(QWebEnginePage::Copy);
    else if (chosen == selectAll) page->triggerAction(QWebEnginePage::SelectAll);
    else if (chosen == searchText) {
        const QUrl url = UrlUtils::toUrl(selected, m_context.settings->searchTemplate());
        m_tabs->createTab(url);
    } else if (chosen == translateText) {
        const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(selected));
        m_tabs->createTab(QUrl(QStringLiteral("https://translate.google.com/?sl=auto&tl=en&text=%1").arg(encoded)));
    } else if (chosen == addVocab) {
        m_sidebar->show();
        m_sidebar->setCurrentSection(QStringLiteral("study"));
    } else if (chosen == addNote) {
        m_sidebar->show();
        m_sidebar->setCurrentSection(QStringLiteral("study"));
    } else if (chosen == addReading) {
        m_sidebar->show();
        m_sidebar->setCurrentSection(QStringLiteral("study"));
    } else if (chosen == viewSrc) {
        viewSource();
    } else if (chosen == inspect) {
        openDevTools();
    }
}

void BrowserWindow::onVisitRecorded(const QString &title, const QUrl &url)
{
    if (m_private)
        return; // never record private history
    m_context.history->recordVisit(title, url);
}

void BrowserWindow::onPermissionRequested(const QUrl &origin, QWebEnginePage::Feature feature)
{
    showPermissionPrompt(origin, feature);
}

void BrowserWindow::showPermissionPrompt(const QUrl &origin, QWebEnginePage::Feature feature)
{
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab || !tab->page())
        return;
    const QString host = origin.host();
    const auto existing = m_context.permissions->decisionFor(host, feature);
    bool granted = false;

    if (existing == PermissionManager::Allow) {
        granted = true;
    } else if (existing == PermissionManager::Deny) {
        granted = false;
    } else {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(tr("Permission request"));
        box.setText(tr("Allow \"%1\" to %2?").arg(host, PermissionManager::featureName(feature)));
        box.setInformativeText(tr("You can change this later in Settings → Privacy."));
        QPushButton *allow = box.addButton(tr("Allow"), QMessageBox::AcceptRole);
        QPushButton *deny = box.addButton(tr("Block"), QMessageBox::RejectRole);
        QCheckBox *remember = new QCheckBox(tr("Remember this decision"), &box);
        box.setCheckBox(remember);
        box.setDefaultButton(deny);
        box.exec();
        granted = (box.clickedButton() == static_cast<QAbstractButton *>(allow));
        if (remember->isChecked())
            m_context.permissions->setDecision(host, feature,
                granted ? PermissionManager::Allow : PermissionManager::Deny);
    }

    tab->page()->answerPermission(origin, feature, granted);
}

void BrowserWindow::onFullScreenRequested(QWebEngineFullScreenRequest request)
{
    request.accept();
    setFullScreenInternal(request.toggleOn());
}

void BrowserWindow::setFullScreenInternal(bool on)
{
    m_fullScreen = on;
    if (on) {
        m_nav->hide();
        if (m_titleBar)
            m_titleBar->hide();
        else
            m_tabBar->hide();
        showFullScreen();
    } else {
        m_nav->show();
        if (m_titleBar)
            m_titleBar->show();
        m_tabBar->setVisible(m_context.settings->value(SettingsManager::kShowTabBar(), true).toBool());
        showNormal();
    }
}

void BrowserWindow::toggleMaximize()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
}

Qt::Edges BrowserWindow::resizeEdgesAt(const QPoint &globalPos) const
{
    const int m = 6; // grab margin, in pixels
    const QRect g = frameGeometry();
    Qt::Edges edges;
    if (globalPos.x() <= g.left() + m)
        edges |= Qt::LeftEdge;
    if (globalPos.x() >= g.right() - m)
        edges |= Qt::RightEdge;
    if (globalPos.y() <= g.top() + m)
        edges |= Qt::TopEdge;
    if (globalPos.y() >= g.bottom() - m)
        edges |= Qt::BottomEdge;
    return edges;
}

bool BrowserWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Resize a frameless window by grabbing any edge. The press is intercepted
    // before the child (e.g. the web view) sees it, then handed to the
    // compositor so the native move-resize loop takes over.
    if (m_customTitleBar && !isMaximized() && !isFullScreen()) {
        auto *w = qobject_cast<QWidget *>(watched);
        if (w && w->window() == this && !qobject_cast<QAbstractButton *>(w)
            && event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                const Qt::Edges edges = resizeEdgesAt(me->globalPosition().toPoint());
                if (edges) {
                    if (QWindow *handle = windowHandle()) {
                        handle->startSystemResize(edges);
                        return true;
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void BrowserWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && m_titleBar)
        m_titleBar->setMaximized(isMaximized());
}

void BrowserWindow::toggleSidebar()
{
    const bool visible = !m_sidebar->isVisible();
    m_sidebar->setVisible(visible);
    m_context.settings->setValue(SettingsManager::kSidebarVisible(), visible);
}

void BrowserWindow::toggleFindBar()
{
    if (m_findBar->isVisible())
        m_findBar->deactivate();
    else
        openFindInPage();
}

void BrowserWindow::openFindInPage()
{
    if (auto *tab = m_tabs->currentTab())
        m_findBar->setTarget(tab->view());
    // Position the find bar as a floating strip near the bottom of the window.
    m_findBar->setGeometry(0, height() - m_findBar->sizeHint().height() - 24, width(),
                           m_findBar->sizeHint().height());
    m_findBar->raise();
    m_findBar->activate();
}

void BrowserWindow::onLoadProgress(int progress)
{
    m_nav->addressBar()->setLoadProgress(progress);
}

void BrowserWindow::printPage()
{
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab)
        return;
    // Qt 6 removed QWebEnginePage::print(QPrinter*). Export the page to a PDF
    // and open it so the user can print from their PDF viewer.
    tab->page()->printToPdf([this](const QByteArray &data) {
        const QString path = QDir::temp().filePath(QStringLiteral("polecat-print.pdf"));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Print page"),
                tr("Could not write the PDF file."));
            return;
        }
        file.write(data);
        file.close();
        QMessageBox::information(this, tr("Print page"),
            tr("The page has been exported to a PDF:\n%1\n\nOpen it to print.").arg(path));
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
}

void BrowserWindow::savePageAsPdf()
{
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab)
        return;
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save as PDF"), QStringLiteral("page.pdf"), tr("PDF document (*.pdf)"));
    if (path.isEmpty())
        return;
    tab->page()->printToPdf(path);
}

void BrowserWindow::saveCurrentPage()
{
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab)
        return;
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save page"), QStringLiteral("page.html"), tr("Web page, complete (*.html)"));
    if (path.isEmpty())
        return;
    tab->page()->save(path, QWebEngineDownloadRequest::CompleteHtmlSaveFormat);
}

void BrowserWindow::viewSource()
{
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab)
        return;
    tab->page()->toHtml([this](const QString &html) {
        BrowserTab *newTab = m_tabs->createTab(QUrl(QStringLiteral("about:source")));
        const QString escaped = html.toHtmlEscaped();
        newTab->view()->setHtml(
            QStringLiteral("<html><head><meta charset='utf-8'></head>"
                           "<body style='margin:0'><pre style=\"white-space:pre-wrap;"
                           "font-family:monospace;font-size:13px;padding:12px\">%1</pre></body></html>")
                .arg(escaped));
    });
}

void BrowserWindow::openDevTools()
{
    if (!m_context.settings->value(SettingsManager::kDevToolsEnabled(), true).toBool()) {
        QMessageBox::information(this, tr("Developer tools disabled"),
            tr("Developer tools are turned off. Enable them in Settings → Advanced."));
        return;
    }
    BrowserTab *tab = m_tabs->currentTab();
    if (!tab || !tab->page())
        return;

    auto *devView = new QWebEngineView();
    devView->setAttribute(Qt::WA_DeleteOnClose, true);
    devView->setWindowTitle(tr("Developer Tools — Polecat Browser"));
    auto *devPage = new QWebEnginePage(tab->page()->profile(), devView);
    devPage->setInspectedPage(tab->page());
    devView->setPage(devPage);
    devView->resize(1000, 640);
    devView->show();
}

void BrowserWindow::zoomIn()
{
    if (auto *tab = m_tabs->currentTab())
        tab->setZoomFactor(tab->zoomFactor() + 0.1);
}

void BrowserWindow::zoomOut()
{
    if (auto *tab = m_tabs->currentTab())
        tab->setZoomFactor(tab->zoomFactor() - 0.1);
}

void BrowserWindow::zoomReset()
{
    if (auto *tab = m_tabs->currentTab())
        tab->setZoomFactor(1.0);
}

void BrowserWindow::copyCurrentUrl()
{
    if (auto *tab = m_tabs->currentTab())
        QApplication::clipboard()->setText(tab->url().toString());
}

void BrowserWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_findBar->isVisible()) {
        m_findBar->deactivate();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void BrowserWindow::closeEvent(QCloseEvent *event)
{
    // A private window persists nothing; a normal window's session is captured
    // by the Application before quitting.
    emit windowClosed(this);
    QMainWindow::closeEvent(event);
}

void BrowserWindow::prepareToClose()
{
    // Nothing extra to flush: history is written as visits happen.
}

QJsonObject BrowserWindow::sessionState() const
{
    return m_tabs->sessionState();
}

void BrowserWindow::restoreSession(const QJsonObject &state)
{
    m_tabs->restoreSession(state);
}

} // namespace Polecat

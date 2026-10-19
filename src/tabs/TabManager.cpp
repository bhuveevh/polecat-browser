#include "tabs/TabManager.h"

#include "tabs/BrowserTab.h"
#include "browser/WebView.h"
#include "browser/WebPage.h"
#include "settings/SettingsManager.h"
#include "utils/Logging.h"
#include "utils/UrlUtils.h"

#include <QJsonArray>
#include <QStackedWidget>
#include <QWebEngineProfile>

namespace Polecat {

TabManager::TabManager(QWebEngineProfile *normalProfile,
                       QWebEngineProfile *privateProfile,
                       SettingsManager *settings,
                       bool isPrivateWindow,
                       QObject *parent)
    : QObject(parent)
    , m_normalProfile(normalProfile)
    , m_privateProfile(privateProfile)
    , m_settings(settings)
    , m_privateWindow(isPrivateWindow)
{
    m_stack = new QStackedWidget();
    m_stack->setObjectName(QStringLiteral("contentStack"));
}

TabManager::~TabManager()
{
    // The content stack is a QStackedWidget that the owning window reparents
    // into its layout, so the widget hierarchy owns it; deleting it here would
    // double-free. We only drop our list. The BrowserTab objects are children
    // of this manager and are destroyed by Qt afterwards; each one frees its
    // view only if that view has already been detached from the stack.
    m_tabs.clear();
}

QWidget *TabManager::contentWidget() const
{
    return m_stack;
}

QWebEngineProfile *TabManager::profileFor(bool wantPrivate) const
{
    return wantPrivate ? m_privateProfile : m_normalProfile;
}

QUrl TabManager::newTabUrl() const
{
    // Private windows show the private-browsing explanation instead of the
    // regular new-tab page.
    if (m_privateWindow)
        return QUrl(QStringLiteral("about:private"));

    const QString startup = m_settings->value(SettingsManager::kStartupBehaviour()).toString();
    if (startup == QLatin1String("home")) {
        const QString home = m_settings->value(SettingsManager::kHomePage()).toString();
        if (!home.isEmpty())
            return QUrl(home);
    }
    return QUrl(QStringLiteral("about:newtab"));
}

BrowserTab *TabManager::createTab(const QUrl &url, bool background, int insertIndex)
{
    QWebEngineProfile *profile = m_privateWindow ? m_privateProfile : m_normalProfile;
    if (!profile) {
        qCCritical(lcApp) << "No WebEngine profile available for new tab";
        return nullptr;
    }

    auto *tab = new BrowserTab(m_nextId++, profile, m_settings, m_privateWindow, this);

    const QUrl target = url.isEmpty() ? newTabUrl() : url;
    tab->view()->loadUrl(target);

    // Default zoom.
    const int zoom = m_settings->value(SettingsManager::kDefaultZoom(), 100).toInt();
    tab->setZoomFactor(zoom / 100.0);

    wireTab(tab);

    if (insertIndex < 0 || insertIndex > m_tabs.size())
        m_tabs.append(tab);
    else
        m_tabs.insert(insertIndex, tab);

    m_stack->addWidget(tab->view());

    emit tabCreated(tab);

    if (!background)
        selectTab(tab);

    return tab;
}

void TabManager::wireTab(BrowserTab *tab)
{
    connect(tab, &BrowserTab::titleChanged, this, &TabManager::onTabTitleChanged);
    connect(tab, &BrowserTab::urlChanged, this, &TabManager::onTabUrlChanged);
    connect(tab, &BrowserTab::faviconChanged, this, &TabManager::onTabFaviconChanged);
    connect(tab, &BrowserTab::loadingChanged, this, &TabManager::onTabLoadingChanged);
    connect(tab, &BrowserTab::mutedChanged, this, &TabManager::onTabMutedChanged);
    connect(tab, &BrowserTab::pinnedChanged, this, &TabManager::onTabPinnedChanged);
    connect(tab->view(), &QWebEngineView::loadFinished, this, &TabManager::onTabLoadFinished);

    if (auto *page = tab->page()) {
        page->setWindowFactory([this](QWebEnginePage::WebWindowType type) -> QWebEnginePage * {
            // Background popups open behind; user "open in new tab" comes to
            // the front. Returning the new page lets WebEngine retarget the
            // pending navigation into it.
            const bool background = (type == QWebEnginePage::WebBrowserBackgroundTab);
            BrowserTab *nt = createTab(QUrl(), background);
            return nt ? nt->page() : nullptr;
        });
    }
}

bool TabManager::closeTab(BrowserTab *tab)
{
    if (!tab)
        return false;

    // "Close to last tab" policy: closing the last tab closes the window, not
    // the tab, when the setting is on.
    if (m_tabs.size() == 1
        && m_settings->value(SettingsManager::kCloseToLastTab(), true).toBool()) {
        emit allTabsClosed();
        return false;
    }

    rememberClosed(tab);
    updateCurrentAfterRemoval(tab);

    m_tabs.removeAll(tab);
    m_stack->removeWidget(tab->view());
    emit tabRemoved(tab);

    // The BrowserTab destructor deletes its view (deleteLater).
    tab->deleteLater();

    if (!m_current && !m_tabs.isEmpty())
        selectTab(m_tabs.first());

    return true;
}

void TabManager::closeCurrentTab()
{
    if (m_current)
        closeTab(m_current);
}

void TabManager::closeOtherTabs(BrowserTab *keep)
{
    const QList<BrowserTab *> others = m_tabs;
    for (BrowserTab *t : others) {
        if (t != keep)
            closeTab(t);
    }
}

void TabManager::selectTab(BrowserTab *tab)
{
    if (!tab || !m_tabs.contains(tab))
        return;
    m_current = tab;
    tab->markActive();
    m_stack->setCurrentWidget(tab->view());
    emit currentTabChanged(tab);
}

void TabManager::selectNextTab()
{
    if (m_tabs.isEmpty())
        return;
    const int i = indexOf(m_current);
    selectTab(m_tabs.at((i + 1) % m_tabs.size()));
}

void TabManager::selectPreviousTab()
{
    if (m_tabs.isEmpty())
        return;
    const int i = indexOf(m_current);
    const int n = m_tabs.size();
    selectTab(m_tabs.at((i - 1 + n) % n));
}

void TabManager::selectIndex(int index)
{
    if (index >= 0 && index < m_tabs.size())
        selectTab(m_tabs.at(index));
}

BrowserTab *TabManager::duplicateTab(BrowserTab *tab)
{
    if (!tab)
        return nullptr;
    BrowserTab *copy = createTab(tab->url(), /*background=*/false, indexOf(tab) + 1);
    if (copy)
        copy->setPinned(tab->isPinned());
    return copy;
}

void TabManager::moveTab(int from, int to)
{
    if (from < 0 || from >= m_tabs.size() || to < 0 || to >= m_tabs.size() || from == to)
        return;
    m_tabs.move(from, to);
    emit tabUpdated(m_tabs.at(to));
}

void TabManager::setTabPinned(BrowserTab *tab, bool pinned)
{
    if (!tab)
        return;
    tab->setPinned(pinned);
    if (pinned) {
        // Move pinned tabs to the front, in pin order.
        const int from = indexOf(tab);
        int to = 0;
        while (to < m_tabs.size() && m_tabs.at(to)->isPinned() && m_tabs.at(to) != tab)
            ++to;
        if (from != to)
            moveTab(from, to);
    }
}

void TabManager::setTabMuted(BrowserTab *tab, bool muted)
{
    if (tab)
        tab->setMuted(muted);
}

void TabManager::applyDefaultZoom()
{
    const int zoom = m_settings->value(SettingsManager::kDefaultZoom(), 100).toInt();
    for (BrowserTab *t : m_tabs)
        t->setZoomFactor(zoom / 100.0);
}

int TabManager::indexOf(BrowserTab *tab) const
{
    return m_tabs.indexOf(tab);
}

void TabManager::rememberClosed(BrowserTab *tab)
{
    if (!tab)
        return;
    const QUrl url = tab->url();
    if (!url.isValid() || url.isEmpty())
        return;
    m_closedStack.append({tab->title(), url});
    constexpr int kMaxClosed = 25;
    while (m_closedStack.size() > kMaxClosed)
        m_closedStack.removeFirst();
}

BrowserTab *TabManager::reopenClosedTab()
{
    if (m_closedStack.isEmpty())
        return nullptr;
    const auto last = m_closedStack.takeLast();
    return createTab(last.second, /*background=*/false);
}

void TabManager::updateCurrentAfterRemoval(BrowserTab *removed)
{
    if (m_current != removed)
        return;
    const int idx = indexOf(removed);
    BrowserTab *next = nullptr;
    if (idx + 1 < m_tabs.size())
        next = m_tabs.at(idx + 1);
    else if (idx - 1 >= 0)
        next = m_tabs.at(idx - 1);
    m_current = next; // may be null; selectTab() is called by the caller path
    if (next)
        selectTab(next);
}

void TabManager::onTabTitleChanged()
{
    auto *tab = qobject_cast<BrowserTab *>(sender());
    if (tab)
        emit tabUpdated(tab);
}

void TabManager::onTabUrlChanged()
{
    auto *tab = qobject_cast<BrowserTab *>(sender());
    if (!tab)
        return;
    emit tabUpdated(tab);
}

void TabManager::onTabFaviconChanged()
{
    auto *tab = qobject_cast<BrowserTab *>(sender());
    if (tab)
        emit tabUpdated(tab);
}

void TabManager::onTabLoadingChanged()
{
    auto *tab = qobject_cast<BrowserTab *>(sender());
    if (tab)
        emit tabUpdated(tab);
}

void TabManager::onTabMutedChanged()
{
    auto *tab = qobject_cast<BrowserTab *>(sender());
    if (tab)
        emit tabUpdated(tab);
}

void TabManager::onTabPinnedChanged()
{
    auto *tab = qobject_cast<BrowserTab *>(sender());
    if (tab)
        emit tabUpdated(tab);
}

void TabManager::onTabLoadFinished(bool ok)
{
    auto *view = qobject_cast<QWebEngineView *>(sender());
    if (!view)
        return;
    BrowserTab *tab = nullptr;
    for (BrowserTab *t : m_tabs) {
        if (t->view() == view) {
            tab = t;
            break;
        }
    }
    if (!tab || !ok)
        return;

    // Record history only for non-private windows; private history is never
    // written (see PrivacyManager / HistoryManager).
    if (!m_privateWindow) {
        const QUrl url = tab->url();
        if (url.isValid() && !url.isEmpty()
            && url.scheme() != QLatin1String("about")) {
            emit visitRecorded(tab->title(), url);
        }
    }
}

QJsonObject TabManager::sessionState() const
{
    QJsonObject root;
    root.insert(QStringLiteral("private"), m_privateWindow);
    QJsonArray arr;
    for (BrowserTab *t : m_tabs) {
        QJsonObject o;
        o.insert(QStringLiteral("url"), t->url().toString());
        o.insert(QStringLiteral("title"), t->title());
        o.insert(QStringLiteral("pinned"), t->isPinned());
        o.insert(QStringLiteral("active"), t == m_current.data());
        arr.append(o);
    }
    root.insert(QStringLiteral("tabs"), arr);
    return root;
}

void TabManager::restoreSession(const QJsonObject &state)
{
    // Private windows never restore.
    if (m_privateWindow)
        return;

    const QJsonArray arr = state.value(QStringLiteral("tabs")).toArray();
    if (arr.isEmpty()) {
        createTab(newTabUrl());
        return;
    }

    BrowserTab *toActivate = nullptr;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QUrl url(o.value(QStringLiteral("url")).toString());
        BrowserTab *tab = createTab(url.isEmpty() ? newTabUrl() : url, /*background=*/true);
        if (!tab)
            continue;
        tab->setPinned(o.value(QStringLiteral("pinned")).toBool());
        if (o.value(QStringLiteral("active")).toBool())
            toActivate = tab;
    }

    if (toActivate)
        selectTab(toActivate);
    else if (!m_tabs.isEmpty())
        selectTab(m_tabs.first());
}

} // namespace Polecat

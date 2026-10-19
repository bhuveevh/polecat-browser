#include "tabs/TabBar.h"

#include "tabs/TabManager.h"
#include "tabs/BrowserTab.h"
#include "utils/IconLoader.h"

#include <QHBoxLayout>
#include <QMenu>
#include <QTabBar>
#include <QToolButton>

namespace Polecat {

TabBar::TabBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("tabBarContainer"));
    // Size to the tabs (plus the new-tab button) rather than expanding, so the
    // frameless TitleBar can use the remaining space as a drag handle.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 0);
    layout->setSpacing(2);

    m_bar = new QTabBar(this);
    m_bar->setObjectName(QStringLiteral("mainTabBar"));
    m_bar->setExpanding(false);
    m_bar->setMovable(true);
    m_bar->setTabsClosable(true);
    m_bar->setDrawBase(false);
    m_bar->setUsesScrollButtons(true);
    m_bar->setElideMode(Qt::ElideRight);
    m_bar->setContextMenuPolicy(Qt::CustomContextMenu);
    m_bar->setAccessibleName(tr("Browser tabs"));

    m_newTabButton = new QToolButton(this);
    m_newTabButton->setObjectName(QStringLiteral("newTabButton"));
    m_newTabButton->setIcon(IconLoader::icon(QStringLiteral("new-tab")));
    m_newTabButton->setToolTip(tr("New tab (Ctrl+T)"));
    m_newTabButton->setAutoRaise(true);
    m_newTabButton->setAccessibleName(tr("New tab"));

    layout->addWidget(m_bar, 1);
    layout->addWidget(m_newTabButton, 0);

    connect(m_newTabButton, &QToolButton::clicked, this, &TabBar::newTabRequested);
    connect(m_bar, &QTabBar::tabCloseRequested, this, &TabBar::onCloseClicked);
    connect(m_bar, &QTabBar::currentChanged, this, &TabBar::onCurrentChanged);
    connect(m_bar, &QTabBar::tabMoved, this, &TabBar::onTabMoved);
    connect(m_bar, &QTabBar::customContextMenuRequested, this, &TabBar::showContextMenu);
}

void TabBar::setTabManager(TabManager *manager)
{
    if (m_manager == manager)
        return;
    m_manager = manager;
    if (!m_manager)
        return;

    connect(m_manager, &TabManager::tabCreated, this, &TabBar::onTabCreated);
    connect(m_manager, &TabManager::tabRemoved, this, &TabBar::onTabRemoved);
    connect(m_manager, &TabManager::tabUpdated, this, &TabBar::onTabUpdated);
    connect(m_manager, &TabManager::currentTabChanged, this, &TabBar::onCurrentTabChanged);

    // Rebuild from any existing tabs.
    for (BrowserTab *tab : m_manager->tabs())
        onTabCreated(tab);
    onCurrentTabChanged(m_manager->currentTab());
}

void TabBar::setNewTabButtonVisible(bool visible)
{
    m_newTabButton->setVisible(visible);
}

int TabBar::indexOfTab(BrowserTab *tab) const
{
    if (!m_manager || !tab)
        return -1;
    return m_manager->indexOf(tab);
}

void TabBar::onTabCreated(BrowserTab *tab)
{
    if (!tab || !m_manager)
        return;
    m_updating = true;
    const int index = m_manager->indexOf(tab);
    m_bar->insertTab(index < 0 ? m_bar->count() : index, tab->favicon(), tab->displayTitle());
    m_bar->setTabToolTip(index, tab->displayTitle());
    m_bar->setTabData(index, tab->id());
    m_updating = false;
}

void TabBar::onTabRemoved(BrowserTab *tab)
{
    if (!tab)
        return;
    // Find by the stable tab id we stored as tab data on creation.
    for (int i = 0; i < m_bar->count(); ++i) {
        if (m_bar->tabData(i).toInt() == tab->id()) {
            m_updating = true;
            m_bar->removeTab(i);
            m_updating = false;
            return;
        }
    }
}

void TabBar::onTabUpdated(BrowserTab *tab)
{
    if (!tab || !m_manager)
        return;
    const int index = m_manager->indexOf(tab);
    if (index < 0)
        return;
    refreshTab(index);
}

void TabBar::refreshTab(int index)
{
    if (!m_manager || index < 0 || index >= m_manager->tabs().size())
        return;
    BrowserTab *tab = m_manager->tabs().at(index);
    m_updating = true;
    if (index < m_bar->count()) {
        m_bar->setTabIcon(index, tab->favicon());
        m_bar->setTabText(index, tab->displayTitle());
        m_bar->setTabToolTip(index, tab->displayTitle());
        m_bar->setTabData(index, tab->id());
        // Reflect muted state without a dedicated column.
        if (tab->isMuted())
            m_bar->setTabText(index, QStringLiteral("🔇 ") + tab->displayTitle());
    }
    m_updating = false;
}

void TabBar::onCurrentTabChanged(BrowserTab *tab)
{
    if (!tab || !m_manager)
        return;
    const int index = m_manager->indexOf(tab);
    if (index < 0 || index >= m_bar->count())
        return;
    m_updating = true;
    m_bar->setCurrentIndex(index);
    m_updating = false;
}

void TabBar::onTabMoved(int from, int to)
{
    if (m_updating)
        return;
    emit moveRequested(from, to);
}

void TabBar::onCurrentChanged(int index)
{
    if (m_updating || !m_manager)
        return;
    m_manager->selectIndex(index);
}

void TabBar::onCloseClicked(int index)
{
    emit closeTabRequested(index);
}

void TabBar::showContextMenu(const QPoint &pos)
{
    const int index = m_bar->tabAt(pos);
    if (index < 0 || !m_manager)
        return;
    BrowserTab *tab = m_manager->tabs().value(index);
    if (!tab)
        return;

    QMenu menu(this);
    QAction *reload = menu.addAction(tr("Reload"));
    QAction *dup = menu.addAction(tr("Duplicate tab"));
    menu.addSeparator();
    QAction *pin = menu.addAction(tab->isPinned() ? tr("Unpin tab") : tr("Pin tab"));
    QAction *mute = menu.addAction(tab->isMuted() ? tr("Unmute tab") : tr("Mute tab"));
    menu.addSeparator();
    QAction *closeOthers = menu.addAction(tr("Close other tabs"));
    QAction *close = menu.addAction(tr("Close tab"));

    QAction *chosen = menu.exec(m_bar->mapToGlobal(pos));
    if (!chosen)
        return;
    if (chosen == reload)
        emit reloadTabRequested(index);
    else if (chosen == dup)
        emit duplicateTabRequested(index);
    else if (chosen == pin)
        emit pinTabRequested(index, !tab->isPinned());
    else if (chosen == mute)
        emit muteTabRequested(index, !tab->isMuted());
    else if (chosen == closeOthers)
        emit closeOthersRequested(index);
    else if (chosen == close)
        emit closeTabRequested(index);
}

} // namespace Polecat

#pragma once

#include <QWidget>
#include <QIcon>

class QTabBar;
class QToolButton;

namespace Polecat {

class TabManager;
class BrowserTab;

// TabBar renders the tab strip: a QTabBar (with favicons, close buttons, drag
// reordering, pinned tabs) plus a trailing "new tab" button. It is a pure view
// over a TabManager and forwards user intent as signals.
class TabBar : public QWidget
{
    Q_OBJECT

public:
    explicit TabBar(QWidget *parent = nullptr);

    void setTabManager(TabManager *manager);
    void setNewTabButtonVisible(bool visible);

signals:
    void newTabRequested();
    void closeTabRequested(int index);
    void duplicateTabRequested(int index);
    void pinTabRequested(int index, bool pinned);
    void muteTabRequested(int index, bool muted);
    void reloadTabRequested(int index);
    void closeOthersRequested(int index);
    void moveRequested(int from, int to);

private slots:
    void onTabCreated(Polecat::BrowserTab *tab);
    void onTabRemoved(Polecat::BrowserTab *tab);
    void onTabUpdated(Polecat::BrowserTab *tab);
    void onCurrentTabChanged(Polecat::BrowserTab *tab);
    void onTabMoved(int from, int to);
    void onCurrentChanged(int index);
    void onCloseClicked(int index);
    void showContextMenu(const QPoint &pos);

private:
    int indexOfTab(Polecat::BrowserTab *tab) const;
    void refreshTab(int index);

    QTabBar *m_bar = nullptr;
    QToolButton *m_newTabButton = nullptr;
    TabManager *m_manager = nullptr;
    bool m_updating = false;
};

} // namespace Polecat

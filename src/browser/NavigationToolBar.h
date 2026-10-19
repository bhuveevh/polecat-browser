#pragma once

#include <QToolBar>
#include <QUrl>

class QAction;
class QLabel;
class QToolButton;

namespace Polecat {

class AddressBar;

// NavigationToolBar holds the Chrome-style top row: Back / Forward /
// Reload-Stop / Home on the left, the expanding address bar, then the bookmark
// star, reader toggle, an optional Private badge, the profile chip and the main
// menu button on the right.
class NavigationToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit NavigationToolBar(QWidget *parent = nullptr);

    AddressBar *addressBar() const { return m_address; }

    void setBackEnabled(bool on);
    void setForwardEnabled(bool on);
    void setLoading(bool loading);
    void setBookmarked(bool on);
    void setReaderAvailable(bool on);

    // Shows the user/profile name next to the avatar chip.
    void setProfileName(const QString &name);
    void setPrivate(bool isPrivate);

signals:
    void backRequested();
    void forwardRequested();
    void reloadRequested();
    void stopRequested();
    void homeRequested();
    void bookmarkRequested();
    void readerRequested();
    void menuRequested();
    void profileRequested();
    void submitRequested(const QString &text);

private:
    AddressBar *m_address = nullptr;
    QAction *m_back = nullptr;
    QAction *m_forward = nullptr;
    QAction *m_reload = nullptr;
    QAction *m_home = nullptr;
    QAction *m_bookmark = nullptr;
    QAction *m_reader = nullptr;
    QAction *m_menu = nullptr;
    QLabel *m_privateBadge = nullptr;
    QToolButton *m_profile = nullptr;
};

} // namespace Polecat

#include "browser/NavigationToolBar.h"

#include "browser/AddressBar.h"
#include "utils/IconLoader.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

namespace Polecat {

NavigationToolBar::NavigationToolBar(QWidget *parent)
    : QToolBar(parent)
{
    setObjectName(QStringLiteral("navigationToolBar"));
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(20, 20));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContentsMargins(4, 2, 4, 2);

    m_back = addAction(IconLoader::icon(QStringLiteral("back")), tr("Back"));
    m_back->setShortcut(QKeySequence(QStringLiteral("Alt+Left")));
    m_back->setToolTip(tr("Back (Alt+Left)"));
    connect(m_back, &QAction::triggered, this, &NavigationToolBar::backRequested);

    m_forward = addAction(IconLoader::icon(QStringLiteral("forward")), tr("Forward"));
    m_forward->setShortcut(QKeySequence(QStringLiteral("Alt+Right")));
    m_forward->setToolTip(tr("Forward (Alt+Right)"));
    connect(m_forward, &QAction::triggered, this, &NavigationToolBar::forwardRequested);

    m_reload = addAction(IconLoader::icon(QStringLiteral("reload")), tr("Reload"));
    m_reload->setShortcut(QKeySequence::Refresh);
    m_reload->setToolTip(tr("Reload (Ctrl+R)"));
    connect(m_reload, &QAction::triggered, this, [this]() {
        if (m_reload->text() == tr("Stop"))
            emit stopRequested();
        else
            emit reloadRequested();
    });

    m_home = addAction(IconLoader::icon(QStringLiteral("home")), tr("Home"));
    m_home->setToolTip(tr("Home page"));
    connect(m_home, &QAction::triggered, this, &NavigationToolBar::homeRequested);

    // The address bar expands to take the remaining width.
    m_address = new AddressBar(this);
    m_address->setObjectName(QStringLiteral("addressBar"));
    addWidget(m_address);
    connect(m_address, &AddressBar::submitRequested, this, &NavigationToolBar::submitRequested);

    m_bookmark = addAction(IconLoader::icon(QStringLiteral("bookmark")), tr("Bookmark this page"));
    m_bookmark->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    m_bookmark->setToolTip(tr("Bookmark this page (Ctrl+D)"));
    connect(m_bookmark, &QAction::triggered, this, &NavigationToolBar::bookmarkRequested);

    m_reader = addAction(IconLoader::icon(QStringLiteral("reader")), tr("Reader mode"));
    m_reader->setCheckable(true);
    m_reader->setToolTip(tr("Reader mode"));
    m_reader->setEnabled(false);
    connect(m_reader, &QAction::triggered, this, &NavigationToolBar::readerRequested);

    m_privateBadge = new QLabel(tr("Private"), this);
    m_privateBadge->setObjectName(QStringLiteral("privateBadge"));
    m_privateBadge->setToolTip(tr("Private browsing is on. Activity is not saved on this device."));
    m_privateBadge->hide();
    addWidget(m_privateBadge);

    // Profile chip (avatar + the system user's name), like Chrome's profile menu.
    m_profile = new QToolButton(this);
    m_profile->setObjectName(QStringLiteral("profileButton"));
    m_profile->setIcon(IconLoader::icon(QStringLiteral("profile")));
    m_profile->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_profile->setAutoRaise(true);
    m_profile->setCursor(Qt::PointingHandCursor);
    m_profile->setToolTip(tr("Profile"));
    m_profile->setAccessibleName(tr("Profile"));
    addWidget(m_profile);
    connect(m_profile, &QToolButton::clicked, this, &NavigationToolBar::profileRequested);

    m_menu = addAction(IconLoader::icon(QStringLiteral("menu")), tr("Main menu"));
    m_menu->setToolTip(tr("Main menu"));
    connect(m_menu, &QAction::triggered, this, &NavigationToolBar::menuRequested);

    setBackEnabled(false);
    setForwardEnabled(false);
    setBookmarked(false);
}

void NavigationToolBar::setBackEnabled(bool on)
{
    m_back->setEnabled(on);
}

void NavigationToolBar::setForwardEnabled(bool on)
{
    m_forward->setEnabled(on);
}

void NavigationToolBar::setLoading(bool loading)
{
    m_reload->setIcon(IconLoader::icon(loading ? QStringLiteral("stop") : QStringLiteral("reload")));
    m_reload->setText(loading ? tr("Stop") : tr("Reload"));
    m_reload->setToolTip(loading ? tr("Stop loading") : tr("Reload (Ctrl+R)"));
}

void NavigationToolBar::setBookmarked(bool on)
{
    m_bookmark->setText(on ? tr("Edit bookmark") : tr("Bookmark this page"));
    m_bookmark->setToolTip(on ? tr("Edit bookmark (Ctrl+D)") : tr("Bookmark this page (Ctrl+D)"));
}

void NavigationToolBar::setReaderAvailable(bool on)
{
    m_reader->setEnabled(on);
}

void NavigationToolBar::setProfileName(const QString &name)
{
    m_profile->setText(name);
    m_profile->setToolTip(name.isEmpty() ? tr("Profile") : name);
}

void NavigationToolBar::setPrivate(bool isPrivate)
{
    m_privateBadge->setVisible(isPrivate);
}

} // namespace Polecat

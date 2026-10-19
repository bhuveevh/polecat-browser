#include "browser/NavigationToolBar.h"

#include "browser/AddressBar.h"
#include "utils/IconLoader.h"

#include <QAction>
#include <QMenu>

namespace Polecat {

NavigationToolBar::NavigationToolBar(QWidget *parent)
    : QToolBar(parent)
{
    setObjectName(QStringLiteral("navigationToolBar"));
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(18, 18));
    setToolButtonStyle(Qt::ToolButtonIconOnly);

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

    m_address = new AddressBar(this);
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

} // namespace Polecat

#include "ui/HeaderBar.h"

#include "utils/IconLoader.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace Polecat {

HeaderBar::HeaderBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("headerBar"));
    setFixedHeight(38);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 2, 6, 2);
    layout->setSpacing(8);

    m_title = new QLabel(tr("Polecat Browser"), this);
    m_title->setObjectName(QStringLiteral("headerTitle"));
    m_title->setTextInteractionFlags(Qt::NoTextInteraction);
    m_title->setAccessibleName(tr("Current page title"));

    m_privateBadge = new QLabel(tr("Private"), this);
    m_privateBadge->setObjectName(QStringLiteral("privateBadge"));
    m_privateBadge->setToolTip(tr("Private browsing is on. Activity is not saved on this device."));
    m_privateBadge->hide();

    m_profileButton = new QToolButton(this);
    m_profileButton->setObjectName(QStringLiteral("profileButton"));
    m_profileButton->setText(tr("Default"));
    m_profileButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_profileButton->setAutoRaise(true);
    m_profileButton->setToolTip(tr("Switch profile"));
    m_profileButton->setAccessibleName(tr("Profile"));

    m_menuButton = new QToolButton(this);
    m_menuButton->setObjectName(QStringLiteral("headerMenuButton"));
    m_menuButton->setIcon(IconLoader::icon(QStringLiteral("menu")));
    m_menuButton->setAutoRaise(true);
    m_menuButton->setToolTip(tr("Main menu"));
    m_menuButton->setAccessibleName(tr("Main menu"));

    layout->addWidget(m_title, 1);
    layout->addWidget(m_privateBadge, 0);
    layout->addWidget(m_profileButton, 0);
    layout->addWidget(m_menuButton, 0);

    connect(m_menuButton, &QToolButton::clicked, this, &HeaderBar::menuRequested);
    connect(m_profileButton, &QToolButton::clicked, this, &HeaderBar::profileRequested);
}

void HeaderBar::setTitle(const QString &title)
{
    m_title->setText(title.isEmpty() ? tr("Polecat Browser") : title);
}

void HeaderBar::setProfileName(const QString &name)
{
    m_profileButton->setText(name.isEmpty() ? tr("Default") : name);
}

void HeaderBar::setPrivate(bool isPrivate)
{
    m_privateBadge->setVisible(isPrivate);
}

} // namespace Polecat

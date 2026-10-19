#include "ui/Sidebar.h"

#include "utils/IconLoader.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace Polecat {

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setMinimumWidth(220);
    setMaximumWidth(360);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 6, 6, 0);
    auto *title = new QLabel(tr("Sidebar"), header);
    title->setObjectName(QStringLiteral("sidebarTitle"));
    auto *closeButton = new QToolButton(header);
    closeButton->setIcon(IconLoader::icon(QStringLiteral("close")));
    closeButton->setAutoRaise(true);
    closeButton->setToolTip(tr("Hide sidebar"));
    headerLayout->addWidget(title, 1);
    headerLayout->addWidget(closeButton, 0);
    connect(closeButton, &QToolButton::clicked, this, &Sidebar::closeRequested);

    m_rail = new QToolBar(this);
    m_rail->setObjectName(QStringLiteral("sidebarRail"));
    m_rail->setOrientation(Qt::Horizontal);
    m_rail->setIconSize(QSize(16, 16));
    m_rail->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_rail->setMovable(false);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("sidebarStack"));

    outer->addWidget(header);
    outer->addWidget(m_rail);
    outer->addWidget(m_stack, 1);
}

void Sidebar::addSection(const QString &id, const QString &title, const QString &iconName, QWidget *panel)
{
    const int index = m_stack->addWidget(panel);
    m_indexById.insert(id, index);

    QAction *action = m_rail->addAction(IconLoader::icon(iconName), title);
    action->setCheckable(true);
    action->setToolTip(title);
    action->setData(id);
    m_idByAction.insert(action, id);
    connect(action, &QAction::triggered, this, &Sidebar::onActionTriggered);

    if (m_currentId.isEmpty())
        setCurrentSection(id);
}

void Sidebar::setCurrentSection(const QString &id)
{
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd())
        return;
    m_currentId = id;
    m_stack->setCurrentIndex(it.value());

    for (auto a = m_idByAction.constBegin(); a != m_idByAction.constEnd(); ++a)
        a.key()->setChecked(a.value() == id);

    emit sectionChanged(id);
}

void Sidebar::onActionTriggered()
{
    auto *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    setCurrentSection(m_idByAction.value(action));
}

} // namespace Polecat

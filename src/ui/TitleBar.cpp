#include "ui/TitleBar.h"

#include "tabs/TabBar.h"
#include "utils/IconLoader.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTabBar>
#include <QToolButton>
#include <QWindow>

namespace Polecat {

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(40);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(6, 4, 6, 0);
    m_layout->setSpacing(2);

    m_minButton = makeButton(QStringLiteral("win-minimize"), tr("Minimise"), false);
    m_maxButton = makeButton(QStringLiteral("win-maximize"), tr("Maximise"), false);
    m_closeButton = makeButton(QStringLiteral("close"), tr("Close"), true);

    // The stretch after the (later inserted) tab strip is the drag handle.
    m_layout->addStretch(1);
    m_layout->addWidget(m_minButton, 0);
    m_layout->addWidget(m_maxButton, 0);
    m_layout->addWidget(m_closeButton, 0);

    connect(m_minButton, &QToolButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maxButton, &QToolButton::clicked, this, &TitleBar::toggleMaximizeRequested);
    connect(m_closeButton, &QToolButton::clicked, this, &TitleBar::closeRequested);
}

QToolButton *TitleBar::makeButton(const QString &iconName, const QString &tooltip, bool isClose)
{
    auto *b = new QToolButton(this);
    b->setObjectName(isClose ? QStringLiteral("winCloseButton") : QStringLiteral("winButton"));
    b->setIcon(IconLoader::icon(iconName));
    b->setIconSize(QSize(16, 16));
    b->setAutoRaise(true);
    b->setFocusPolicy(Qt::NoFocus);
    b->setToolTip(tooltip);
    b->setAccessibleName(tooltip);
    b->setFixedSize(34, 28);
    return b;
}

void TitleBar::setTabBar(TabBar *tabs)
{
    if (!tabs)
        return;
    m_tabs = tabs;
    // Put the tab strip at the far left, before the trailing stretch.
    m_layout->insertWidget(0, tabs, 0);
    // Dragging the empty area of the tab strip moves the window too.
    tabs->installEventFilter(this);
    if (QTabBar *bar = tabs->tabBar())
        bar->installEventFilter(this);
}

void TitleBar::setMaximized(bool maximized)
{
    m_maxButton->setIcon(IconLoader::icon(maximized ? QStringLiteral("win-restore")
                                                    : QStringLiteral("win-maximize")));
    m_maxButton->setToolTip(maximized ? tr("Restore") : tr("Maximise"));
    m_maxButton->setAccessibleName(maximized ? tr("Restore") : tr("Maximise"));
}

void TitleBar::startMove()
{
    if (QWidget *w = window()) {
        if (QWindow *handle = w->windowHandle())
            handle->startSystemMove();
    }
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        startMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit toggleMaximizeRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool TitleBar::eventFilter(QObject *watched, QEvent *event)
{
    // Only the tab strip (and its inner QTabBar) needs help: the strip covers
    // most of the row, so empty-space drags would otherwise never reach us.
    const bool isTabArea = (watched == m_tabs) || qobject_cast<QTabBar *>(watched) != nullptr;
    if (!isTabArea)
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonPress
        || event->type() == QEvent::MouseButtonDblClick) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            // Never steal a click that lands on an actual tab.
            bool onTab = false;
            if (auto *bar = qobject_cast<QTabBar *>(watched))
                onTab = bar->tabAt(me->position().toPoint()) >= 0;
            if (!onTab) {
                if (event->type() == QEvent::MouseButtonDblClick)
                    emit toggleMaximizeRequested();
                else
                    startMove();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace Polecat

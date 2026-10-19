#pragma once

#include <QWidget>

class QHBoxLayout;
class QToolButton;

namespace Polecat {

class TabBar;

// TitleBar is the application-drawn window title bar (client-side decoration),
// used when the window is frameless. It hosts the tab strip on the left and the
// window controls (minimise / maximise-restore / close) on the right, and lets
// the user drag the window from the empty space in between, exactly like Chrome
// and Firefox do. The actual move/resize is delegated to the compositor via
// QWindow::startSystemMove()/startSystemResize(), so it works on both Wayland
// and X11.
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

    // Adopts the tab strip (reparents it into this widget, at the far left).
    void setTabBar(TabBar *tabs);

    void setMaximized(bool maximized);

signals:
    void minimizeRequested();
    void toggleMaximizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QToolButton *makeButton(const QString &iconName, const QString &tooltip, bool isClose);
    void startMove();

    QHBoxLayout *m_layout = nullptr;
    TabBar *m_tabs = nullptr;
    QToolButton *m_minButton = nullptr;
    QToolButton *m_maxButton = nullptr;
    QToolButton *m_closeButton = nullptr;
};

} // namespace Polecat

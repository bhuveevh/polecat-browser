#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class QToolButton;

namespace Polecat {

// HeaderBar is the GNOME-style top strip: application/tab title on the left,
// an optional Private badge, a profile chip and the main menu button on the
// right. It is intentionally minimal and degrades to a normal title bar when
// the platform does not support client-side decorations.
class HeaderBar : public QWidget
{
    Q_OBJECT

public:
    explicit HeaderBar(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setProfileName(const QString &name);
    void setPrivate(bool isPrivate);

signals:
    void menuRequested();
    void profileRequested();

private:
    QLabel *m_title = nullptr;
    QLabel *m_privateBadge = nullptr;
    QToolButton *m_profileButton = nullptr;
    QToolButton *m_menuButton = nullptr;
};

} // namespace Polecat

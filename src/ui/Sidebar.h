#pragma once

#include <QWidget>
#include <QHash>
#include <QString>

class QStackedWidget;
class QToolBar;
class QAction;

namespace Polecat {

// Sidebar is the optional, collapsible left panel holding Bookmarks, History,
// Downloads, Reading List, Notes and Study Tools. On narrow windows the window
// hides it by default; it never permanently steals too much content width.
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget *parent = nullptr);

    // Registers a section. `id` is stable ("bookmarks", "history", ...).
    void addSection(const QString &id, const QString &title, const QString &iconName, QWidget *panel);

    void setCurrentSection(const QString &id);
    QString currentSection() const { return m_currentId; }

signals:
    void sectionChanged(const QString &id);
    void closeRequested();

private:
    void onActionTriggered();

    QStackedWidget *m_stack = nullptr;
    QToolBar *m_rail = nullptr;
    QHash<QString, int> m_indexById;
    QHash<QAction *, QString> m_idByAction;
    QString m_currentId;
};

} // namespace Polecat

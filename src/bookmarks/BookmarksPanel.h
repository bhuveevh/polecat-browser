#pragma once

#include <QWidget>
#include <QUrl>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace Polecat {

class BookmarkManager;

// BookmarksPanel is the sidebar view of bookmarks: a search field over a tree
// of folders and bookmarks, with open / edit / delete actions.
class BookmarksPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BookmarksPanel(BookmarkManager *manager, QWidget *parent = nullptr);

    void reload();

signals:
    void openUrl(const QUrl &url);

private slots:
    void onSearchChanged(const QString &text);
    void onItemActivated(QTreeWidgetItem *item, int column);
    void showContextMenu(const QPoint &pos);

private:
    void buildTree();

    BookmarkManager *m_manager = nullptr;
    QLineEdit *m_search = nullptr;
    QTreeWidget *m_tree = nullptr;
};

} // namespace Polecat

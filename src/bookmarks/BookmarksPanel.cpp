#include "bookmarks/BookmarksPanel.h"

#include "bookmarks/BookmarkManager.h"
#include "utils/IconLoader.h"

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QMenu>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace Polecat {

BookmarksPanel::BookmarksPanel(BookmarkManager *manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search bookmarks"));
    m_search->setClearButtonEnabled(true);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setAccessibleName(tr("Bookmarks"));

    layout->addWidget(m_search);
    layout->addWidget(m_tree, 1);

    connect(m_search, &QLineEdit::textChanged, this, &BookmarksPanel::onSearchChanged);
    connect(m_tree, &QTreeWidget::itemActivated, this, &BookmarksPanel::onItemActivated);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &BookmarksPanel::showContextMenu);

    if (m_manager) {
        connect(m_manager, &BookmarkManager::changed, this, &BookmarksPanel::reload);
        m_manager->ensureRootFolder();
    }
    reload();
}

void BookmarksPanel::reload()
{
    if (!m_manager)
        return;
    if (m_search->text().trimmed().isEmpty())
        buildTree();
    else
        onSearchChanged(m_search->text());
}

void BookmarksPanel::buildTree()
{
    m_tree->clear();

    // Add folders as top-level nodes, then loose bookmarks.
    const QList<Bookmark> roots = m_manager->children(-1);
    for (const Bookmark &b : roots) {
        if (b.isFolder()) {
            auto *folder = new QTreeWidgetItem(m_tree);
            folder->setText(0, b.title);
            folder->setIcon(0, IconLoader::icon(QStringLiteral("bookmarks")));
            folder->setData(0, Qt::UserRole, b.id);
            folder->setData(0, Qt::UserRole + 1, QString());
            const QList<Bookmark> kids = m_manager->children(b.id);
            for (const Bookmark &k : kids) {
                auto *child = new QTreeWidgetItem(folder);
                child->setText(0, k.title.isEmpty() ? k.url.toString() : k.title);
                child->setIcon(0, IconLoader::icon(QStringLiteral("bookmark")));
                child->setData(0, Qt::UserRole, k.id);
                child->setData(0, Qt::UserRole + 1, k.url.toString());
                child->setToolTip(0, k.url.toString());
            }
            folder->setExpanded(true);
        } else {
            auto *item = new QTreeWidgetItem(m_tree);
            item->setText(0, b.title.isEmpty() ? b.url.toString() : b.title);
            item->setIcon(0, IconLoader::icon(QStringLiteral("bookmark")));
            item->setData(0, Qt::UserRole, b.id);
            item->setData(0, Qt::UserRole + 1, b.url.toString());
            item->setToolTip(0, b.url.toString());
        }
    }
}

void BookmarksPanel::onSearchChanged(const QString &text)
{
    if (!m_manager)
        return;
    m_tree->clear();
    if (text.trimmed().isEmpty()) {
        buildTree();
        return;
    }
    const QList<Bookmark> results = m_manager->search(text);
    for (const Bookmark &b : results) {
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(0, b.title.isEmpty() ? b.url.toString() : b.title);
        item->setIcon(0, IconLoader::icon(QStringLiteral("bookmark")));
        item->setData(0, Qt::UserRole, b.id);
        item->setData(0, Qt::UserRole + 1, b.url.toString());
        item->setToolTip(0, b.url.toString());
    }
}

void BookmarksPanel::onItemActivated(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item)
        return;
    const QString url = item->data(0, Qt::UserRole + 1).toString();
    if (!url.isEmpty())
        emit openUrl(QUrl(url));
}

void BookmarksPanel::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    if (!item || !m_manager)
        return;

    const qint64 id = item->data(0, Qt::UserRole).toLongLong();
    const QString url = item->data(0, Qt::UserRole + 1).toString();
    const bool isFolder = url.isEmpty();

    QMenu menu(this);
    QAction *open = isFolder ? nullptr : menu.addAction(tr("Open"));
    QAction *copyUrl = isFolder ? nullptr : menu.addAction(tr("Copy link"));
    menu.addSeparator();
    QAction *remove = menu.addAction(tr("Delete"));

    QAction *chosen = menu.exec(m_tree->mapToGlobal(pos));
    if (!chosen)
        return;
    if (open && chosen == open)
        emit openUrl(QUrl(url));
    else if (copyUrl && chosen == copyUrl)
        QApplication::clipboard()->setText(url);
    else if (chosen == remove)
        m_manager->removeBookmark(id);
}

} // namespace Polecat

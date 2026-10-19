#pragma once

#include <QObject>
#include <QList>
#include <QUrl>
#include <QString>

namespace Polecat {

class DatabaseManager;

struct Bookmark {
    qint64 id = -1;
    qint64 parentId = -1;
    QString title;
    QUrl url;
    QString favicon;
    qint64 created = 0;
    qint64 modified = 0;
    int position = 0;

    bool isFolder() const { return url.isEmpty(); }
};

// BookmarkManager provides CRUD over the bookmarks table. Folders are rows with
// an empty url and a parent_id forming a tree; the implicit root is parent_id
// NULL (represented as -1 in this API).
class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    explicit BookmarkManager(DatabaseManager *db, QObject *parent = nullptr);

    qint64 addBookmark(qint64 parentId, const QString &title, const QUrl &url,
                       const QString &favicon = QString());
    qint64 addFolder(qint64 parentId, const QString &title);

    bool updateBookmark(qint64 id, const QString &title, const QUrl &url);
    bool removeBookmark(qint64 id);
    bool moveBookmark(qint64 id, qint64 newParentId, int newPosition = -1);

    QList<Bookmark> children(qint64 parentId) const;
    QList<Bookmark> all() const;
    QList<Bookmark> search(const QString &query) const;

    bool isBookmarked(const QUrl &url) const;
    Bookmark findByUrl(const QUrl &url) const;

    // Ensures a default "Bookmarks" root folder exists; returns its id.
    qint64 ensureRootFolder();

    bool importHtml(const QString &filePath, qint64 parentId = -1);
    bool exportHtml(const QString &filePath) const;

signals:
    void changed();

private:
    qint64 nextPosition(qint64 parentId) const;
    void touch(qint64 id);

    DatabaseManager *m_db = nullptr;
};

} // namespace Polecat

#include "bookmarks/BookmarkManager.h"

#include "database/DatabaseManager.h"
#include "utils/Logging.h"
#include "utils/UrlUtils.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QVariant>

namespace Polecat {

namespace {
qint64 nowSecs() { return QDateTime::currentSecsSinceEpoch(); }
}

BookmarkManager::BookmarkManager(DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

qint64 BookmarkManager::nextPosition(qint64 parentId) const
{
    QSqlQuery q(m_db->database());
    if (parentId < 0)
        q.prepare(QStringLiteral("SELECT COALESCE(MAX(position), -1) + 1 FROM bookmarks WHERE parent_id IS NULL"));
    else {
        q.prepare(QStringLiteral("SELECT COALESCE(MAX(position), -1) + 1 FROM bookmarks WHERE parent_id = ?"));
        q.addBindValue(parentId);
    }
    if (q.exec() && q.next())
        return q.value(0).toLongLong();
    return 0;
}

qint64 BookmarkManager::addBookmark(qint64 parentId, const QString &title, const QUrl &url,
                                    const QString &favicon)
{
    if (!m_db->isOpen() || !url.isValid() || url.isEmpty())
        return -1;

    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "INSERT INTO bookmarks(parent_id, title, url, favicon, created, modified, position) "
        "VALUES(?, ?, ?, ?, ?, ?, ?)"));
    const qint64 t = nowSecs();
    q.addBindValue(parentId < 0 ? QVariant() : QVariant(parentId));
    q.addBindValue(title.isEmpty() ? url.host() : title);
    q.addBindValue(url.toString());
    q.addBindValue(favicon);
    q.addBindValue(t);
    q.addBindValue(t);
    q.addBindValue(nextPosition(parentId));

    if (!q.exec()) {
        qCWarning(lcDb) << "addBookmark failed:" << q.lastError().text();
        return -1;
    }
    emit changed();
    return q.lastInsertId().toLongLong();
}

qint64 BookmarkManager::addFolder(qint64 parentId, const QString &title)
{
    if (!m_db->isOpen())
        return -1;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "INSERT INTO bookmarks(parent_id, title, url, favicon, created, modified, position) "
        "VALUES(?, ?, NULL, NULL, ?, ?, ?)"));
    const qint64 t = nowSecs();
    q.addBindValue(parentId < 0 ? QVariant() : QVariant(parentId));
    q.addBindValue(title);
    q.addBindValue(t);
    q.addBindValue(t);
    q.addBindValue(nextPosition(parentId));
    if (!q.exec()) {
        qCWarning(lcDb) << "addFolder failed:" << q.lastError().text();
        return -1;
    }
    emit changed();
    return q.lastInsertId().toLongLong();
}

bool BookmarkManager::updateBookmark(qint64 id, const QString &title, const QUrl &url)
{
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("UPDATE bookmarks SET title = ?, url = ?, modified = ? WHERE id = ?"));
    q.addBindValue(title);
    q.addBindValue(url.toString());
    q.addBindValue(nowSecs());
    q.addBindValue(id);
    const bool ok = q.exec();
    if (ok)
        emit changed();
    return ok;
}

bool BookmarkManager::removeBookmark(qint64 id)
{
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("DELETE FROM bookmarks WHERE id = ?"));
    q.addBindValue(id);
    const bool ok = q.exec();
    if (ok)
        emit changed();
    return ok;
}

bool BookmarkManager::moveBookmark(qint64 id, qint64 newParentId, int newPosition)
{
    if (!m_db->beginTransaction())
        return false;

    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("UPDATE bookmarks SET parent_id = ?, position = ?, modified = ? WHERE id = ?"));
    q.addBindValue(newParentId < 0 ? QVariant() : QVariant(newParentId));
    q.addBindValue(newPosition < 0 ? nextPosition(newParentId) : newPosition);
    q.addBindValue(nowSecs());
    q.addBindValue(id);

    if (!q.exec()) {
        m_db->rollback();
        return false;
    }
    if (!m_db->commit()) {
        m_db->rollback();
        return false;
    }
    emit changed();
    return true;
}

QList<Bookmark> BookmarkManager::children(qint64 parentId) const
{
    QList<Bookmark> result;
    QSqlQuery q(m_db->database());
    if (parentId < 0)
        q.prepare(QStringLiteral("SELECT id,parent_id,title,url,favicon,created,modified,position "
                                 "FROM bookmarks WHERE parent_id IS NULL ORDER BY position, title"));
    else {
        q.prepare(QStringLiteral("SELECT id,parent_id,title,url,favicon,created,modified,position "
                                 "FROM bookmarks WHERE parent_id = ? ORDER BY position, title"));
        q.addBindValue(parentId);
    }
    if (!q.exec())
        return result;
    while (q.next()) {
        Bookmark b;
        b.id = q.value(0).toLongLong();
        b.parentId = q.value(1).isNull() ? -1 : q.value(1).toLongLong();
        b.title = q.value(2).toString();
        b.url = QUrl(q.value(3).toString());
        b.favicon = q.value(4).toString();
        b.created = q.value(5).toLongLong();
        b.modified = q.value(6).toLongLong();
        b.position = q.value(7).toInt();
        result.append(b);
    }
    return result;
}

QList<Bookmark> BookmarkManager::all() const
{
    QList<Bookmark> result;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("SELECT id,parent_id,title,url,favicon,created,modified,position "
                             "FROM bookmarks ORDER BY parent_id, position, title"));
    if (!q.exec())
        return result;
    while (q.next()) {
        Bookmark b;
        b.id = q.value(0).toLongLong();
        b.parentId = q.value(1).isNull() ? -1 : q.value(1).toLongLong();
        b.title = q.value(2).toString();
        b.url = QUrl(q.value(3).toString());
        b.favicon = q.value(4).toString();
        b.created = q.value(5).toLongLong();
        b.modified = q.value(6).toLongLong();
        b.position = q.value(7).toInt();
        result.append(b);
    }
    return result;
}

QList<Bookmark> BookmarkManager::search(const QString &query) const
{
    QList<Bookmark> result;
    if (query.trimmed().isEmpty())
        return all();

    const QString needle = QStringLiteral("%") + UrlUtils::normalizeForSearch(query) + QStringLiteral("%");
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "SELECT id,parent_id,title,url,favicon,created,modified,position FROM bookmarks "
        "WHERE url IS NOT NULL AND (lower(title) LIKE ? OR lower(url) LIKE ?) "
        "ORDER BY title LIMIT 500"));
    q.addBindValue(needle);
    q.addBindValue(needle);
    if (!q.exec())
        return result;
    while (q.next()) {
        Bookmark b;
        b.id = q.value(0).toLongLong();
        b.parentId = q.value(1).isNull() ? -1 : q.value(1).toLongLong();
        b.title = q.value(2).toString();
        b.url = QUrl(q.value(3).toString());
        b.favicon = q.value(4).toString();
        b.created = q.value(5).toLongLong();
        b.modified = q.value(6).toLongLong();
        b.position = q.value(7).toInt();
        result.append(b);
    }
    return result;
}

bool BookmarkManager::isBookmarked(const QUrl &url) const
{
    return findByUrl(url).id > 0;
}

Bookmark BookmarkManager::findByUrl(const QUrl &url) const
{
    Bookmark b;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("SELECT id,parent_id,title,url,favicon,created,modified,position "
                             "FROM bookmarks WHERE url = ? LIMIT 1"));
    q.addBindValue(url.toString());
    if (q.exec() && q.next()) {
        b.id = q.value(0).toLongLong();
        b.parentId = q.value(1).isNull() ? -1 : q.value(1).toLongLong();
        b.title = q.value(2).toString();
        b.url = QUrl(q.value(3).toString());
        b.favicon = q.value(4).toString();
        b.created = q.value(5).toLongLong();
        b.modified = q.value(6).toLongLong();
        b.position = q.value(7).toInt();
    }
    return b;
}

qint64 BookmarkManager::ensureRootFolder()
{
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("SELECT id FROM bookmarks WHERE parent_id IS NULL AND url IS NULL AND title = ? LIMIT 1"));
    q.addBindValue(QStringLiteral("Bookmarks"));
    if (q.exec() && q.next())
        return q.value(0).toLongLong();
    return addFolder(-1, QStringLiteral("Bookmarks"));
}

bool BookmarkManager::exportHtml(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(lcDb) << "Cannot write bookmark export:" << filePath;
        return false;
    }
    QTextStream out(&f);
    out << "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
        << "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
        << "<TITLE>Bookmarks</TITLE>\n<H1>Bookmarks</H1>\n<DL><p>\n";

    const QList<Bookmark> items = all();
    for (const Bookmark &b : items) {
        if (b.isFolder())
            continue;
        out << QStringLiteral("    <DT><A HREF=\"%1\" ADD_DATE=\"%2\">%3</A>\n")
                   .arg(b.url.toString().toHtmlEscaped())
                   .arg(b.created)
                   .arg(b.title.toHtmlEscaped());
    }
    out << "</DL><p>\n";
    return true;
}

bool BookmarkManager::importHtml(const QString &filePath, qint64 parentId)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    const QString content = QString::fromUtf8(f.readAll());
    static const QRegularExpression linkRe(
        QStringLiteral("<A[^>]*HREF=\"([^\"]+)\"[^>]*>([^<]*)</A>"),
        QRegularExpression::CaseInsensitiveOption);

    if (!m_db->beginTransaction())
        return false;

    int imported = 0;
    auto it = linkRe.globalMatch(content);
    while (it.hasNext()) {
        const auto m = it.next();
        const QUrl url(m.captured(1));
        if (!url.isValid() || url.isEmpty())
            continue;
        if (addBookmark(parentId, m.captured(2).trimmed(), url) > 0)
            ++imported;
    }
    m_db->commit();
    qCInfo(lcDb) << "Imported" << imported << "bookmarks from" << filePath;
    emit changed();
    return imported > 0;
}

void BookmarkManager::touch(qint64 id)
{
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("UPDATE bookmarks SET modified = ? WHERE id = ?"));
    q.addBindValue(nowSecs());
    q.addBindValue(id);
    q.exec();
}

} // namespace Polecat

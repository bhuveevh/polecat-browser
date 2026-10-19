#include "history/HistoryManager.h"

#include "database/DatabaseManager.h"
#include "utils/Logging.h"
#include "utils/UrlUtils.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace Polecat {

namespace {
qint64 nowSecs() { return QDateTime::currentSecsSinceEpoch(); }
}

HistoryManager::HistoryManager(DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

bool HistoryManager::recordVisit(const QString &title, const QUrl &url)
{
    if (!m_db->isOpen() || !url.isValid() || url.isEmpty())
        return false;

    // Never store internal pages or non-web schemes.
    const QString scheme = url.scheme();
    if (scheme == QLatin1String("about") || scheme == QLatin1String("file")
        || scheme == QLatin1String("data") || scheme == QLatin1String("chrome"))
        return false;

    const QString urlStr = url.toString();
    const QString titleNorm = UrlUtils::normalizeForSearch(title);
    const QString urlNorm = UrlUtils::normalizeForSearch(urlStr);
    const qint64 t = nowSecs();

    QSqlQuery find(m_db->database());
    find.prepare(QStringLiteral("SELECT id, visit_count FROM history WHERE url = ? LIMIT 1"));
    find.addBindValue(urlStr);

    if (find.exec() && find.next()) {
        const qint64 id = find.value(0).toLongLong();
        const int count = find.value(1).toInt();
        QSqlQuery upd(m_db->database());
        upd.prepare(QStringLiteral(
            "UPDATE history SET title = ?, visit_count = ?, last_visit = ?, "
            "title_norm = ?, url_norm = ? WHERE id = ?"));
        upd.addBindValue(title);
        upd.addBindValue(count + 1);
        upd.addBindValue(t);
        upd.addBindValue(titleNorm);
        upd.addBindValue(urlNorm);
        upd.addBindValue(id);
        if (!upd.exec()) {
            qCWarning(lcDb) << "history update failed:" << upd.lastError().text();
            return false;
        }
    } else {
        QSqlQuery ins(m_db->database());
        ins.prepare(QStringLiteral(
            "INSERT INTO history(title, url, visit_time, visit_count, last_visit, title_norm, url_norm) "
            "VALUES(?, ?, ?, 1, ?, ?, ?)"));
        ins.addBindValue(title);
        ins.addBindValue(urlStr);
        ins.addBindValue(t);
        ins.addBindValue(t);
        ins.addBindValue(titleNorm);
        ins.addBindValue(urlNorm);
        if (!ins.exec()) {
            qCWarning(lcDb) << "history insert failed:" << ins.lastError().text();
            return false;
        }
    }

    emit changed();
    return true;
}

QList<HistoryEntry> HistoryManager::recent(int limit) const
{
    QList<HistoryEntry> result;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "SELECT id,title,url,visit_time,visit_count,last_visit FROM history "
        "ORDER BY last_visit DESC LIMIT ?"));
    q.addBindValue(limit);
    if (!q.exec())
        return result;
    while (q.next()) {
        HistoryEntry e;
        e.id = q.value(0).toLongLong();
        e.title = q.value(1).toString();
        e.url = QUrl(q.value(2).toString());
        e.visitTime = q.value(3).toLongLong();
        e.visitCount = q.value(4).toInt();
        e.lastVisit = q.value(5).toLongLong();
        result.append(e);
    }
    return result;
}

QList<HistoryEntry> HistoryManager::search(const QString &query, int limit) const
{
    QList<HistoryEntry> result;
    if (query.trimmed().isEmpty())
        return recent(limit);

    const QString needle = QStringLiteral("%") + UrlUtils::normalizeForSearch(query) + QStringLiteral("%");
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "SELECT id,title,url,visit_time,visit_count,last_visit FROM history "
        "WHERE title_norm LIKE ? OR url_norm LIKE ? "
        "ORDER BY last_visit DESC LIMIT ?"));
    q.addBindValue(needle);
    q.addBindValue(needle);
    q.addBindValue(limit);
    if (!q.exec())
        return result;
    while (q.next()) {
        HistoryEntry e;
        e.id = q.value(0).toLongLong();
        e.title = q.value(1).toString();
        e.url = QUrl(q.value(2).toString());
        e.visitTime = q.value(3).toLongLong();
        e.visitCount = q.value(4).toInt();
        e.lastVisit = q.value(5).toLongLong();
        result.append(e);
    }
    return result;
}

QList<HistoryEntry> HistoryManager::topSites(int limit) const
{
    QList<HistoryEntry> result;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "SELECT id,title,url,visit_time,visit_count,last_visit FROM history "
        "ORDER BY visit_count DESC, last_visit DESC LIMIT ?"));
    q.addBindValue(limit);
    if (!q.exec())
        return result;
    while (q.next()) {
        HistoryEntry e;
        e.id = q.value(0).toLongLong();
        e.title = q.value(1).toString();
        e.url = QUrl(q.value(2).toString());
        e.visitTime = q.value(3).toLongLong();
        e.visitCount = q.value(4).toInt();
        e.lastVisit = q.value(5).toLongLong();
        result.append(e);
    }
    return result;
}

bool HistoryManager::removeEntry(qint64 id)
{
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("DELETE FROM history WHERE id = ?"));
    q.addBindValue(id);
    const bool ok = q.exec();
    if (ok)
        emit changed();
    return ok;
}

bool HistoryManager::removeEntries(const QList<qint64> &ids)
{
    if (ids.isEmpty())
        return true;
    if (!m_db->beginTransaction())
        return false;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("DELETE FROM history WHERE id = ?"));
    for (qint64 id : ids) {
        q.addBindValue(id);
        if (!q.exec()) {
            m_db->rollback();
            return false;
        }
        q.finish();
    }
    if (!m_db->commit()) {
        m_db->rollback();
        return false;
    }
    emit changed();
    return true;
}

bool HistoryManager::clearAll()
{
    QSqlQuery q(m_db->database());
    const bool ok = q.exec(QStringLiteral("DELETE FROM history"));
    if (ok)
        emit changed();
    return ok;
}

bool HistoryManager::clearRange(qint64 fromSecs, qint64 toSecs)
{
    QSqlQuery q(m_db->database());
    if (fromSecs < 0 && toSecs < 0)
        return clearAll();

    if (fromSecs >= 0 && toSecs >= 0) {
        q.prepare(QStringLiteral("DELETE FROM history WHERE last_visit BETWEEN ? AND ?"));
        q.addBindValue(fromSecs);
        q.addBindValue(toSecs);
    } else if (fromSecs >= 0) {
        q.prepare(QStringLiteral("DELETE FROM history WHERE last_visit >= ?"));
        q.addBindValue(fromSecs);
    } else {
        q.prepare(QStringLiteral("DELETE FROM history WHERE last_visit <= ?"));
        q.addBindValue(toSecs);
    }
    const bool ok = q.exec();
    if (ok)
        emit changed();
    return ok;
}

int HistoryManager::count() const
{
    QSqlQuery q(m_db->database());
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM history")) && q.next())
        return q.value(0).toInt();
    return 0;
}

} // namespace Polecat

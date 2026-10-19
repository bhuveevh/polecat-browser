#pragma once

#include <QObject>
#include <QList>
#include <QUrl>
#include <QString>

namespace Polecat {

class DatabaseManager;

struct HistoryEntry {
    qint64 id = -1;
    QString title;
    QUrl url;
    qint64 visitTime = 0;
    int visitCount = 1;
    qint64 lastVisit = 0;
};

// HistoryManager records and queries browsing history in SQLite.
//
// Privacy: callers must not invoke recordVisit() for off-the-record pages.
// recordVisit() also refuses "about:" URLs and internal pages.
class HistoryManager : public QObject
{
    Q_OBJECT

public:
    explicit HistoryManager(DatabaseManager *db, QObject *parent = nullptr);

    // Records a visit, merging with an existing row for the same URL.
    bool recordVisit(const QString &title, const QUrl &url);

    QList<HistoryEntry> recent(int limit = 200) const;
    QList<HistoryEntry> search(const QString &query, int limit = 500) const;
    QList<HistoryEntry> topSites(int limit = 12) const;

    bool removeEntry(qint64 id);
    bool removeEntries(const QList<qint64> &ids);
    bool clearAll();
    // Clears entries with last_visit in [fromSecs, toSecs]; pass -1 for open ends.
    bool clearRange(qint64 fromSecs, qint64 toSecs);

    int count() const;

signals:
    void changed();

private:
    DatabaseManager *m_db = nullptr;
};

} // namespace Polecat

#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>
#include <QPointer>

class QWebEngineProfile;
class QWebEngineDownloadRequest;

namespace Polecat {

class SettingsManager;
class DatabaseManager;

// DownloadEntry tracks a single download. It wraps a QWebEngineDownloadRequest
// (whose lifetime is owned by Qt WebEngine) and exposes a stable, UI-friendly
// view of its state.
class DownloadEntry : public QObject
{
    Q_OBJECT

public:
    enum State { InProgress, Paused, Completed, Cancelled, Interrupted };

    DownloadEntry(QWebEngineDownloadRequest *request, bool persistent, QObject *parent = nullptr);

    qint64 id() const { return m_id; }
    QString fileName() const;
    QUrl sourceUrl() const;
    QString destination() const;
    qint64 totalBytes() const;
    qint64 receivedBytes() const;
    QString mimeType() const;
    State state() const;
    bool isPaused() const;
    bool isPersistent() const { return m_persistent; }

    // Returns 0..100, or -1 when total size is unknown.
    int progressPercent() const;
    double speedBytesPerSecond() const { return m_speed; }
    qint64 remainingSeconds() const;   // -1 when unknown

    void pause();
    void resume();
    void cancel();
    bool openFile();
    void openContainingFolder();

signals:
    void updated();

private slots:
    void onReceivedBytesChanged();
    void onStateChanged();
    void onTotalBytesChanged();

private:
    void recomputeSpeed();

    QPointer<QWebEngineDownloadRequest> m_request;
    qint64 m_id = -1;
    bool m_persistent = true;
    qint64 m_lastBytes = 0;
    qint64 m_lastSampleMs = 0;
    double m_speed = 0.0;
};

// DownloadsManager listens to download requests for one or more profiles, asks
// the user where to save when configured, sanitises filenames, warns about
// dangerous types and keeps an in-memory list of entries (persisted to SQLite
// for non-private profiles only).
class DownloadsManager : public QObject
{
    Q_OBJECT

public:
    DownloadsManager(SettingsManager *settings, DatabaseManager *db, QObject *parent = nullptr);

    // Begin observing a profile. Private (off-the-record) profiles are still
    // observed but their metadata is never persisted.
    void registerProfile(QWebEngineProfile *profile, bool persistent);

    const QList<DownloadEntry *> &entries() const { return m_entries; }
    int activeCount() const;

    void removeEntry(DownloadEntry *entry);
    void clearCompleted();
    void cancelAll();

signals:
    void downloadStarted(Polecat::DownloadEntry *entry);
    void downloadFinished(Polecat::DownloadEntry *entry);
    void entriesChanged();

private slots:
    void onDownloadRequested(QWebEngineDownloadRequest *request);

private:
    bool prepareDestination(QWebEngineDownloadRequest *request, QString &dirOut, QString &nameOut);
    void persistRow(DownloadEntry *entry);
    void updateRow(DownloadEntry *entry);

    SettingsManager *m_settings = nullptr;
    DatabaseManager *m_db = nullptr;
    QList<DownloadEntry *> m_entries;
    qint64 m_nextId = 1;
};

} // namespace Polecat

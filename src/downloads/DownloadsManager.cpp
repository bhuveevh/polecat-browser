#include "downloads/DownloadsManager.h"

#include "database/DatabaseManager.h"
#include "settings/SettingsManager.h"
#include "utils/Logging.h"
#include "utils/Paths.h"
#include "utils/UrlUtils.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QWebEngineDownloadRequest>
#include <QWebEnginePage>
#include <QWebEngineProfile>

namespace Polecat {

// ---------------------------------------------------------------------------
// DownloadEntry
// ---------------------------------------------------------------------------

DownloadEntry::DownloadEntry(QWebEngineDownloadRequest *request, bool persistent, QObject *parent)
    : QObject(parent)
    , m_request(request)
    , m_persistent(persistent)
    , m_id(request ? request->id() : -1)
{
    m_lastSampleMs = QDateTime::currentMSecsSinceEpoch();
    if (m_request) {
        connect(m_request, &QWebEngineDownloadRequest::receivedBytesChanged,
                this, &DownloadEntry::onReceivedBytesChanged);
        connect(m_request, &QWebEngineDownloadRequest::totalBytesChanged,
                this, &DownloadEntry::onTotalBytesChanged);
        connect(m_request, &QWebEngineDownloadRequest::stateChanged,
                this, &DownloadEntry::onStateChanged);
    }
}

QString DownloadEntry::fileName() const
{
    return m_request ? m_request->downloadFileName() : QString();
}

QUrl DownloadEntry::sourceUrl() const
{
    return m_request ? m_request->url() : QUrl();
}

QString DownloadEntry::destination() const
{
    if (!m_request)
        return QString();
    return QDir(m_request->downloadDirectory()).filePath(m_request->downloadFileName());
}

qint64 DownloadEntry::totalBytes() const
{
    return m_request ? m_request->totalBytes() : 0;
}

qint64 DownloadEntry::receivedBytes() const
{
    return m_request ? m_request->receivedBytes() : 0;
}

QString DownloadEntry::mimeType() const
{
    return m_request ? m_request->mimeType() : QString();
}

DownloadEntry::State DownloadEntry::state() const
{
    if (!m_request)
        return Interrupted;
    switch (m_request->state()) {
    case QWebEngineDownloadRequest::DownloadRequested:
    case QWebEngineDownloadRequest::DownloadInProgress:
        return m_request->isPaused() ? Paused : InProgress;
    case QWebEngineDownloadRequest::DownloadCompleted:
        return Completed;
    case QWebEngineDownloadRequest::DownloadCancelled:
        return Cancelled;
    case QWebEngineDownloadRequest::DownloadInterrupted:
        return Interrupted;
    }
    return Interrupted;
}

bool DownloadEntry::isPaused() const
{
    return m_request && m_request->isPaused();
}

int DownloadEntry::progressPercent() const
{
    const qint64 total = totalBytes();
    if (total <= 0)
        return -1;
    return int((receivedBytes() * 100) / total);
}

qint64 DownloadEntry::remainingSeconds() const
{
    const qint64 total = totalBytes();
    if (total <= 0 || m_speed <= 1.0)
        return -1;
    const qint64 remaining = total - receivedBytes();
    if (remaining <= 0)
        return 0;
    return qint64(remaining / m_speed);
}

void DownloadEntry::pause()
{
    if (m_request)
        m_request->pause();
}

void DownloadEntry::resume()
{
    if (m_request)
        m_request->resume();
}

void DownloadEntry::cancel()
{
    if (m_request)
        m_request->cancel();
}

bool DownloadEntry::openFile()
{
    const QString path = destination();
    if (!QFileInfo::exists(path))
        return false;
    // Never auto-execute: warn for dangerous types and let the user decide.
    if (UrlUtils::isDangerousExtension(path)) {
        const auto choice = QMessageBox::warning(
            nullptr, tr("Open downloaded file?"),
            tr("\"%1\" may be unsafe to run. Open it anyway?").arg(QFileInfo(path).fileName()),
            QMessageBox::Open | QMessageBox::Cancel, QMessageBox::Cancel);
        if (choice != QMessageBox::Open)
            return false;
    }
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void DownloadEntry::openContainingFolder()
{
    const QString path = destination();
    const QString dir = QFileInfo(path).absolutePath();
    if (QDir(dir).exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void DownloadEntry::recomputeSpeed()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsed = nowMs - m_lastSampleMs;
    if (elapsed <= 0)
        return;
    const qint64 bytes = receivedBytes();
    const qint64 delta = bytes - m_lastBytes;
    if (delta >= 0) {
        const double instant = double(delta) * 1000.0 / double(elapsed);
        // Exponential smoothing so the displayed speed is readable.
        m_speed = (m_speed <= 0.0) ? instant : (m_speed * 0.7 + instant * 0.3);
    }
    m_lastBytes = bytes;
    m_lastSampleMs = nowMs;
}

void DownloadEntry::onReceivedBytesChanged()
{
    recomputeSpeed();
    emit updated();
}

void DownloadEntry::onStateChanged()
{
    if (state() == Completed || state() == Cancelled || state() == Interrupted)
        m_speed = 0.0;
    emit updated();
}

void DownloadEntry::onTotalBytesChanged()
{
    emit updated();
}

// ---------------------------------------------------------------------------
// DownloadsManager
// ---------------------------------------------------------------------------

DownloadsManager::DownloadsManager(SettingsManager *settings, DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_db(db)
{
}

void DownloadsManager::registerProfile(QWebEngineProfile *profile, bool persistent)
{
    if (!profile)
        return;
    connect(profile, &QWebEngineProfile::downloadRequested,
            this, &DownloadsManager::onDownloadRequested);
    Q_UNUSED(persistent);
}

void DownloadsManager::onDownloadRequested(QWebEngineDownloadRequest *request)
{
    if (!request)
        return;

    const bool persistent = !request->page()
                            || !request->page()->profile()->isOffTheRecord();

    QString dir;
    QString name;
    if (!prepareDestination(request, dir, name)) {
        request->cancel();
        return;
    }

    request->setDownloadDirectory(dir);
    request->setDownloadFileName(name);
    request->accept();

    auto *entry = new DownloadEntry(request, persistent, this);
    m_entries.append(entry);
    connect(entry, &DownloadEntry::updated, this, [this, entry]() {
        updateRow(entry);
        if (entry->state() == DownloadEntry::Completed
            || entry->state() == DownloadEntry::Cancelled
            || entry->state() == DownloadEntry::Interrupted) {
            emit downloadFinished(entry);
        }
    });

    if (persistent)
        persistRow(entry);

    emit downloadStarted(entry);
    emit entriesChanged();
    qCInfo(lcNet) << "Download started:" << name << "->" << dir;
}

bool DownloadsManager::prepareDestination(QWebEngineDownloadRequest *request,
                                          QString &dirOut, QString &nameOut)
{
    // 1. Sanitise the filename (defeats path traversal and unsafe characters).
    const QString safeName = UrlUtils::sanitizeFileName(request->downloadFileName());

    // 2. Warn about dangerous extensions before accepting.
    if (UrlUtils::isDangerousExtension(safeName)) {
        const auto choice = QMessageBox::warning(
            nullptr, tr("Download potentially unsafe file?"),
            tr("\"%1\" is an executable or script and could harm your computer.\n"
               "Only open files you trust.").arg(safeName),
            QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Cancel);
        if (choice != QMessageBox::Save)
            return false;
    }

    // 3. Determine the directory.
    QString dir = m_settings->downloadDir();
    const bool ask = m_settings->value(SettingsManager::kAskWhereToSave(), false).toBool();

    if (ask) {
        const QString chosen = QFileDialog::getSaveFileName(
            nullptr, tr("Save file"), QDir(dir).filePath(safeName));
        if (chosen.isEmpty())
            return false;
        dir = QFileInfo(chosen).absolutePath();
        nameOut = QFileInfo(chosen).fileName();
    } else {
        nameOut = safeName;
    }

    // 4. Validate/create the directory; fall back to the XDG download dir.
    if (!Paths::ensureDir(dir) || !Paths::isWritableDir(dir)) {
        qCWarning(lcNet) << "Download directory unusable:" << dir << "- falling back";
        dir = Paths::downloadsDir();
        if (!Paths::ensureDir(dir) || !Paths::isWritableDir(dir)) {
            QMessageBox::warning(nullptr, tr("Download failed"),
                tr("Polecat Browser could not find a writable folder to save the file to."));
            return false;
        }
    }

    // 5. Avoid silently overwriting an existing file.
    const QString full = QDir(dir).filePath(nameOut);
    if (QFileInfo::exists(full)) {
        const auto choice = QMessageBox::question(
            nullptr, tr("File already exists"),
            tr("A file named \"%1\" already exists. Replace it?").arg(nameOut),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            // Save under a unique name instead of overwriting.
            QFileInfo info(full);
            const QString base = info.completeBaseName();
            const QString suffix = info.suffix();
            int n = 1;
            QString candidate;
            do {
                candidate = suffix.isEmpty()
                    ? QStringLiteral("%1 (%2)").arg(base).arg(n)
                    : QStringLiteral("%1 (%2).%3").arg(base).arg(n).arg(suffix);
                ++n;
            } while (QFileInfo::exists(QDir(dir).filePath(candidate)) && n < 1000);
            nameOut = candidate;
        }
    }

    dirOut = dir;
    return true;
}

void DownloadsManager::persistRow(DownloadEntry *entry)
{
    if (!m_db || !m_db->isOpen())
        return;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "INSERT INTO downloads(file_name, source_url, destination, total_bytes, received_bytes, "
        "mime, start_time, end_time, state) VALUES(?, ?, ?, ?, ?, ?, ?, 0, 0)"));
    q.addBindValue(entry->fileName());
    q.addBindValue(entry->sourceUrl().toString());
    q.addBindValue(entry->destination());
    q.addBindValue(entry->totalBytes());
    q.addBindValue(entry->receivedBytes());
    q.addBindValue(entry->mimeType());
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!q.exec())
        qCWarning(lcDb) << "download persist failed:" << q.lastError().text();
}

void DownloadsManager::updateRow(DownloadEntry *entry)
{
    if (!m_db || !m_db->isOpen() || !entry->isPersistent())
        return;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "UPDATE downloads SET received_bytes = ?, total_bytes = ?, state = ?, end_time = ? "
        "WHERE source_url = ? AND file_name = ?"));
    q.addBindValue(entry->receivedBytes());
    q.addBindValue(entry->totalBytes());
    q.addBindValue(int(entry->state()));
    const bool finished = entry->state() == DownloadEntry::Completed
                          || entry->state() == DownloadEntry::Cancelled
                          || entry->state() == DownloadEntry::Interrupted;
    q.addBindValue(finished ? QDateTime::currentSecsSinceEpoch() : 0);
    q.addBindValue(entry->sourceUrl().toString());
    q.addBindValue(entry->fileName());
    q.exec();
}

int DownloadsManager::activeCount() const
{
    int n = 0;
    for (DownloadEntry *e : m_entries) {
        if (e->state() == DownloadEntry::InProgress || e->state() == DownloadEntry::Paused)
            ++n;
    }
    return n;
}

void DownloadsManager::removeEntry(DownloadEntry *entry)
{
    if (!entry)
        return;
    m_entries.removeAll(entry);
    entry->deleteLater();
    emit entriesChanged();
}

void DownloadsManager::clearCompleted()
{
    const QList<DownloadEntry *> copy = m_entries;
    for (DownloadEntry *e : copy) {
        if (e->state() == DownloadEntry::Completed
            || e->state() == DownloadEntry::Cancelled
            || e->state() == DownloadEntry::Interrupted) {
            m_entries.removeAll(e);
            e->deleteLater();
        }
    }
    emit entriesChanged();
}

void DownloadsManager::cancelAll()
{
    for (DownloadEntry *e : m_entries) {
        if (e->state() == DownloadEntry::InProgress || e->state() == DownloadEntry::Paused)
            e->cancel();
    }
}

} // namespace Polecat

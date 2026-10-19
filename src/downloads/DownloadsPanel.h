#pragma once

#include <QWidget>

namespace Polecat {

class DownloadsManager;
class DownloadEntry;
class QListWidget;

// DownloadsPanel shows the download list with progress, speed and actions
// (pause, resume, cancel, open, open folder, remove, clear completed).
class DownloadsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadsPanel(DownloadsManager *manager, QWidget *parent = nullptr);

    void reload();

private slots:
    void onEntriesChanged();
    void showContextMenu(const QPoint &pos);
    void onItemActivated(QListWidget *list, class QListWidgetItem *item);

private:
    QString describe(DownloadEntry *entry) const;

    DownloadsManager *m_manager = nullptr;
    QListWidget *m_list = nullptr;
};

} // namespace Polecat

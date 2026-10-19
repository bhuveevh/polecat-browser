#include "downloads/DownloadsPanel.h"

#include "downloads/DownloadsManager.h"
#include "utils/IconLoader.h"
#include "utils/UrlUtils.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace Polecat {

DownloadsPanel::DownloadsPanel(DownloadsManager *manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    auto *headerRow = new QHBoxLayout();
    auto *title = new QLabel(tr("Downloads"), this);
    title->setObjectName(QStringLiteral("sidebarTitle"));
    auto *clearButton = new QToolButton(this);
    clearButton->setText(tr("Clear completed"));
    clearButton->setToolTip(tr("Remove finished downloads from the list"));
    clearButton->setAutoRaise(true);
    headerRow->addWidget(title, 1);
    headerRow->addWidget(clearButton, 0);

    m_list = new QListWidget(this);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setAccessibleName(tr("Downloads"));

    layout->addLayout(headerRow);
    layout->addWidget(m_list, 1);

    connect(clearButton, &QToolButton::clicked, this, [this]() {
        if (m_manager)
            m_manager->clearCompleted();
    });
    connect(m_list, &QListWidget::customContextMenuRequested, this, &DownloadsPanel::showContextMenu);
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        onItemActivated(m_list, item);
    });

    if (m_manager)
        connect(m_manager, &DownloadsManager::entriesChanged, this, &DownloadsPanel::onEntriesChanged);

    reload();
}

QString DownloadsPanel::describe(DownloadEntry *entry) const
{
    if (!entry)
        return QString();
    const int pct = entry->progressPercent();
    QString status;
    switch (entry->state()) {
    case DownloadEntry::InProgress:
        status = pct >= 0 ? tr("%1%").arg(pct) : tr("Downloading…");
        if (entry->speedBytesPerSecond() > 0)
            status += tr("  ·  %1/s").arg(QLocale().formattedDataSize(qint64(entry->speedBytesPerSecond())));
        break;
    case DownloadEntry::Paused:      status = tr("Paused"); break;
    case DownloadEntry::Completed:   status = tr("Completed"); break;
    case DownloadEntry::Cancelled:   status = tr("Cancelled"); break;
    case DownloadEntry::Interrupted: status = tr("Interrupted"); break;
    }

    const QString size = entry->totalBytes() > 0
        ? QLocale().formattedDataSize(entry->totalBytes())
        : QLocale().formattedDataSize(entry->receivedBytes());

    return QStringLiteral("%1\n%2  ·  %3")
        .arg(entry->fileName(), status, size);
}

void DownloadsPanel::reload()
{
    if (!m_manager)
        return;
    m_list->clear();
    for (DownloadEntry *entry : m_manager->entries()) {
        auto *item = new QListWidgetItem(describe(entry), m_list);
        item->setIcon(IconLoader::icon(QStringLiteral("downloads")));
        item->setData(Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(entry)));
        item->setToolTip(entry->destination());
    }
}

void DownloadsPanel::onEntriesChanged()
{
    reload();
}

void DownloadsPanel::onItemActivated(QListWidget *list, QListWidgetItem *item)
{
    Q_UNUSED(list);
    if (!item)
        return;
    auto *entry = reinterpret_cast<DownloadEntry *>(item->data(Qt::UserRole).value<quintptr>());
    if (entry && entry->state() == DownloadEntry::Completed)
        entry->openFile();
}

void DownloadsPanel::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item || !m_manager)
        return;
    auto *entry = reinterpret_cast<DownloadEntry *>(item->data(Qt::UserRole).value<quintptr>());
    if (!entry)
        return;

    QMenu menu(this);
    QAction *open = entry->state() == DownloadEntry::Completed ? menu.addAction(tr("Open")) : nullptr;
    QAction *openFolder = entry->state() == DownloadEntry::Completed ? menu.addAction(tr("Open containing folder")) : nullptr;
    if (open)
        menu.addSeparator();
    QAction *pause = entry->state() == DownloadEntry::InProgress ? menu.addAction(tr("Pause")) : nullptr;
    QAction *resume = entry->state() == DownloadEntry::Paused ? menu.addAction(tr("Resume")) : nullptr;
    QAction *cancel = (entry->state() == DownloadEntry::InProgress || entry->state() == DownloadEntry::Paused)
                          ? menu.addAction(tr("Cancel")) : nullptr;
    menu.addSeparator();
    QAction *remove = menu.addAction(tr("Remove from list"));

    QAction *chosen = menu.exec(m_list->mapToGlobal(pos));
    if (!chosen)
        return;
    if (open && chosen == open)              entry->openFile();
    else if (openFolder && chosen == openFolder) entry->openContainingFolder();
    else if (pause && chosen == pause)       entry->pause();
    else if (resume && chosen == resume)     entry->resume();
    else if (cancel && chosen == cancel)     entry->cancel();
    else if (chosen == remove)               m_manager->removeEntry(entry);
}

} // namespace Polecat

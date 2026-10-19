#include "history/HistoryPanel.h"

#include "history/HistoryManager.h"
#include "utils/IconLoader.h"
#include "utils/UrlUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

namespace Polecat {

HistoryPanel::HistoryPanel(HistoryManager *manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search history"));
    m_search->setClearButtonEnabled(true);

    m_list = new QListWidget(this);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setAccessibleName(tr("Browsing history"));

    layout->addWidget(m_search);
    layout->addWidget(m_list, 1);

    connect(m_search, &QLineEdit::textChanged, this, &HistoryPanel::onSearchChanged);
    connect(m_list, &QListWidget::itemActivated, this, &HistoryPanel::onItemActivated);
    connect(m_list, &QListWidget::customContextMenuRequested, this, &HistoryPanel::showContextMenu);

    if (m_manager)
        connect(m_manager, &HistoryManager::changed, this, &HistoryPanel::reload);

    reload();
}

void HistoryPanel::reload()
{
    if (!m_manager)
        return;
    m_list->clear();

    const QString query = m_search->text().trimmed();
    const QList<HistoryEntry> entries = query.isEmpty()
                                            ? m_manager->recent(300)
                                            : m_manager->search(query, 500);

    QString lastDay;
    for (const HistoryEntry &e : entries) {
        const QDateTime dt = QDateTime::fromSecsSinceEpoch(e.lastVisit);
        const QString day = QLocale::system().toString(dt.date(), QLocale::LongFormat);
        if (day != lastDay) {
            auto *header = new QListWidgetItem(day, m_list);
            header->setFlags(Qt::NoItemFlags);
            header->setForeground(QApplication::palette().color(QPalette::PlaceholderText));
            lastDay = day;
        }
        const QString label = QStringLiteral("%1\n%2  ·  %3")
                                  .arg(e.title.isEmpty() ? e.url.host() : e.title,
                                       QLocale::system().toString(dt.time(), QLocale::ShortFormat),
                                       UrlUtils::displayString(e.url));
        auto *item = new QListWidgetItem(label, m_list);
        item->setIcon(IconLoader::icon(QStringLiteral("history")));
        item->setData(Qt::UserRole, e.url.toString());
        item->setData(Qt::UserRole + 1, e.id);
        item->setToolTip(e.url.toString());
    }
}

void HistoryPanel::onSearchChanged(const QString &text)
{
    Q_UNUSED(text);
    reload();
}

void HistoryPanel::onItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const QString url = item->data(Qt::UserRole).toString();
    if (!url.isEmpty())
        emit openUrl(QUrl(url));
}

void HistoryPanel::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item || !m_manager)
        return;
    const QString url = item->data(Qt::UserRole).toString();
    if (url.isEmpty())
        return;
    const qint64 id = item->data(Qt::UserRole + 1).toLongLong();

    QMenu menu(this);
    QAction *open = menu.addAction(tr("Open"));
    QAction *openNew = menu.addAction(tr("Open in new tab"));
    QAction *copyUrl = menu.addAction(tr("Copy link"));
    menu.addSeparator();
    QAction *remove = menu.addAction(tr("Delete from history"));
    menu.addSeparator();
    QAction *clearAll = menu.addAction(tr("Clear all history…"));

    QAction *chosen = menu.exec(m_list->mapToGlobal(pos));
    if (!chosen)
        return;
    if (chosen == open)
        emit openUrl(QUrl(url));
    else if (chosen == openNew)
        emit openUrl(QUrl(url));
    else if (chosen == copyUrl)
        QApplication::clipboard()->setText(url);
    else if (chosen == remove)
        m_manager->removeEntry(id);
    else if (chosen == clearAll)
        clearAllHistory();
}

void HistoryPanel::clearAllHistory()
{
    const auto choice = QMessageBox::question(
        this, tr("Clear all history?"),
        tr("This removes all browsing history from this device. This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (choice == QMessageBox::Yes && m_manager)
        m_manager->clearAll();
}

} // namespace Polecat

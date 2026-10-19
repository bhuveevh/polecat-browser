#pragma once

#include <QWidget>
#include <QUrl>

class QLineEdit;
class QListWidget;
class QListWidgetItem;

namespace Polecat {

class HistoryManager;

// HistoryPanel lists browsing history grouped by day, with search, open and
// delete. It never shows private-window activity because none is stored.
class HistoryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPanel(HistoryManager *manager, QWidget *parent = nullptr);

    void reload();

signals:
    void openUrl(const QUrl &url);

private slots:
    void onSearchChanged(const QString &text);
    void onItemActivated(QListWidgetItem *item);
    void showContextMenu(const QPoint &pos);
    void clearAllHistory();

private:
    HistoryManager *m_manager = nullptr;
    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
};

} // namespace Polecat

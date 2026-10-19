#pragma once

#include <QToolBar>
#include <QUrl>

class QAction;

namespace Polecat {

class AddressBar;

// NavigationToolBar holds Back / Forward / Reload-Stop, the AddressBar, the
// bookmark star, the reader toggle and the main menu button.
class NavigationToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit NavigationToolBar(QWidget *parent = nullptr);

    AddressBar *addressBar() const { return m_address; }

    void setBackEnabled(bool on);
    void setForwardEnabled(bool on);
    void setLoading(bool loading);
    void setBookmarked(bool on);
    void setReaderAvailable(bool on);

signals:
    void backRequested();
    void forwardRequested();
    void reloadRequested();
    void stopRequested();
    void homeRequested();
    void bookmarkRequested();
    void readerRequested();
    void menuRequested();
    void submitRequested(const QString &text);

private:
    AddressBar *m_address = nullptr;
    QAction *m_back = nullptr;
    QAction *m_forward = nullptr;
    QAction *m_reload = nullptr;
    QAction *m_home = nullptr;
    QAction *m_bookmark = nullptr;
    QAction *m_reader = nullptr;
    QAction *m_menu = nullptr;
};

} // namespace Polecat

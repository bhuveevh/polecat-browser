#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;
class QWebEngineView;

namespace Polecat {

// FindBar is the in-page find overlay. It drives QWebEnginePage::findText and
// reports the match count back to the user.
class FindBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindBar(QWidget *parent = nullptr);

    void setTarget(QWebEngineView *view);
    void activate();
    void deactivate();

signals:
    void closeRequested();

private slots:
    void findNext();
    void findPrevious();
    void onTextChanged(const QString &text);

private:
    void doFind(bool forward);

    QLineEdit *m_field = nullptr;
    QLabel *m_status = nullptr;
    QWebEngineView *m_view = nullptr;
    bool m_caseSensitive = false;
};

} // namespace Polecat

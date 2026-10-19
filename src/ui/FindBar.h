#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;
class QShowEvent;
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

    // Places the bar as a floating card at the top-right of the window.
    void repositionToParent(int topOffset);

signals:
    void closeRequested();

private slots:
    void findNext();
    void findPrevious();
    void onTextChanged(const QString &text);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void doFind(bool forward);
    int m_topOffset = 8;

    QLineEdit *m_field = nullptr;
    QLabel *m_status = nullptr;
    QWebEngineView *m_view = nullptr;
    bool m_caseSensitive = false;
};

} // namespace Polecat

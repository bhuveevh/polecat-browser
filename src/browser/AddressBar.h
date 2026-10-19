#pragma once

#include <QLineEdit>
#include <QUrl>

class QAction;
class QToolButton;
class QProgressBar;

namespace Polecat {

// AddressBar is the combined URL / search field.
//
// Behaviour:
//  * Shows a friendly, scheme-stripped URL when unfocused and the full URL
//    (selected) when focused, so Ctrl+L gives you something you can retype.
//  * Displays a security indicator (secure / insecure / internal).
//  * Shows load progress subtly via a thin underline.
//  * Emits submitRequested() with the raw typed text; the window decides
//    whether it is a URL or a search query.
class AddressBar : public QLineEdit
{
    Q_OBJECT

public:
    explicit AddressBar(QWidget *parent = nullptr);

    void setCurrentUrl(const QUrl &url);
    QUrl currentUrl() const { return m_url; }

    void setSecurityState(bool secure, bool internal);
    void setLoadProgress(int percent);      // 0..100, or -1 to hide
    void selectAllForEditing();

    // Shown in the site-info popup.
    bool isSecure() const { return m_secure; }
    bool isInternal() const { return m_internal; }

signals:
    void submitRequested(const QString &text);
    // Emitted when the user clicks the leading site-security indicator.
    void siteInfoRequested();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void refreshDisplay();

    QUrl m_url;
    QAction *m_securityAction = nullptr;
    QProgressBar *m_progress = nullptr;
    bool m_secure = false;
    bool m_internal = false;
    bool m_editing = false;
};

} // namespace Polecat

#include "browser/AddressBar.h"

#include "utils/IconLoader.h"
#include "utils/UrlUtils.h"

#include <QAction>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QProgressBar>
#include <QResizeEvent>
#include <QTimer>

namespace Polecat {

AddressBar::AddressBar(QWidget *parent)
    : QLineEdit(parent)
{
    setClearButtonEnabled(true);
    setPlaceholderText(tr("Search anything with Polecat Browser…   Type Here"));
    setAccessibleName(tr("Address and search bar"));
    setMinimumWidth(240);

    // Leading site-security indicator (lock / not-secure), like Chrome/Firefox.
    m_securityAction = addAction(IconLoader::icon(QStringLiteral("insecure")), QLineEdit::LeadingPosition);
    m_securityAction->setToolTip(tr("Site information"));
    connect(m_securityAction, &QAction::triggered, this, &AddressBar::siteInfoRequested);

    // A thin progress underline sits at the bottom edge of the field.
    m_progress = new QProgressBar(this);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(2);
    m_progress->setRange(0, 100);
    m_progress->hide();
    m_progress->setObjectName(QStringLiteral("addressProgress"));

    connect(this, &QLineEdit::returnPressed, this, [this]() {
        const QString text = this->text().trimmed();
        if (text.isEmpty())
            return;
        emit submitRequested(text);
        clearFocus();
    });
}

void AddressBar::setCurrentUrl(const QUrl &url)
{
    m_url = url;
    m_internal = url.scheme() == QLatin1String("about");
    if (!m_editing)
        refreshDisplay();
}

void AddressBar::setSecurityState(bool secure, bool internal)
{
    m_secure = secure;
    m_internal = internal;
    if (internal) {
        m_securityAction->setIcon(IconLoader::icon(QStringLiteral("info")));
        m_securityAction->setToolTip(tr("Polecat internal page"));
    } else if (secure) {
        m_securityAction->setIcon(IconLoader::icon(QStringLiteral("secure")));
        m_securityAction->setToolTip(tr("Secure connection — click for site information"));
    } else {
        m_securityAction->setIcon(IconLoader::icon(QStringLiteral("insecure")));
        m_securityAction->setToolTip(tr("Not secure — click for site information"));
    }
}

void AddressBar::setLoadProgress(int percent)
{
    if (percent < 0 || percent >= 100) {
        m_progress->hide();
        return;
    }
    m_progress->setValue(percent);
    m_progress->setGeometry(0, height() - 2, width(), 2);
    m_progress->show();
    m_progress->raise();
}

void AddressBar::refreshDisplay()
{
    if (m_editing)
        return;
    if (m_url.isEmpty() || m_url.toString() == QLatin1String("about:blank"))
        setText(QString());
    else
        setText(UrlUtils::displayString(m_url));
}

void AddressBar::selectAllForEditing()
{
    setFocus(Qt::ShortcutFocusReason);
    selectAll();
}

void AddressBar::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    m_editing = true;
    // Reveal the full URL for editing.
    if (!m_url.isEmpty())
        setText(m_url.toString());
    QTimer::singleShot(0, this, [this]() { selectAll(); });
}

void AddressBar::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);
    m_editing = false;
    refreshDisplay();
}

void AddressBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        m_editing = false;
        refreshDisplay();
        clearFocus();
        return;
    }
    QLineEdit::keyPressEvent(event);
}

void AddressBar::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    if (m_progress->isVisible())
        m_progress->setGeometry(0, height() - 2, width(), 2);
}

} // namespace Polecat

#include "tabs/BrowserTab.h"

#include "browser/WebPage.h"
#include "browser/WebView.h"
#include "utils/UrlUtils.h"

#include <QWebEngineProfile>

namespace Polecat {

BrowserTab::BrowserTab(int id,
                       QWebEngineProfile *profile,
                       SettingsManager *settings,
                       bool isPrivate,
                       QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_private(isPrivate)
    , m_createdAt(QDateTime::currentDateTime())
    , m_lastActiveAt(m_createdAt)
{
    m_view = new WebView(profile, settings, isPrivate);
    m_view->setObjectName(QStringLiteral("webView_%1").arg(id));

    connect(m_view, &QWebEngineView::titleChanged, this, &BrowserTab::onTitleChanged);
    connect(m_view, &QWebEngineView::urlChanged, this, &BrowserTab::onUrlChanged);
    connect(m_view, &QWebEngineView::iconChanged, this, &BrowserTab::onIconChanged);
    connect(m_view, &QWebEngineView::loadStarted, this, &BrowserTab::onLoadStarted);
    connect(m_view, &QWebEngineView::loadFinished, this, &BrowserTab::onLoadFinished);
    connect(m_view, &QWebEngineView::audioMutedChanged, this, &BrowserTab::onAudioMutedChanged);

    m_url = m_view->url();
    m_title = tr("New Tab");
}

BrowserTab::~BrowserTab()
{
    // A view still parented to the content stack is owned by that widget and
    // will be destroyed with it. A view detached by TabManager::closeTab()
    // (removeWidget) has no parent, so we free it here to release the renderer
    // and its memory. m_view is a QPointer, so it is already null if the stack
    // destroyed the view first.
    if (m_view && !m_view->parent())
        m_view->deleteLater();
}

WebPage *BrowserTab::page() const
{
    return m_view ? m_view->webPage() : nullptr;
}

qreal BrowserTab::zoomFactor() const
{
    return m_view ? m_view->zoomFactor() : 1.0;
}

void BrowserTab::setPinned(bool pinned)
{
    if (m_pinned == pinned)
        return;
    m_pinned = pinned;
    emit pinnedChanged(pinned);
}

void BrowserTab::setMuted(bool muted)
{
    if (!m_view)
        return;
    m_view->page()->setAudioMuted(muted);
}

void BrowserTab::markActive()
{
    m_lastActiveAt = QDateTime::currentDateTime();
}

void BrowserTab::setZoomFactor(qreal factor)
{
    if (!m_view)
        return;
    factor = qBound(0.25, factor, 5.0);
    m_view->setZoomFactor(factor);
    emit zoomChanged(factor);
}

QString BrowserTab::displayTitle() const
{
    if (!m_title.trimmed().isEmpty())
        return m_title.trimmed();
    if (m_url.isValid() && !m_url.isEmpty())
        return UrlUtils::displayString(m_url);
    return tr("New Tab");
}

void BrowserTab::onTitleChanged(const QString &title)
{
    m_title = title;
    emit titleChanged(title);
}

void BrowserTab::onUrlChanged(const QUrl &url)
{
    m_url = url;
    emit urlChanged(url);
}

void BrowserTab::onIconChanged(const QIcon &icon)
{
    if (icon.isNull())
        return;
    m_favicon = icon;
    emit faviconChanged(icon);
}

void BrowserTab::onLoadStarted()
{
    m_loading = true;
    emit loadingChanged(true);
}

void BrowserTab::onLoadFinished(bool ok)
{
    m_loading = false;
    emit loadingChanged(false);
    if (ok && m_view) {
        const QIcon ic = m_view->icon();
        if (!ic.isNull()) {
            m_favicon = ic;
            emit faviconChanged(ic);
        }
    }
}

void BrowserTab::onAudioMutedChanged(bool muted)
{
    m_muted = muted;
    emit mutedChanged(muted);
}

} // namespace Polecat

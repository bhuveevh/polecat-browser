#pragma once

#include <QObject>
#include <QUrl>
#include <QIcon>
#include <QDateTime>
#include <QPointer>

class QWebEngineProfile;

namespace Polecat {

class SettingsManager;
class WebView;
class WebPage;

// BrowserTab wraps a single WebView plus the metadata the UI needs. It is a
// QObject (not a widget) so the tab can be reasoned about and tested without a
// visible window; the actual widget is exposed via view().
class BrowserTab : public QObject
{
    Q_OBJECT

public:
    BrowserTab(int id,
               QWebEngineProfile *profile,
               SettingsManager *settings,
               bool isPrivate,
               QObject *parent = nullptr);
    ~BrowserTab() override;

    int id() const { return m_id; }
    WebView *view() const { return m_view; }
    WebPage *page() const;
    bool isPrivate() const { return m_private; }

    QString title() const { return m_title; }
    QUrl url() const { return m_url; }
    QIcon favicon() const { return m_favicon; }
    bool isLoading() const { return m_loading; }
    bool isMuted() const { return m_muted; }
    bool isPinned() const { return m_pinned; }
    qreal zoomFactor() const;
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime lastActiveAt() const { return m_lastActiveAt; }

    void setPinned(bool pinned);
    void setMuted(bool muted);
    void markActive();
    void setZoomFactor(qreal factor);

    // Returns a stable, human title used in the tab bar and window title.
    QString displayTitle() const;

signals:
    void titleChanged(const QString &title);
    void urlChanged(const QUrl &url);
    void faviconChanged(const QIcon &icon);
    void loadingChanged(bool loading);
    void mutedChanged(bool muted);
    void pinnedChanged(bool pinned);
    void zoomChanged(qreal factor);

private slots:
    void onTitleChanged(const QString &title);
    void onUrlChanged(const QUrl &url);
    void onIconChanged(const QIcon &icon);
    void onLoadStarted();
    void onLoadFinished(bool ok);
    void onAudioMutedChanged(bool muted);

private:
    int m_id;
    QPointer<WebView> m_view;
    bool m_private = false;

    QString m_title;
    QUrl m_url;
    QIcon m_favicon;
    bool m_loading = false;
    bool m_muted = false;
    bool m_pinned = false;

    QDateTime m_createdAt;
    QDateTime m_lastActiveAt;
};

} // namespace Polecat

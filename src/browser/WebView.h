#pragma once

#include <QWebEngineView>
#include <QUrl>
#include <QPoint>

class QWebEngineProfile;
class QContextMenuEvent;

namespace Polecat {

class SettingsManager;
class WebPage;

// WebView is a thin QWebEngineView subclass that owns a WebPage, forwards the
// context-menu request to the window (so study/privacy actions can be added),
// and turns renderer crashes into a friendly, recoverable error page.
class WebView : public QWebEngineView
{
    Q_OBJECT

public:
    WebView(QWebEngineProfile *profile,
            SettingsManager *settings,
            bool isPrivate,
            QWidget *parent = nullptr);

    WebPage *webPage() const { return m_page; }
    bool isPrivate() const { return m_private; }

    // Loads a URL, handling Polecat's internal "about:" pages (about:newtab,
    // about:private, about:blank) by rendering the bundled HTML for them.
    void loadUrl(const QUrl &url);

signals:
    // Emitted instead of showing a menu, so the window can compose one.
    void contextMenuRequested(const QPoint &globalPos, const QPoint &viewPos);
    void renderProcessCrashed();
    void renderProcessUnresponsive();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void showCrashPage(const QString &detail);

    WebPage *m_page = nullptr;
    bool m_private = false;
};

} // namespace Polecat

#pragma once

#include <QWebEngineView>
#include <QUrl>
#include <QPoint>

class QWebEngineProfile;
class QContextMenuEvent;
class QMenu;
class QWebEngineContextMenuRequest;

namespace Polecat {

class SettingsManager;
class BookmarkManager;
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

    // Used to populate the New Tab page's bookmark grid.
    void setBookmarks(BookmarkManager *bookmarks) { m_bookmarks = bookmarks; }

    // Loads a URL, handling Polecat's internal "about:" pages (about:newtab,
    // about:settings, about:private, about:blank) by rendering the bundled HTML
    // for them.
    void loadUrl(const QUrl &url);

signals:
    // Emitted with a fully-built standard context menu (and the request that
    // describes what was clicked) so the window can extend it before showing it.
    // The receiver takes ownership of the menu.
    void contextMenuRequested(QMenu *menu,
                              QWebEngineContextMenuRequest *request,
                              const QPoint &globalPos);
    void renderProcessCrashed();
    void renderProcessUnresponsive();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void showCrashPage(const QString &detail);
    QString buildSettingsHtml() const;
    QString buildNewTabHtml() const;

    WebPage *m_page = nullptr;
    SettingsManager *m_settings = nullptr;
    BookmarkManager *m_bookmarks = nullptr;
    bool m_private = false;
};

} // namespace Polecat

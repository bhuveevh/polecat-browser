#include "browser/WebView.h"

#include "bookmarks/BookmarkManager.h"
#include "browser/WebPage.h"
#include "settings/SettingsManager.h"
#include "utils/Logging.h"
#include "utils/Paths.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMenu>
#include <QPalette>
#include <QRegularExpression>
#include <QVariant>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace Polecat {

WebView::WebView(QWebEngineProfile *profile,
                 SettingsManager *settings,
                 bool isPrivate,
                 QWidget *parent)
    : QWebEngineView(parent)
    , m_settings(settings)
    , m_private(isPrivate)
{
    m_page = new WebPage(profile, settings, this);
    m_page->setPrivate(isPrivate);
    setPage(m_page);

    // Keep the view responsive and avoid surprising scroll animations.
    if (auto *s = m_page->settings()) {
        s->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    }

    connect(m_page, &QWebEnginePage::renderProcessTerminated, this,
            [this](QWebEnginePage::RenderProcessTerminationStatus status, int exitCode) {
                if (status == QWebEnginePage::NormalTerminationStatus)
                    return;
                emit renderProcessCrashed();
                showCrashPage(tr("The web page stopped unexpectedly (status %1, code %2).")
                                  .arg(int(status)).arg(exitCode));
            });
}

void WebView::loadUrl(const QUrl &url)
{
    // Internal pages are rendered from bundled HTML so they work offline and
    // never touch the network.
    if (url.scheme() == QLatin1String("about")) {
        const QString page = url.path().toLower();
        QString resource;
        if (page == QLatin1String("private"))
            resource = QStringLiteral(":/html/privatenote.html");

        if (page == QLatin1String("newtab")) {
            const QString html = buildNewTabHtml();
            if (!html.isEmpty()) {
                setHtml(html, url);
                return;
            }
            resource = QStringLiteral(":/html/newtab.html");
        }

        if (page == QLatin1String("settings")) {
            const QString html = buildSettingsHtml();
            if (!html.isEmpty()) {
                setHtml(html, url);
                return;
            }
        }

        if (!resource.isEmpty()) {
            QFile f(resource);
            if (f.open(QIODevice::ReadOnly)) {
                setHtml(QString::fromUtf8(f.readAll()), url);
                return;
            }
        }
        if (page == QLatin1String("blank")) {
            setHtml(QStringLiteral("<html><body></body></html>"), url);
            return;
        }
    }
    setUrl(url);
}

QString WebView::buildNewTabHtml() const
{
    QFile f(QStringLiteral(":/html/newtab.html"));
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    QString html = QString::fromUtf8(f.readAll());

    // Wallpaper (bundled SVG, inlined so the page needs no extra requests).
    QString wallpaper;
    QFile w(QStringLiteral(":/wallpaper.svg"));
    if (w.open(QIODevice::ReadOnly)) {
        wallpaper = QString::fromUtf8(w.readAll());
        wallpaper.remove(QRegularExpression(QStringLiteral("<\\?xml[^>]*\\?>")));
        wallpaper.replace(QStringLiteral("class=\"light\""), QString());
    }

    // Bookmarks: top level, plus one level inside folders.
    QJsonArray arr;
    auto addBookmark = [&arr](const Bookmark &b) {
        QJsonObject o;
        o.insert(QStringLiteral("title"), b.title.isEmpty() ? b.url.host() : b.title);
        o.insert(QStringLiteral("url"), b.url.toString());
        o.insert(QStringLiteral("favicon"), b.favicon);
        arr.append(o);
    };
    if (m_bookmarks) {
        for (const Bookmark &b : m_bookmarks->children(-1)) {
            if (b.isFolder()) {
                for (const Bookmark &k : m_bookmarks->children(b.id))
                    addBookmark(k);
            } else {
                addBookmark(b);
            }
        }
    }

    // User name, as a JSON string literal.
    QJsonArray tmp;
    tmp.append(Paths::userDisplayName());
    QString userJson = QString::fromUtf8(QJsonDocument(tmp).toJson(QJsonDocument::Compact));
    userJson = userJson.mid(1, userJson.length() - 2); // strip the surrounding []

    // Dark mode follows the palette the theme manager applied.
    const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;

    html.replace(QStringLiteral("__POLECAT_WALLPAPER__"), wallpaper);
    html.replace(QStringLiteral("__POLECAT_USER__"), userJson);
    html.replace(QStringLiteral("__POLECAT_BOOKMARKS__"),
                 QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    html.replace(QStringLiteral("__POLECAT_DARK__"), dark ? QStringLiteral("true")
                                                          : QStringLiteral("false"));
    return html;
}

QString WebView::buildSettingsHtml() const
{
    QFile f(QStringLiteral(":/html/settings.html"));
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    QString html = QString::fromUtf8(f.readAll());

    static const char *const keys[] = {
        "appearance/theme", "appearance/nativeTitleBar",
        "general/startup", "general/homePage", "general/searchEngine",
        "privacy/blockAdsTrackers", "privacy/blockThirdPartyCookies", "privacy/httpsOnly",
        "privacy/doNotTrack", "privacy/javascriptEnabled", "privacy/imagesEnabled",
        "privacy/popupBlocking",
        "downloads/askWhereToSave", "downloads/defaultDir",
        "tabs/showTabBar", "tabs/restoreSession",
        "advanced/devTools", "advanced/verboseLogging",
    };

    QJsonObject obj;
    for (const char *const k : keys) {
        const QString key = QString::fromLatin1(k);
        const QVariant v = m_settings ? m_settings->value(key) : QVariant();
        obj.insert(key, QJsonValue::fromVariant(v));
    }
    const QString json = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    html.replace(QStringLiteral("__POLECAT_STATE_JSON__"), json);
    return html;
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    // Let Qt WebEngine build its element-aware standard menu (Back, Reload,
    // Save page, Open link in new tab, Save image, Copy, View source, Inspect…),
    // then hand it to the window to extend before it is shown.
    QMenu *menu = createStandardContextMenu();
    if (!menu) {
        QWebEngineView::contextMenuEvent(event);
        return;
    }
    // lastContextMenuRequest() is only valid during this call.
    emit contextMenuRequested(menu, lastContextMenuRequest(), event->globalPos());
    event->accept();
}

void WebView::showCrashPage(const QString &detail)
{
    qCWarning(lcNet) << "Renderer crash:" << detail;
    const QString html = QStringLiteral(
        "<html><head><meta charset='utf-8'><style>"
        "body{font-family:system-ui,sans-serif;background:#1e1e1e;color:#f5f5f5;"
        "display:flex;align-items:center;justify-content:center;height:100vh;margin:0}"
        ".card{max-width:520px;text-align:center;padding:32px}"
        "h1{font-size:20px;font-weight:600}"
        "p{color:#b8b8b8;line-height:1.5}"
        ".muted{font-size:12px;color:#888;margin-top:16px}"
        "</style></head><body><div class='card'>"
        "<h1>%1</h1><p>%2</p><p class='muted'>%3</p>"
        "</div></body></html>")
        .arg(tr("This page stopped working"),
             tr("Something went wrong while displaying this page. Reloading the "
                "page usually fixes it."),
             detail.toHtmlEscaped());

    setHtml(html, QUrl(QStringLiteral("about:crash")));
}

} // namespace Polecat

#include "utils/IconLoader.h"

#include <QApplication>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyle>
#include <QSvgRenderer>

namespace {

// Logical name -> bundled Google "Material Symbols" icon (rounded style, 24x24
// SVG under :/icons/ms/).
const QHash<QString, QString> &materialRegistry()
{
    static const QHash<QString, QString> reg = {
        {QStringLiteral("back"),      QStringLiteral(":/icons/ms/arrow_back.svg")},
        {QStringLiteral("forward"),   QStringLiteral(":/icons/ms/arrow_forward.svg")},
        {QStringLiteral("reload"),    QStringLiteral(":/icons/ms/refresh.svg")},
        {QStringLiteral("stop"),      QStringLiteral(":/icons/ms/close.svg")},
        {QStringLiteral("home"),      QStringLiteral(":/icons/ms/home.svg")},
        {QStringLiteral("new-tab"),   QStringLiteral(":/icons/ms/add.svg")},
        {QStringLiteral("close"),     QStringLiteral(":/icons/ms/close.svg")},
        {QStringLiteral("bookmark"),  QStringLiteral(":/icons/ms/star.svg")},
        {QStringLiteral("bookmarked"),QStringLiteral(":/icons/ms/star_filled.svg")},
        {QStringLiteral("bookmarks"), QStringLiteral(":/icons/ms/bookmarks.svg")},
        {QStringLiteral("history"),   QStringLiteral(":/icons/ms/history.svg")},
        {QStringLiteral("downloads"), QStringLiteral(":/icons/ms/download.svg")},
        {QStringLiteral("menu"),      QStringLiteral(":/icons/ms/more_vert.svg")},
        {QStringLiteral("private"),   QStringLiteral(":/icons/ms/visibility_off.svg")},
        {QStringLiteral("reader"),    QStringLiteral(":/icons/ms/article.svg")},
        {QStringLiteral("find"),      QStringLiteral(":/icons/ms/search.svg")},
        {QStringLiteral("sidebar"),   QStringLiteral(":/icons/ms/view_sidebar.svg")},
        {QStringLiteral("secure"),    QStringLiteral(":/icons/ms/lock.svg")},
        {QStringLiteral("insecure"),  QStringLiteral(":/icons/ms/lock_open.svg")},
        {QStringLiteral("settings"),  QStringLiteral(":/icons/ms/settings.svg")},
        {QStringLiteral("profile"),   QStringLiteral(":/icons/ms/account_circle.svg")},
        {QStringLiteral("win-minimize"), QStringLiteral(":/icons/ms/remove.svg")},
        {QStringLiteral("win-maximize"), QStringLiteral(":/icons/ms/crop_square.svg")},
        {QStringLiteral("win-restore"),  QStringLiteral(":/icons/ms/filter_none.svg")},
        {QStringLiteral("zoom-in"),   QStringLiteral(":/icons/ms/zoom_in.svg")},
        {QStringLiteral("zoom-out"),  QStringLiteral(":/icons/ms/zoom_out.svg")},
        {QStringLiteral("print"),     QStringLiteral(":/icons/ms/print.svg")},
        {QStringLiteral("save"),      QStringLiteral(":/icons/ms/save.svg")},
        {QStringLiteral("note"),      QStringLiteral(":/icons/ms/edit_note.svg")},
        {QStringLiteral("translate"), QStringLiteral(":/icons/ms/translate.svg")},
        {QStringLiteral("fullscreen"),QStringLiteral(":/icons/ms/fullscreen.svg")},
        {QStringLiteral("code"),      QStringLiteral(":/icons/ms/code.svg")},
        {QStringLiteral("info"),      QStringLiteral(":/icons/ms/info.svg")},
        {QStringLiteral("new-window"),QStringLiteral(":/icons/ms/open_in_new.svg")},
        {QStringLiteral("restore-session"), QStringLiteral(":/icons/ms/history.svg")},
        {QStringLiteral("quit"),      QStringLiteral(":/icons/ms/logout.svg")},
        {QStringLiteral("copy"),      QStringLiteral(":/icons/ms/content_copy.svg")},
        {QStringLiteral("clear"),     QStringLiteral(":/icons/ms/delete.svg")},
    };
    return reg;
}

// Fallbacks to the platform style's own window buttons, used only if the
// bundled glyph fails to render so the controls are never invisible.
QStyle::StandardPixmap windowButtonFallback(const QString &name)
{
    if (name == QLatin1String("win-minimize"))
        return QStyle::SP_TitleBarMinButton;
    if (name == QLatin1String("win-maximize"))
        return QStyle::SP_TitleBarMaxButton;
    if (name == QLatin1String("win-restore"))
        return QStyle::SP_TitleBarNormalButton;
    if (name == QLatin1String("close"))
        return QStyle::SP_TitleBarCloseButton;
    return QStyle::SP_CustomBase;
}

// The colour toolbar icons should be drawn in, following the active palette so
// they read on both light and dark themes.
QColor iconColor()
{
    QColor c = QApplication::palette().color(QPalette::ButtonText);
    if (!c.isValid())
        c = QApplication::palette().color(QPalette::WindowText);
    if (!c.isValid())
        c = QColor(0x5f, 0x63, 0x68); // Material grey, a safe default.
    return c;
}

// Renders a bundled SVG with QSvgRenderer (so we do NOT depend on the Qt SVG
// *image plugin* being installed) and recolours it for normal/active/disabled
// states, at several sizes so the glyphs stay crisp.
QIcon tinted(const QString &resource, const QColor &color)
{
    QSvgRenderer renderer(resource);
    if (!renderer.isValid())
        return QIcon();

    auto recolor = [&renderer](int size, const QColor &c) {
        QPixmap pm(size, size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        renderer.render(&p);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(pm.rect(), c);
        p.end();
        return pm;
    };

    QColor disabled = color;
    disabled.setAlpha(110);

    QIcon icon;
    for (int size : {16, 20, 24, 32, 48, 64}) {
        icon.addPixmap(recolor(size, color), QIcon::Normal);
        icon.addPixmap(recolor(size, color), QIcon::Active);
        icon.addPixmap(recolor(size, disabled), QIcon::Disabled);
    }
    return icon;
}

} // namespace

namespace Polecat {
namespace IconLoader {

QIcon icon(const QString &name)
{
    const auto it = materialRegistry().constFind(name);
    if (it != materialRegistry().constEnd()) {
        QIcon material = tinted(it.value(), iconColor());
        if (!material.isNull() && !material.availableSizes().isEmpty())
            return material;
    }

    // Fall back to the platform style's own window buttons (so the title bar is
    // never left with invisible controls), then to the user's icon theme.
    const QStyle::StandardPixmap sp = windowButtonFallback(name);
    if (sp != QStyle::SP_CustomBase) {
        if (QStyle *style = qApp ? qApp->style() : nullptr) {
            QIcon fallback = style->standardIcon(sp);
            if (!fallback.isNull())
                return fallback;
        }
    }

    QIcon themed = QIcon::fromTheme(name);
    if (!themed.isNull() && !themed.availableSizes().isEmpty())
        return themed;
    return QIcon::fromTheme(name);
}

QIcon appIcon()
{
    QSvgRenderer renderer(QStringLiteral(":/icons/org.opentiaos.PolecatBrowser.svg"));
    if (renderer.isValid()) {
        QIcon icon;
        for (int size : {32, 64, 128, 256}) {
            QPixmap pm(size, size);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            renderer.render(&p);
            p.end();
            icon.addPixmap(pm);
        }
        return icon;
    }
    return QIcon::fromTheme(QStringLiteral("org.opentiaos.PolecatBrowser"));
}

} // namespace IconLoader
} // namespace Polecat

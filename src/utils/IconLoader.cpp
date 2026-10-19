#include "utils/IconLoader.h"

#include <QApplication>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyle>

namespace {

// Logical name -> bundled icon file. The files are Google "Material Symbols"
// (rounded) pre-rendered to PNG at build time, so displaying them needs no SVG
// plugin and cannot silently fail. They are stored white and tinted at runtime
// to follow the active light/dark theme.
const QHash<QString, QString> &registry()
{
    static const QHash<QString, QString> reg = {
        {QStringLiteral("back"),      QStringLiteral("arrow_back")},
        {QStringLiteral("forward"),   QStringLiteral("arrow_forward")},
        {QStringLiteral("reload"),    QStringLiteral("refresh")},
        {QStringLiteral("stop"),      QStringLiteral("close")},
        {QStringLiteral("home"),      QStringLiteral("home")},
        {QStringLiteral("new-tab"),   QStringLiteral("add")},
        {QStringLiteral("close"),     QStringLiteral("close")},
        {QStringLiteral("bookmark"),  QStringLiteral("star")},
        {QStringLiteral("bookmarked"),QStringLiteral("star_filled")},
        {QStringLiteral("bookmarks"), QStringLiteral("bookmarks")},
        {QStringLiteral("history"),   QStringLiteral("history")},
        {QStringLiteral("downloads"), QStringLiteral("download")},
        {QStringLiteral("menu"),      QStringLiteral("more_vert")},
        {QStringLiteral("private"),   QStringLiteral("visibility_off")},
        {QStringLiteral("reader"),    QStringLiteral("article")},
        {QStringLiteral("find"),      QStringLiteral("search")},
        {QStringLiteral("sidebar"),   QStringLiteral("view_sidebar")},
        {QStringLiteral("secure"),    QStringLiteral("lock")},
        {QStringLiteral("insecure"),  QStringLiteral("lock_open")},
        {QStringLiteral("settings"),  QStringLiteral("settings")},
        {QStringLiteral("profile"),   QStringLiteral("account_circle")},
        {QStringLiteral("win-minimize"), QStringLiteral("remove")},
        {QStringLiteral("win-maximize"), QStringLiteral("crop_square")},
        {QStringLiteral("win-restore"),  QStringLiteral("filter_none")},
        {QStringLiteral("zoom-in"),   QStringLiteral("zoom_in")},
        {QStringLiteral("zoom-out"),  QStringLiteral("zoom_out")},
        {QStringLiteral("print"),     QStringLiteral("print")},
        {QStringLiteral("save"),      QStringLiteral("save")},
        {QStringLiteral("note"),      QStringLiteral("edit_note")},
        {QStringLiteral("translate"), QStringLiteral("translate")},
        {QStringLiteral("fullscreen"),QStringLiteral("fullscreen")},
        {QStringLiteral("code"),      QStringLiteral("code")},
        {QStringLiteral("info"),      QStringLiteral("info")},
        {QStringLiteral("new-window"),QStringLiteral("open_in_new")},
        {QStringLiteral("quit"),      QStringLiteral("logout")},
        {QStringLiteral("copy"),      QStringLiteral("content_copy")},
        {QStringLiteral("clear"),     QStringLiteral("delete")},
    };
    return reg;
}

// Fallbacks to the platform style's own window buttons, used only if the
// bundled glyph is missing so the controls are never invisible.
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

QColor iconColor()
{
    QColor c = QApplication::palette().color(QPalette::ButtonText);
    if (!c.isValid())
        c = QApplication::palette().color(QPalette::WindowText);
    if (!c.isValid())
        c = QColor(0x5f, 0x63, 0x68);
    return c;
}

// Loads a bundled PNG and recolours it for normal/active/disabled states.
QIcon tinted(const QString &resource, const QColor &color)
{
    const QPixmap base(resource);
    if (base.isNull())
        return QIcon();

    auto recolor = [&base](const QColor &c) {
        QPixmap pm = base;
        QPainter p(&pm);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(pm.rect(), c);
        p.end();
        return pm;
    };

    QColor disabled = color;
    disabled.setAlpha(110);

    QIcon icon;
    icon.addPixmap(recolor(color), QIcon::Normal);
    icon.addPixmap(recolor(color), QIcon::Active);
    icon.addPixmap(recolor(disabled), QIcon::Disabled);
    return icon;
}

} // namespace

namespace Polecat {
namespace IconLoader {

QIcon icon(const QString &name)
{
    const auto it = registry().constFind(name);
    if (it != registry().constEnd()) {
        QIcon glyph = tinted(QStringLiteral(":/icons/png/") + it.value() + QStringLiteral(".png"),
                             iconColor());
        if (!glyph.isNull() && !glyph.availableSizes().isEmpty())
            return glyph;
    }

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
    const QPixmap pm(QStringLiteral(":/icons/png/brand.png"));
    if (!pm.isNull())
        return QIcon(pm);
    return QIcon::fromTheme(QStringLiteral("org.opentiaos.PolecatBrowser"));
}

} // namespace IconLoader
} // namespace Polecat

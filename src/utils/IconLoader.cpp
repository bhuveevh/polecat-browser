#include "utils/IconLoader.h"

#include <QApplication>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyle>

namespace {

// Logical name -> bundled Google "Material Symbols" icon (rounded style,
// 24x24 SVG under :/icons/ms/). These are the modern Material 3 glyphs and are
// used ahead of the user's icon theme so the toolbar has one consistent,
// up-to-date look.
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
    };
    return reg;
}

// Fallbacks to the platform style's own window buttons, used only if the
// bundled glyph fails to load so the controls are never invisible.
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

// Renders a bundled SVG and recolours it, producing normal/active/disabled
// variants so the icon follows the theme instead of being a fixed black glyph.
QIcon tinted(const QString &resource, const QColor &color)
{
    const QPixmap base = QIcon(resource).pixmap(QSize(64, 64));
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
    QIcon themed = QIcon::fromTheme(QStringLiteral("org.opentiaos.PolecatBrowser"));
    if (!themed.isNull() && !themed.availableSizes().isEmpty())
        return themed;
    return QIcon(QStringLiteral(":/icons/org.opentiaos.PolecatBrowser.svg"));
}

} // namespace IconLoader
} // namespace Polecat

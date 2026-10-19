#include "utils/IconLoader.h"

#include <QApplication>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QPalette>
#include <QPixmap>

namespace {

// Logical name -> bundled Google Material Design icon (24x24 SVG under :/icons/material/).
// These are used ahead of the user's icon theme so the toolbar has a single,
// modern, consistent look.
const QHash<QString, QString> &materialRegistry()
{
    static const QHash<QString, QString> reg = {
        {QStringLiteral("back"),      QStringLiteral(":/icons/material/arrow_back.svg")},
        {QStringLiteral("forward"),   QStringLiteral(":/icons/material/arrow_forward.svg")},
        {QStringLiteral("reload"),    QStringLiteral(":/icons/material/refresh.svg")},
        {QStringLiteral("stop"),      QStringLiteral(":/icons/material/close.svg")},
        {QStringLiteral("home"),      QStringLiteral(":/icons/material/home.svg")},
        {QStringLiteral("new-tab"),   QStringLiteral(":/icons/material/add.svg")},
        {QStringLiteral("close"),     QStringLiteral(":/icons/material/close.svg")},
        {QStringLiteral("bookmark"),  QStringLiteral(":/icons/material/star.svg")},
        {QStringLiteral("bookmarks"), QStringLiteral(":/icons/material/bookmarks.svg")},
        {QStringLiteral("history"),   QStringLiteral(":/icons/material/history.svg")},
        {QStringLiteral("downloads"), QStringLiteral(":/icons/material/download.svg")},
        {QStringLiteral("menu"),      QStringLiteral(":/icons/material/more_vert.svg")},
        {QStringLiteral("private"),   QStringLiteral(":/icons/material/visibility_off.svg")},
        {QStringLiteral("reader"),    QStringLiteral(":/icons/material/article.svg")},
        {QStringLiteral("find"),      QStringLiteral(":/icons/material/search.svg")},
        {QStringLiteral("sidebar"),   QStringLiteral(":/icons/material/view_sidebar.svg")},
        {QStringLiteral("secure"),    QStringLiteral(":/icons/material/lock.svg")},
        {QStringLiteral("insecure"),  QStringLiteral(":/icons/material/lock_open.svg")},
        {QStringLiteral("settings"),  QStringLiteral(":/icons/material/settings.svg")},
        {QStringLiteral("profile"),   QStringLiteral(":/icons/material/account_circle.svg")},
    };
    return reg;
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

    // Fall back to the user's icon theme, then to the name directly.
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

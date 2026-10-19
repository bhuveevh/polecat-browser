#include "utils/IconLoader.h"

#include <QIcon>
#include <QHash>
#include <QStringList>

namespace {
// Logical name -> (themed fallback names..., bundled resource name)
struct IconSpec {
    QStringList themeNames;
    QString resource;
};

const QHash<QString, IconSpec> &registry()
{
    static const QHash<QString, IconSpec> reg = {
        {QStringLiteral("back"),      {{QStringLiteral("go-previous"), QStringLiteral("go-previous-symbolic")}, QStringLiteral("icons/polecat-back.svg")}},
        {QStringLiteral("forward"),   {{QStringLiteral("go-next"), QStringLiteral("go-next-symbolic")},         QStringLiteral("icons/polecat-forward.svg")}},
        {QStringLiteral("reload"),    {{QStringLiteral("view-refresh"), QStringLiteral("view-refresh-symbolic")}, QStringLiteral("icons/polecat-reload.svg")}},
        {QStringLiteral("stop"),      {{QStringLiteral("process-stop"), QStringLiteral("process-stop-symbolic")}, QStringLiteral("icons/polecat-stop.svg")}},
        {QStringLiteral("home"),      {{QStringLiteral("go-home"), QStringLiteral("go-home-symbolic")},         QStringLiteral("icons/polecat-home.svg")}},
        {QStringLiteral("new-tab"),   {{QStringLiteral("tab-new"), QStringLiteral("list-add")},                 QStringLiteral("icons/polecat-new-tab.svg")}},
        {QStringLiteral("close"),     {{QStringLiteral("window-close"), QStringLiteral("window-close-symbolic")}, QStringLiteral("icons/polecat-close.svg")}},
        {QStringLiteral("bookmark"),  {{QStringLiteral("user-bookmarks"), QStringLiteral("starred")},           QStringLiteral("icons/polecat-bookmark.svg")}},
        {QStringLiteral("bookmarks"), {{QStringLiteral("user-bookmarks")},                                      QStringLiteral("icons/polecat-bookmarks.svg")}},
        {QStringLiteral("history"),   {{QStringLiteral("document-open-recent")},                                QStringLiteral("icons/polecat-history.svg")}},
        {QStringLiteral("downloads"), {{QStringLiteral("folder-download")},                                     QStringLiteral("icons/polecat-downloads.svg")}},
        {QStringLiteral("menu"),      {{QStringLiteral("open-menu-symbolic"), QStringLiteral("application-menu")}, QStringLiteral("icons/polecat-menu.svg")}},
        {QStringLiteral("private"),   {{QStringLiteral("view-private"), QStringLiteral("security-high")},        QStringLiteral("icons/polecat-private.svg")}},
        {QStringLiteral("reader"),    {{QStringLiteral("accessories-dictionary")},                              QStringLiteral("icons/polecat-reader.svg")}},
        {QStringLiteral("find"),      {{QStringLiteral("edit-find")},                                           QStringLiteral("icons/polecat-find.svg")}},
        {QStringLiteral("sidebar"),   {{QStringLiteral("sidebar-show")},                                        QStringLiteral("icons/polecat-sidebar.svg")}},
        {QStringLiteral("secure"),    {{QStringLiteral("channel-secure")},                                      QStringLiteral("icons/polecat-secure.svg")}},
        {QStringLiteral("insecure"),  {{QStringLiteral("channel-insecure")},                                    QStringLiteral("icons/polecat-insecure.svg")}},
        {QStringLiteral("settings"),  {{QStringLiteral("preferences-system")},                                  QStringLiteral("icons/polecat-settings.svg")}},
    };
    return reg;
}
} // namespace

namespace Polecat {
namespace IconLoader {

QIcon icon(const QString &name)
{
    const auto it = registry().constFind(name);
    if (it != registry().constEnd()) {
        for (const QString &themeName : it->themeNames) {
            QIcon themed = QIcon::fromTheme(themeName);
            if (!themed.isNull() && !themed.availableSizes().isEmpty())
                return themed;
        }
        QIcon bundled(it->resource);
        if (!bundled.isNull() && !bundled.availableSizes().isEmpty())
            return bundled;
    }

    // Last resort: ask the theme directly, then give an empty icon.
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

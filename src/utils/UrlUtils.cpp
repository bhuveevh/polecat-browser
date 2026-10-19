#include "utils/UrlUtils.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QSet>

namespace Polecat {
namespace UrlUtils {

bool isLikelyUrl(const QString &text)
{
    const QString t = text.trimmed();
    if (t.isEmpty())
        return false;

    // Explicit scheme.
    static const QRegularExpression schemeRe(QStringLiteral("^[a-zA-Z][a-zA-Z0-9+.-]*://"));
    if (t.contains(schemeRe))
        return true;

    // Known schemeless prefixes.
    if (t.startsWith(QLatin1String("about:")) || t.startsWith(QLatin1String("file://")))
        return true;

    // No spaces and contains a dot or is localhost / an IP -> treat as URL.
    if (!t.contains(QLatin1Char(' '))) {
        if (t == QLatin1String("localhost") || t.startsWith(QLatin1String("localhost:")))
            return true;

        static const QRegularExpression ipRe(
            QStringLiteral("^\\d{1,3}(\\.\\d{1,3}){3}(:\\d+)?(/.*)?$"));
        if (t.contains(ipRe))
            return true;

        static const QRegularExpression hostRe(
            QStringLiteral("^[^\\s/]+\\.[^\\s/]{2,}(:\\d+)?(/.*)?$"));
        if (t.contains(hostRe))
            return true;
    }

    return false;
}

QUrl toUrl(const QString &text, const QString &searchTemplate)
{
    const QString t = text.trimmed();
    if (t.isEmpty())
        return QUrl(QStringLiteral("about:blank"));

    if (t.startsWith(QLatin1String("about:")) || t.startsWith(QLatin1String("file://")))
        return QUrl(t);

    if (isLikelyUrl(t)) {
        QUrl url = QUrl::fromUserInput(t);
        return url;
    }

    // Search query.
    QString tmpl = searchTemplate;
    if (!tmpl.contains(QLatin1String("%1")))
        tmpl = QStringLiteral("https://duckduckgo.com/?q=%1");
    const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(t));
    return QUrl(tmpl.replace(QLatin1String("%1"), encoded));
}

QString displayString(const QUrl &url)
{
    if (url.scheme() == QLatin1String("about"))
        return url.toString();

    QString s = url.toDisplayString(QUrl::RemovePassword | QUrl::RemoveUserInfo);
    // Show a readable host (punycode -> unicode when possible).
    s.replace(QLatin1String("https://"), QString());
    s.replace(QLatin1String("http://"), QString());
    if (s.endsWith(QLatin1Char('/')) && s.count(QLatin1Char('/')) == 0)
        s.chop(1);
    return s;
}

QString normalizeForSearch(const QString &text)
{
    QString s = text.toLower();
    s = s.simplified();
    return s;
}

QString hostKey(const QUrl &url)
{
    QString host = url.host().toLower();
    if (host.isEmpty())
        host = url.scheme(); // e.g. "about", "file"
    return host;
}

QString sanitizeFileName(const QString &proposed)
{
    if (proposed.isEmpty())
        return QStringLiteral("download");

    QString name = proposed;

    // Take only the final path component to defeat "../" and absolute paths.
    name = name.section(QLatin1Char('/'), -1);
    name = name.section(QLatin1Char('\\'), -1);

    // Drop control characters and characters problematic on Linux desktops.
    name.remove(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]")));
    name.replace(QRegularExpression(QStringLiteral("[<>:\"|?*]")), QStringLiteral("_"));

    name = name.trimmed();
    if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String(".."))
        name = QStringLiteral("download");

    return name;
}

bool isDangerousExtension(const QString &fileName)
{
    static const QSet<QString> dangerous = {
        QStringLiteral("exe"),  QStringLiteral("msi"),  QStringLiteral("bat"),
        QStringLiteral("cmd"),  QStringLiteral("com"),  QStringLiteral("scr"),
        QStringLiteral("pif"),  QStringLiteral("app"),  QStringLiteral("dmg"),
        QStringLiteral("deb"),  QStringLiteral("rpm"),  QStringLiteral("apk"),
        QStringLiteral("sh"),   QStringLiteral("run"),  QStringLiteral("bin"),
        QStringLiteral("desktop"), QStringLiteral("jar"), QStringLiteral("vbs"),
        QStringLiteral("js"),   QStringLiteral("ps1"),  QStringLiteral("py"),
    };
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    return dangerous.contains(suffix);
}

QString stripQueryAndFragment(const QUrl &url)
{
    QUrl copy = url;
    copy.setQuery(QString());
    copy.setFragment(QString());
    return copy.toString();
}

} // namespace UrlUtils
} // namespace Polecat

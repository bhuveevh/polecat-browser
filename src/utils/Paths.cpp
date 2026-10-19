#include "utils/Paths.h"

#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QString>

#include <pwd.h>
#include <unistd.h>

namespace {
constexpr const char *kAppDirName = "polecat-browser";
}

namespace Polecat {
namespace Paths {

static QString appName()
{
    return QString::fromLatin1(kAppDirName);
}

QString configDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + QLatin1Char('/') + appName();
}

QString dataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + QLatin1Char('/') + appName();
}

QString cacheDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
           + QLatin1Char('/') + appName();
}

QString downloadsDir()
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (d.isEmpty())
        d = QDir::homePath() + QStringLiteral("/Downloads");
    return d;
}

QString userDisplayName()
{
    // Prefer the human name from the password database, then the login name.
    if (const struct passwd *pw = ::getpwuid(::getuid())) {
        const QString gecos = QString::fromLocal8Bit(pw->pw_gecos ? pw->pw_gecos : "");
        const QString full = gecos.section(QLatin1Char(','), 0, 0).trimmed();
        if (!full.isEmpty())
            return full;
        const QString login = QString::fromLocal8Bit(pw->pw_name ? pw->pw_name : "");
        if (!login.isEmpty())
            return login;
    }
    const QString env = qEnvironmentVariable("USER");
    if (!env.isEmpty())
        return env;
    return QDir::home().dirName();
}

QString databaseFile()
{
    return dataDir() + QStringLiteral("/polecat.db");
}

QString profilesRoot()
{
    return dataDir() + QStringLiteral("/profiles");
}

QString profileDir(const QString &profileName)
{
    const QString safe = profileName.isEmpty() ? QStringLiteral("default") : profileName;
    return profilesRoot() + QLatin1Char('/') + safe;
}

QString profileCacheDir(const QString &profileName)
{
    const QString safe = profileName.isEmpty() ? QStringLiteral("default") : profileName;
    return cacheDir() + QStringLiteral("/profiles/") + safe + QStringLiteral("/cache");
}

QString resourcePath(const QString &relative)
{
    return QStringLiteral(":/") + relative;
}

bool ensureDir(const QString &path)
{
    if (path.isEmpty())
        return false;
    QDir dir;
    if (dir.exists(path))
        return true;
    return dir.mkpath(path);
}

bool isWritableDir(const QString &path)
{
    if (path.isEmpty())
        return false;
    QFileInfo info(path);
    if (!info.exists() || !info.isDir())
        return false;
    return info.isWritable();
}

} // namespace Paths
} // namespace Polecat

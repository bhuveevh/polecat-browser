#include "utils/Logging.h"

#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <cstdio>

Q_LOGGING_CATEGORY(lcApp, "polecat.app")
Q_LOGGING_CATEGORY(lcDb, "polecat.db")
Q_LOGGING_CATEGORY(lcNet, "polecat.net")
Q_LOGGING_CATEGORY(lcUi, "polecat.ui")
Q_LOGGING_CATEGORY(lcPrivacy, "polecat.privacy")

namespace {
constexpr int kMaxRemembered = 400;
QList<QString> g_messages;
QMutex g_mutex;
bool g_verbose = false;
QtMessageHandler g_previous = nullptr;

const char *levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return "DEBUG";
    case QtInfoMsg:     return "INFO";
    case QtWarningMsg:  return "WARN";
    case QtCriticalMsg: return "CRIT";
    case QtFatalMsg:    return "FATAL";
    }
    return "INFO";
}

void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const QString line = QStringLiteral("%1 [%2] %3")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate),
             QString::fromLatin1(levelName(type)),
             msg);

    {
        QMutexLocker lock(&g_mutex);
        g_messages.append(line);
        while (g_messages.size() > kMaxRemembered)
            g_messages.removeFirst();
    }

    if (g_previous)
        g_previous(type, context, msg);
    else
        std::fprintf(stderr, "%s\n", qPrintable(line));
}
} // namespace

namespace Polecat {
namespace Logging {

void install()
{
    g_previous = qInstallMessageHandler(handler);
}

QString diagnosticLog()
{
    QMutexLocker lock(&g_mutex);
    return g_messages.join(QLatin1Char('\n'));
}

bool isVerbose()
{
    return g_verbose;
}

void setVerbose(bool on)
{
    g_verbose = on;
}

} // namespace Logging
} // namespace Polecat

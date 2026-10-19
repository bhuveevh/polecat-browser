#include "privacy/PermissionManager.h"

#include "database/DatabaseManager.h"
#include "utils/Logging.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace Polecat {

PermissionManager::PermissionManager(DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

QString PermissionManager::featureKey(QWebEnginePage::Feature feature)
{
    switch (feature) {
    case QWebEnginePage::MediaAudioCapture:         return QStringLiteral("microphone");
    case QWebEnginePage::MediaVideoCapture:         return QStringLiteral("camera");
    case QWebEnginePage::MediaAudioVideoCapture:    return QStringLiteral("camera-microphone");
    case QWebEnginePage::Geolocation:               return QStringLiteral("location");
    case QWebEnginePage::Notifications:             return QStringLiteral("notifications");
    case QWebEnginePage::MouseLock:                 return QStringLiteral("mouse-lock");
    case QWebEnginePage::DesktopVideoCapture:       return QStringLiteral("desktop-video");
    case QWebEnginePage::DesktopAudioVideoCapture:  return QStringLiteral("desktop-audio-video");
    default:                                        return QStringLiteral("unknown");
    }
}

QString PermissionManager::featureName(QWebEnginePage::Feature feature)
{
    switch (feature) {
    case QWebEnginePage::MediaAudioCapture:         return QObject::tr("use your microphone");
    case QWebEnginePage::MediaVideoCapture:         return QObject::tr("use your camera");
    case QWebEnginePage::MediaAudioVideoCapture:    return QObject::tr("use your camera and microphone");
    case QWebEnginePage::Geolocation:               return QObject::tr("know your location");
    case QWebEnginePage::Notifications:             return QObject::tr("show notifications");
    case QWebEnginePage::MouseLock:                 return QObject::tr("lock your mouse pointer");
    case QWebEnginePage::DesktopVideoCapture:       return QObject::tr("capture your screen");
    case QWebEnginePage::DesktopAudioVideoCapture:  return QObject::tr("capture your screen and audio");
    default:                                        return QObject::tr("use a protected feature");
    }
}

PermissionManager::Decision PermissionManager::decisionFor(const QString &host,
                                                           QWebEnginePage::Feature feature) const
{
    if (host.isEmpty())
        return Ask;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("SELECT decision FROM permissions WHERE host = ? AND feature = ? LIMIT 1"));
    q.addBindValue(host.toLower());
    q.addBindValue(featureKey(feature));
    if (q.exec() && q.next())
        return Decision(q.value(0).toInt());
    return Ask;
}

void PermissionManager::setDecision(const QString &host, QWebEnginePage::Feature feature,
                                    Decision decision)
{
    if (host.isEmpty())
        return;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral(
        "INSERT INTO permissions(host, feature, decision, updated) VALUES(?, ?, ?, ?) "
        "ON CONFLICT(host, feature) DO UPDATE SET decision = excluded.decision, updated = excluded.updated"));
    q.addBindValue(host.toLower());
    q.addBindValue(featureKey(feature));
    q.addBindValue(int(decision));
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!q.exec())
        qCWarning(lcPrivacy) << "permission store failed:" << q.lastError().text();
    else
        emit changed();
}

void PermissionManager::clearAll()
{
    QSqlQuery q(m_db->database());
    q.exec(QStringLiteral("DELETE FROM permissions"));
    emit changed();
}

void PermissionManager::clearHost(const QString &host)
{
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("DELETE FROM permissions WHERE host = ?"));
    q.addBindValue(host.toLower());
    q.exec();
    emit changed();
}

} // namespace Polecat

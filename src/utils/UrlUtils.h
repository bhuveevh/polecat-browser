#pragma once

#include <QString>
#include <QUrl>

namespace Polecat {
namespace UrlUtils {

// Returns true if `text` should be treated as a URL rather than a search query.
// Handles scheme: prefixes, bare hosts with a dot, localhost, and IPv4.
bool isLikelyUrl(const QString &text);

// Converts whatever the user typed in the address bar into a QUrl.
// `searchTemplate` must contain "%1" for the query (e.g.
// "https://duckduckgo.com/?q=%1").
QUrl toUrl(const QString &text, const QString &searchTemplate);

// Human-friendly form for display: strips "https://" and a trailing slash,
// shows a readable (unicode) host.
QString displayString(const QUrl &url);

// Lower-cased, whitespace-collapsed copy used for database search columns.
QString normalizeForSearch(const QString &text);

// The registrable-ish host used as a permission/database key.
QString hostKey(const QUrl &url);

// Strips directory components and control characters from a proposed download
// filename, defeating path traversal (../, absolute paths, NUL, etc.).
QString sanitizeFileName(const QString &proposed);

// True for extensions we warn about before opening (executables, scripts,
// disk images, etc.). Downloading is allowed; auto-opening is not.
bool isDangerousExtension(const QString &fileName);

// Removes the query and fragment for privacy when storing/logging a URL.
QString stripQueryAndFragment(const QUrl &url);

} // namespace UrlUtils
} // namespace Polecat

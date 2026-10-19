#pragma once

#include <QString>

namespace Polecat {
namespace Paths {

// All runtime locations follow the XDG Base Directory specification and live
// under the user's home directory. Nothing is ever written inside /usr or /opt.

// ~/.config/polecat-browser
QString configDir();

// ~/.local/share/polecat-browser
QString dataDir();

// ~/.cache/polecat-browser
QString cacheDir();

// The user's XDG download directory (falls back to ~/Downloads).
QString downloadsDir();

// SQLite database file: <dataDir>/polecat.db
QString databaseFile();

// Per-profile storage root: <dataDir>/profiles
QString profilesRoot();

// <profilesRoot>/<profileName>
QString profileDir(const QString &profileName);

// Cache for a profile: <cacheDir>/profiles/<profileName>/cache
QString profileCacheDir(const QString &profileName);

// Bundled resource path helper, e.g. resourcePath("html/newtab.html").
QString resourcePath(const QString &relative);

// Creates a directory (recursively) if it does not exist. Returns true on
// success or if the directory already exists.
bool ensureDir(const QString &path);

// True if a writable directory can be created/used at `path`.
bool isWritableDir(const QString &path);

} // namespace Paths
} // namespace Polecat

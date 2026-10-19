#pragma once

#include <QIcon>
#include <QString>

namespace Polecat {
namespace IconLoader {

// Returns an icon for a symbolic name, preferring the user's icon theme and
// falling back to a bundled SVG under :/icons/. `name` is the logical name
// used by the application, e.g. "back", "reload", "bookmark".
QIcon icon(const QString &name);

// The application's own brand icon (window icon, About dialog, menu).
QIcon appIcon();

} // namespace IconLoader
} // namespace Polecat

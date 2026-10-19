#pragma once

#include <QLoggingCategory>

// Logging categories. Enable at runtime, e.g.:
//   QT_LOGGING_RULES="polecat.*=true" polecat-browser
Q_DECLARE_LOGGING_CATEGORY(lcApp)
Q_DECLARE_LOGGING_CATEGORY(lcDb)
Q_DECLARE_LOGGING_CATEGORY(lcNet)
Q_DECLARE_LOGGING_CATEGORY(lcUi)
Q_DECLARE_LOGGING_CATEGORY(lcPrivacy)

namespace Polecat {
namespace Logging {

// Installs a message handler that keeps the last N messages in memory so the
// About dialog can offer a "copy diagnostic information" action, and forwards
// them to stderr.
void install();

// Recent diagnostic log lines (newest last).
QString diagnosticLog();

// True when verbose logging was requested on the command line / environment.
bool isVerbose();
void setVerbose(bool on);

} // namespace Logging
} // namespace Polecat

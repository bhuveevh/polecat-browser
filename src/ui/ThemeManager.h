#pragma once

#include <QObject>
#include <QString>

class QApplication;

namespace Polecat {

class SettingsManager;

// ThemeManager applies the light/dark/system theme and the Polecat accent
// colours. It never forces brand colours over a user's system theme: accents
// are used only for the address-bar progress, focus rings and the header bar.
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum Mode { System, Light, Dark };

    explicit ThemeManager(SettingsManager *settings, QObject *parent = nullptr);

    // Applies the mode from settings (or the given mode) to the application.
    void apply(Mode mode);
    void applyFromSettings();

    static Mode modeFromString(const QString &s);
    static QString modeToString(Mode mode);

    // Brand palette (used sparingly).
    static QString accentPrimary()  { return QStringLiteral("#D88A24"); } // amber
    static QString accentSecondary(){ return QStringLiteral("#3D78B8"); } // blue
    static QString warningColor()   { return QStringLiteral("#D97706"); }
    static QString errorColor()     { return QStringLiteral("#C62828"); }
    static QString successColor()   { return QStringLiteral("#2E7D32"); }

signals:
    void themeChanged(Mode mode);

private:
    bool systemPrefersDark() const;
    void applyPalette(bool dark);
    void applyStyleSheet(bool dark);

    SettingsManager *m_settings = nullptr;
    Mode m_current = System;
};

} // namespace Polecat

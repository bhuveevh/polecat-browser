#include "ui/ThemeManager.h"

#include "settings/SettingsManager.h"

#include <QApplication>
#include <QColor>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

namespace Polecat {

ThemeManager::ThemeManager(SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
}

ThemeManager::Mode ThemeManager::modeFromString(const QString &s)
{
    if (s == QLatin1String("light")) return Light;
    if (s == QLatin1String("dark"))  return Dark;
    return System;
}

QString ThemeManager::modeToString(Mode mode)
{
    switch (mode) {
    case Light: return QStringLiteral("light");
    case Dark:  return QStringLiteral("dark");
    case System: default: return QStringLiteral("system");
    }
}

void ThemeManager::applyFromSettings()
{
    apply(modeFromString(m_settings->value(SettingsManager::kTheme()).toString()));
}

void ThemeManager::apply(Mode mode)
{
    m_current = mode;
    bool dark = false;
    if (mode == Dark)
        dark = true;
    else if (mode == Light)
        dark = false;
    else
        dark = systemPrefersDark();

    applyPalette(dark);
    applyStyleSheet(dark);
    emit themeChanged(mode);
}

bool ThemeManager::systemPrefersDark() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Qt 6.5+ exposes the platform colour scheme directly (works with GNOME's
    // dark/light preference). Older Qt falls back to palette inspection.
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme != Qt::ColorScheme::Unknown)
        return scheme == Qt::ColorScheme::Dark;
#endif
    // Fallback: infer from the current window background lightness.
    const QColor bg = QApplication::palette().color(QPalette::Window);
    return bg.lightness() < 128;
}

void ThemeManager::applyPalette(bool dark)
{
    QPalette p;

    if (dark) {
        const QColor window(0x1E, 0x1E, 0x1E);
        const QColor base(0x25, 0x25, 0x26);
        const QColor alt(0x2B, 0x2B, 0x2C);
        const QColor text(0xF5, 0xF5, 0xF5);
        const QColor disabled(0x8A, 0x8A, 0x8A);

        p.setColor(QPalette::Window, window);
        p.setColor(QPalette::WindowText, text);
        p.setColor(QPalette::Base, base);
        p.setColor(QPalette::AlternateBase, alt);
        p.setColor(QPalette::Text, text);
        p.setColor(QPalette::Button, alt);
        p.setColor(QPalette::ButtonText, text);
        p.setColor(QPalette::ToolTipBase, alt);
        p.setColor(QPalette::ToolTipText, text);
        p.setColor(QPalette::Highlight, QColor(accentSecondary()));
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::Link, QColor(accentSecondary()));
        p.setColor(QPalette::PlaceholderText, disabled);

        p.setColor(QPalette::Disabled, QPalette::Text, disabled);
        p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
        p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    } else {
        const QColor window(0xFA, 0xFA, 0xFA);
        const QColor base(0xFF, 0xFF, 0xFF);
        const QColor alt(0xF0, 0xF0, 0xF0);
        const QColor text(0x20, 0x21, 0x24);
        const QColor disabled(0x9A, 0x9A, 0x9A);

        p.setColor(QPalette::Window, window);
        p.setColor(QPalette::WindowText, text);
        p.setColor(QPalette::Base, base);
        p.setColor(QPalette::AlternateBase, alt);
        p.setColor(QPalette::Text, text);
        p.setColor(QPalette::Button, alt);
        p.setColor(QPalette::ButtonText, text);
        p.setColor(QPalette::ToolTipBase, base);
        p.setColor(QPalette::ToolTipText, text);
        p.setColor(QPalette::Highlight, QColor(accentSecondary()));
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::Link, QColor(accentSecondary()));
        p.setColor(QPalette::PlaceholderText, disabled);

        p.setColor(QPalette::Disabled, QPalette::Text, disabled);
        p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
        p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    }

    QApplication::setPalette(p);
}

void ThemeManager::applyStyleSheet(bool dark)
{
    // Minimal, GNOME-adjacent styling. Kept small on purpose so the app blends
    // into whatever GTK/Qt platform theme is active.
    const QString bg = dark ? QStringLiteral("#1e1e1e") : QStringLiteral("#fafafa");
    const QString barBg = dark ? QStringLiteral("#2b2b2c") : QStringLiteral("#f0f0f0");
    const QString border = dark ? QStringLiteral("#3a3a3a") : QStringLiteral("#d8d8d8");
    const QString text = dark ? QStringLiteral("#f5f5f5") : QStringLiteral("#202124");
    const QString subtle = dark ? QStringLiteral("#9a9a9a") : QStringLiteral("#6b6b6b");

    const QString qss = QStringLiteral(R"(
QMainWindow, QDialog { background: %1; color: %4; }
#headerBar { background: %2; border-bottom: 1px solid %3; }
#headerBar QLabel#headerTitle { color: %4; font-weight: 600; }
#headerBar QLabel#privateBadge {
    background: %5; color: #ffffff; border-radius: 8px; padding: 1px 8px;
    font-size: 11px; font-weight: 600;
}
QToolBar#navigationToolBar { background: %2; border-bottom: 1px solid %3; padding: 2px; spacing: 2px; }
QToolBar#navigationToolBar QToolButton {
    border: none; padding: 5px; border-radius: 6px;
}
QToolBar#navigationToolBar QToolButton:hover { background: %3; }
QToolBar#navigationToolBar QToolButton:checked { background: %3; }
QLineEdit#addressBar, QLineEdit {
    background: %1; color: %4; border: 1px solid %3; border-radius: 8px; padding: 5px 8px;
}
QLineEdit:focus { border: 1px solid %6; }
QProgressBar#addressProgress { border: none; background: transparent; }
QProgressBar#addressProgress::chunk { background: %6; }
QTabBar#mainTabBar::tab {
    background: transparent; color: %7; padding: 6px 12px; margin-right: 2px;
    border-top-left-radius: 6px; border-top-right-radius: 6px; min-width: 90px;
}
QTabBar#mainTabBar::tab:selected { background: %1; color: %4; }
QTabBar#mainTabBar::tab:hover { background: %3; }
QTabWidget::pane { border: 1px solid %3; }
QListWidget { background: %1; border: none; }
QListWidget::item { padding: 6px; border-radius: 6px; }
QListWidget::item:selected { background: %6; color: #ffffff; }
#sidebar { background: %2; border-right: 1px solid %3; }
#sidebar QLabel#sidebarTitle { color: %7; font-weight: 600; padding: 8px; }
QStatusBar { background: %2; color: %7; }
#findBar { background: %2; border-top: 1px solid %3; }
)")
        .arg(bg, barBg, border, text,
             QString::fromLatin1(errorColor().toLatin1()), // %5 (private badge red)
             QString::fromLatin1(accentSecondary().toLatin1()), // %6 accent
             subtle); // %7

    qApp->setStyleSheet(qss);
}

} // namespace Polecat

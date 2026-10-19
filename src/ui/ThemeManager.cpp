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
        const QColor window(0x20, 0x21, 0x24);
        const QColor base(0x25, 0x25, 0x26);
        const QColor alt(0x2B, 0x2B, 0x2C);
        const QColor text(0xE8, 0xEA, 0xED);
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
        const QColor window(0xFF, 0xFF, 0xFF);
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
    // A light, Chrome-like look: tab strip on top, one flat toolbar and a
    // rounded address pill. Kept small and driven by the active light/dark mode
    // so the app still follows the system theme.
    const QString window    = dark ? QStringLiteral("#202124") : QStringLiteral("#ffffff");
    const QString barBg     = dark ? QStringLiteral("#35363a") : QStringLiteral("#ffffff");
    const QString tabStrip  = dark ? QStringLiteral("#202124") : QStringLiteral("#dee1e6");
    const QString tabActive = dark ? QStringLiteral("#35363a") : QStringLiteral("#ffffff");
    const QString field     = dark ? QStringLiteral("#202124") : QStringLiteral("#f1f3f4");
    const QString border    = dark ? QStringLiteral("#3c4043") : QStringLiteral("#dadce0");
    const QString text      = dark ? QStringLiteral("#e8eaed") : QStringLiteral("#202124");
    const QString subtle    = dark ? QStringLiteral("#9aa0a6") : QStringLiteral("#5f6368");

    const QString qss = QStringLiteral(R"(
QMainWindow, QDialog { background: %1; color: %7; }

QToolBar#navigationToolBar {
    background: %2; border: none; border-bottom: 1px solid %5;
    padding: 4px 6px; spacing: 2px;
}
QToolBar#navigationToolBar QToolButton {
    border: none; background: transparent; padding: 6px; border-radius: 16px;
}
QToolBar#navigationToolBar QToolButton:hover { background: %5; }
QToolBar#navigationToolBar QToolButton:pressed { background: %5; }
QToolBar#navigationToolBar QToolButton:checked { background: %5; }

QToolButton#profileButton {
    border: none; background: transparent; padding: 3px 10px;
    border-radius: 16px; color: %7; font-weight: 500;
}
QToolButton#profileButton:hover { background: %5; }

QLabel#privateBadge {
    background: #d93025; color: #ffffff; border-radius: 9px;
    padding: 1px 8px; font-size: 11px; font-weight: 600; margin-right: 4px;
}

QLineEdit#addressBar {
    background: %4; color: %7; border: 1px solid transparent;
    border-radius: 16px; padding: 6px 12px; selection-background-color: %6;
}
QLineEdit#addressBar:focus { background: %1; border: 1px solid %6; }
QProgressBar#addressProgress { border: none; background: transparent; }
QProgressBar#addressProgress::chunk { background: %6; }

#tabBarContainer { background: %3; }
QTabBar#mainTabBar { background: %3; }
QTabBar#mainTabBar::tab {
    background: transparent; color: %8; padding: 7px 14px;
    margin: 4px 2px 0 2px; min-width: 110px;
    border-top-left-radius: 8px; border-top-right-radius: 8px;
}
QTabBar#mainTabBar::tab:selected { background: %9; color: %7; }
QTabBar#mainTabBar::tab:hover:!selected { background: %5; }

QToolButton#newTabButton {
    border: none; background: transparent; padding: 4px; border-radius: 14px;
}
QToolButton#newTabButton:hover { background: %5; }

QListWidget { background: %1; border: none; }
QListWidget::item { padding: 6px; border-radius: 6px; }
QListWidget::item:selected { background: %6; color: #ffffff; }
#sidebar { background: %2; border-right: 1px solid %5; }
QStatusBar { background: %2; color: %8; }
#findBar { background: %2; border-top: 1px solid %5; }
)")
        .arg(window, barBg, tabStrip, field, border,
             accentSecondary(), text, subtle, tabActive);

    qApp->setStyleSheet(qss);
}

} // namespace Polecat

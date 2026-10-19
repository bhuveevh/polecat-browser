#pragma once

#include <QDialog>

class QListWidget;
class QStackedWidget;

namespace Polecat {

class SettingsManager;
class PrivacyManager;
class ProfileManager;

// SettingsDialog is the organised preferences window with the sections
// required by the specification: General, Appearance, Privacy, Search,
// Downloads, Tabs, Study and Advanced.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(SettingsManager *settings,
                   PrivacyManager *privacy,
                   ProfileManager *profiles,
                   QWidget *parent = nullptr);

signals:
    // Emitted after any change so the running windows can react immediately.
    void settingsChanged();

private:
    QWidget *buildGeneral();
    QWidget *buildAppearance();
    QWidget *buildPrivacy();
    QWidget *buildSearch();
    QWidget *buildDownloads();
    QWidget *buildTabs();
    QWidget *buildStudy();
    QWidget *buildAdvanced();

    void addSection(const QString &title, QWidget *page);
    void resetAllSettings();

    SettingsManager *m_settings = nullptr;
    PrivacyManager *m_privacy = nullptr;
    ProfileManager *m_profiles = nullptr;

    QListWidget *m_sections = nullptr;
    QStackedWidget *m_stack = nullptr;
};

} // namespace Polecat

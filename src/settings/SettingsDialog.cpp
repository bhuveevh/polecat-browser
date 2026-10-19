#include "settings/SettingsDialog.h"

#include "privacy/PrivacyManager.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsManager.h"
#include "utils/IconLoader.h"
#include "utils/Paths.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace Polecat {

namespace {

// Small helper: a check box bound to a boolean setting.
QCheckBox *boundCheck(QWidget *parent, const QString &text, SettingsManager *s,
                      const QString &key, bool def = false)
{
    auto *box = new QCheckBox(text, parent);
    box->setChecked(s->value(key, def).toBool());
    QObject::connect(box, &QCheckBox::toggled, s, [s, key](bool on) { s->setValue(key, on); });
    return box;
}

// Small helper: a combo bound to a string setting, with (label, value) pairs.
QComboBox *boundCombo(QWidget *parent, SettingsManager *s, const QString &key,
                      const QList<QPair<QString, QString>> &options)
{
    auto *combo = new QComboBox(parent);
    const QString current = s->value(key).toString();
    for (const auto &opt : options) {
        combo->addItem(opt.first, opt.second);
        if (opt.second == current)
            combo->setCurrentIndex(combo->count() - 1);
    }
    QObject::connect(combo, &QComboBox::currentIndexChanged, s, [combo, s, key](int) {
        s->setValue(key, combo->currentData().toString());
    });
    return combo;
}

QWidget *scrollWrap(QWidget *inner)
{
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(inner);
    return scroll;
}

} // namespace

SettingsDialog::SettingsDialog(SettingsManager *settings,
                               PrivacyManager *privacy,
                               ProfileManager *profiles,
                               QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_privacy(privacy)
    , m_profiles(profiles)
{
    setWindowTitle(tr("Polecat Browser Settings"));
    setWindowIcon(IconLoader::icon(QStringLiteral("settings")));
    resize(760, 560);

    auto *mainLayout = new QVBoxLayout(this);

    m_sections = new QListWidget(this);
    m_sections->setObjectName(QStringLiteral("settingsSections"));
    m_sections->setFixedWidth(180);

    m_stack = new QStackedWidget(this);

    auto *contentRow = new QHBoxLayout();
    contentRow->addWidget(m_sections);
    contentRow->addWidget(m_stack, 1);
    mainLayout->addLayout(contentRow, 1);

    addSection(tr("General"),    buildGeneral());
    addSection(tr("Appearance"), buildAppearance());
    addSection(tr("Privacy"),    buildPrivacy());
    addSection(tr("Search"),     buildSearch());
    addSection(tr("Downloads"),  buildDownloads());
    addSection(tr("Tabs"),       buildTabs());
    addSection(tr("Study"),      buildStudy());
    addSection(tr("Advanced"),   buildAdvanced());

    connect(m_sections, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto *resetButton = buttons->addButton(tr("Reset settings"), QDialogButtonBox::ResetRole);
    connect(resetButton, &QPushButton::clicked, this, &SettingsDialog::resetAllSettings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    mainLayout->addWidget(buttons);
}

void SettingsDialog::addSection(const QString &title, QWidget *page)
{
    m_sections->addItem(title);
    m_stack->addWidget(page);
    if (m_sections->count() == 1)
        m_sections->setCurrentRow(0);
}

QWidget *SettingsDialog::buildGeneral()
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);

    form->addRow(tr("On startup"),
                 boundCombo(page, m_settings, SettingsManager::kStartupBehaviour(), {
                     {tr("Open the new-tab page"), QStringLiteral("newtab")},
                     {tr("Open my home page"),     QStringLiteral("home")},
                     {tr("Restore my last session"), QStringLiteral("restore")},
                 }));

    auto *home = new QLineEdit(m_settings->value(SettingsManager::kHomePage()).toString(), page);
    connect(home, &QLineEdit::editingFinished, this, [this, home]() {
        m_settings->setValue(SettingsManager::kHomePage(), home->text().trimmed());
        emit settingsChanged();
    });
    form->addRow(tr("Home page"), home);

    form->addRow(tr("Language"),
                 boundCombo(page, m_settings, SettingsManager::kLanguage(), {
                     {tr("System default"), QStringLiteral("system")},
                     {tr("English"),        QStringLiteral("en")},
                     {tr("हिन्दी (Hindi)"),  QStringLiteral("hi")},
                 }));

    auto *defaultBrowser = new QLabel(tr(
        "Polecat Browser can be set as your default browser from your desktop's "
        "settings (GNOME Settings → Default Applications)."), page);
    defaultBrowser->setWordWrap(true);
    form->addRow(tr("Default browser"), defaultBrowser);

    return scrollWrap(page);
}

QWidget *SettingsDialog::buildAppearance()
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);

    form->addRow(tr("Theme"),
                 boundCombo(page, m_settings, SettingsManager::kTheme(), {
                     {tr("Follow system"), QStringLiteral("system")},
                     {tr("Light"),         QStringLiteral("light")},
                     {tr("Dark"),          QStringLiteral("dark")},
                 }));
    form->addRow(tr("Density"),
                 boundCombo(page, m_settings, SettingsManager::kDensity(), {
                     {tr("Comfortable"), QStringLiteral("comfortable")},
                     {tr("Compact"),     QStringLiteral("compact")},
                 }));
    form->addRow(tr("Tab bar position"),
                 boundCombo(page, m_settings, SettingsManager::kTabsPosition(), {
                     {tr("Top"),    QStringLiteral("top")},
                     {tr("Bottom"), QStringLiteral("bottom")},
                 }));

    auto *zoom = new QSpinBox(page);
    zoom->setRange(25, 500);
    zoom->setSuffix(QStringLiteral(" %"));
    zoom->setValue(m_settings->value(SettingsManager::kDefaultZoom(), 100).toInt());
    connect(zoom, &QSpinBox::valueChanged, m_settings, [this](int v) {
        m_settings->setValue(SettingsManager::kDefaultZoom(), v);
    });
    form->addRow(tr("Default zoom"), zoom);

    form->addRow(boundCheck(page, tr("Use hardware acceleration when available"),
                            m_settings, SettingsManager::kHardwareAcceleration(), true));

    auto *note = new QLabel(tr(
        "Polecat Browser never overrides your system theme's colours. Brand "
        "accents are used only for indicators and the address bar."), page);
    note->setWordWrap(true);
    form->addRow(note);

    return scrollWrap(page);
}

QWidget *SettingsDialog::buildPrivacy()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *browsing = new QGroupBox(tr("Browsing"), page);
    auto *browsingForm = new QFormLayout(browsing);
    browsingForm->addRow(boundCheck(browsing, tr("Block third-party cookies"),
                                    m_settings, SettingsManager::kBlockThirdPartyCookies(), true));
    browsingForm->addRow(boundCheck(browsing, tr("Send \"Do Not Track\" requests"),
                                    m_settings, SettingsManager::kDoNotTrack(), false));
    browsingForm->addRow(boundCheck(browsing, tr("Enable HTTPS-only mode"),
                                    m_settings, SettingsManager::kHttpsOnly(), false));
    browsingForm->addRow(boundCheck(browsing, tr("Enable JavaScript"),
                                    m_settings, SettingsManager::kJavascriptEnabled(), true));
    browsingForm->addRow(boundCheck(browsing, tr("Load images automatically"),
                                    m_settings, SettingsManager::kImagesEnabled(), true));
    browsingForm->addRow(boundCheck(browsing, tr("Block pop-up windows"),
                                    m_settings, SettingsManager::kPopupBlocking(), true));
    browsingForm->addRow(tr("Automatic media playback"),
                 boundCombo(browsing, m_settings, SettingsManager::kAutoplayPolicy(), {
                     {tr("Require a click"), QStringLiteral("userGesture")},
                     {tr("Allow"),           QStringLiteral("allow")},
                 }));

    auto *content = new QGroupBox(tr("Content blocking"), page);
    auto *contentForm = new QFormLayout(content);
    contentForm->addRow(boundCheck(content, tr("Block known ads and trackers"),
                                   m_settings, SettingsManager::kBlockAdsTrackers(), false));
    auto *sourceLabel = new QLabel(tr(
        "Filter lists are bundled with the application and read from disk; they "
        "are never downloaded without your knowledge. Blocking is best-effort "
        "and is not a guarantee of privacy."), content);
    sourceLabel->setWordWrap(true);
    contentForm->addRow(sourceLabel);

    auto *clearBox = new QGroupBox(tr("Clear browsing data"), page);
    auto *clearLayout = new QVBoxLayout(clearBox);
    auto *clearButtons = new QHBoxLayout();
    auto *clearHistory = new QPushButton(tr("Clear history"), clearBox);
    auto *clearCookies = new QPushButton(tr("Clear cookies & site data"), clearBox);
    auto *clearCache = new QPushButton(tr("Clear cache"), clearBox);
    auto *clearAll = new QPushButton(tr("Clear everything"), clearBox);
    clearButtons->addWidget(clearHistory);
    clearButtons->addWidget(clearCookies);
    clearButtons->addWidget(clearCache);
    clearButtons->addWidget(clearAll);
    clearButtons->addStretch(1);
    clearLayout->addLayout(clearButtons);

    connect(clearHistory, &QPushButton::clicked, this, [this]() {
        m_privacy->clearData(PrivacyManager::History);
    });
    connect(clearCookies, &QPushButton::clicked, this, [this]() {
        m_privacy->clearData(PrivacyManager::Cookies);
    });
    connect(clearCache, &QPushButton::clicked, this, [this]() {
        m_privacy->clearData(PrivacyManager::Cache);
    });
    connect(clearAll, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, tr("Clear everything?"),
                tr("This clears history, cookies, cache, downloads list and site "
                   "permissions on this device. This cannot be undone."),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
            m_privacy->clearEverything();
    });

    layout->addWidget(browsing);
    layout->addWidget(content);
    layout->addWidget(clearBox);
    layout->addStretch(1);

    return scrollWrap(page);
}

QWidget *SettingsDialog::buildSearch()
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);

    form->addRow(tr("Default search engine"),
                 boundCombo(page, m_settings, SettingsManager::kSearchEngine(), {
                     {tr("DuckDuckGo"),        QStringLiteral("duckduckgo")},
                     {tr("DuckDuckGo Lite"),   QStringLiteral("ddg-lite")},
                     {tr("Google"),            QStringLiteral("google")},
                     {tr("Bing"),              QStringLiteral("bing")},
                     {tr("Wikipedia"),         QStringLiteral("wikipedia")},
                 }));
    form->addRow(boundCheck(page, tr("Show search suggestions"),
                            m_settings, SettingsManager::kSearchSuggestions(), false));

    auto *note = new QLabel(tr(
        "Search suggestions send what you type to your search engine. Leave "
        "this off for maximum privacy."), page);
    note->setWordWrap(true);
    form->addRow(note);

    return scrollWrap(page);
}

QWidget *SettingsDialog::buildDownloads()
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);

    auto *dirRow = new QWidget(page);
    auto *dirLayout = new QHBoxLayout(dirRow);
    dirLayout->setContentsMargins(0, 0, 0, 0);
    auto *dir = new QLineEdit(m_settings->value(SettingsManager::kDownloadDir()).toString(), dirRow);
    auto *browse = new QPushButton(tr("Browse…"), dirRow);
    dirLayout->addWidget(dir, 1);
    dirLayout->addWidget(browse);
    connect(browse, &QPushButton::clicked, this, [this, dir]() {
        const QString chosen = QFileDialog::getExistingDirectory(this, tr("Download folder"), dir->text());
        if (!chosen.isEmpty()) {
            dir->setText(chosen);
            m_settings->setValue(SettingsManager::kDownloadDir(), chosen);
        }
    });
    connect(dir, &QLineEdit::editingFinished, this, [this, dir]() {
        m_settings->setValue(SettingsManager::kDownloadDir(), dir->text().trimmed());
    });
    form->addRow(tr("Save downloads to"), dirRow);

    form->addRow(boundCheck(page, tr("Ask where to save each file"),
                            m_settings, SettingsManager::kAskWhereToSave(), false));
    form->addRow(boundCheck(page, tr("Show a notification when a download finishes"),
                            m_settings, SettingsManager::kDownloadNotifications(), true));
    form->addRow(boundCheck(page, tr("Automatically open safe file types when finished"),
                            m_settings, SettingsManager::kAutoOpenSafe(), false));

    auto *warn = new QLabel(tr(
        "Downloaded files are never executed automatically. Executables and "
        "scripts always show a warning first."), page);
    warn->setWordWrap(true);
    form->addRow(warn);

    return scrollWrap(page);
}

QWidget *SettingsDialog::buildTabs()
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);

    form->addRow(boundCheck(page, tr("Restore tabs when the browser restarts"),
                            m_settings, SettingsManager::kRestoreSession(), true));
    form->addRow(boundCheck(page, tr("Open links from other apps in a new tab"),
                            m_settings, SettingsManager::kOpenLinksInNewTab(), false));
    form->addRow(boundCheck(page, tr("Closing the last tab closes the window"),
                            m_settings, SettingsManager::kCloseToLastTab(), true));
    form->addRow(boundCheck(page, tr("Always show the tab bar"),
                            m_settings, SettingsManager::kShowTabBar(), true));

    return scrollWrap(page);
}

QWidget *SettingsDialog::buildStudy()
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);

    form->addRow(boundCheck(page, tr("Enable study tools"),
                            m_settings, SettingsManager::kStudyEnabled(), true));
    form->addRow(boundCheck(page, tr("Enable reader mode"),
                            m_settings, SettingsManager::kReaderModeEnabled(), true));
    form->addRow(boundCheck(page, tr("Enable the selected-text dictionary"),
                            m_settings, SettingsManager::kDictionaryEnabled(), true));
    form->addRow(boundCheck(page, tr("Enable the translation helper"),
                            m_settings, SettingsManager::kTranslationEnabled(), true));

    auto *timer = new QSpinBox(page);
    timer->setRange(1, 180);
    timer->setSuffix(tr(" min"));
    timer->setValue(m_settings->value(SettingsManager::kStudyTimerMinutes(), 25).toInt());
    connect(timer, &QSpinBox::valueChanged, m_settings, [this](int v) {
        m_settings->setValue(SettingsManager::kStudyTimerMinutes(), v);
    });
    form->addRow(tr("Default focus session length"), timer);

    auto *note = new QLabel(tr(
        "Study tools are optional and are designed not to slow down normal "
        "browsing."), page);
    note->setWordWrap(true);
    form->addRow(note);

    return scrollWrap(page);
}

QWidget *SettingsDialog::buildAdvanced()
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);

    form->addRow(boundCheck(page, tr("Enable developer tools (Ctrl+Shift+I)"),
                            m_settings, SettingsManager::kDevToolsEnabled(), true));
    form->addRow(tr("Proxy"),
                 boundCombo(page, m_settings, SettingsManager::kProxyMode(), {
                     {tr("Use system proxy"), QStringLiteral("system")},
                     {tr("No proxy"),         QStringLiteral("none")},
                     {tr("Manual"),           QStringLiteral("manual")},
                 }));
    auto *proxyUrl = new QLineEdit(m_settings->value(SettingsManager::kProxyUrl()).toString(), page);
    proxyUrl->setPlaceholderText(QStringLiteral("http://host:port"));
    connect(proxyUrl, &QLineEdit::editingFinished, m_settings, [this, proxyUrl]() {
        m_settings->setValue(SettingsManager::kProxyUrl(), proxyUrl->text().trimmed());
    });
    form->addRow(tr("Manual proxy URL"), proxyUrl);

    form->addRow(boundCheck(page, tr("Enable verbose logging"),
                            m_settings, SettingsManager::kVerboseLogging(), false));
    form->addRow(boundCheck(page, tr("Enable experimental features"),
                            m_settings, SettingsManager::kExperimental(), false));

    auto *openProfile = new QPushButton(tr("Open profile folder"), page);
    connect(openProfile, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::dataDir()));
    });
    form->addRow(openProfile);

    return scrollWrap(page);
}

void SettingsDialog::resetAllSettings()
{
    if (QMessageBox::question(this, tr("Reset all settings?"),
            tr("All settings will return to their defaults. Your bookmarks, "
               "history and downloads are not affected."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    m_settings->reset();
    emit settingsChanged();
    QMessageBox::information(this, tr("Settings reset"),
        tr("Settings have been reset. Some changes apply after restarting Polecat Browser."));
}

} // namespace Polecat

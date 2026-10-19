#include "ui/AboutDialog.h"

#include "utils/IconLoader.h"
#include "utils/Logging.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSysInfo>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtWebEngineCore/qtwebenginecoreglobal.h>

#ifndef POLECAT_VERSION
#define POLECAT_VERSION "1.0.0"
#endif

namespace Polecat {

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Polecat Browser"));
    setWindowIcon(IconLoader::appIcon());
    setMinimumSize(520, 460);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    // Header: icon + name + tagline.
    auto *headerRow = new QHBoxLayout();
    auto *iconLabel = new QLabel(this);
    iconLabel->setPixmap(IconLoader::appIcon().pixmap(64, 64));
    auto *nameBox = new QVBoxLayout();
    auto *name = new QLabel(QStringLiteral("<b style='font-size:18px'>Polecat Browser</b>"), this);
    auto *tagline = new QLabel(tr("A modern, privacy-focused web browser for Debian Linux."), this);
    tagline->setWordWrap(true);
    nameBox->addWidget(name);
    nameBox->addWidget(tagline);
    headerRow->addWidget(iconLabel, 0, Qt::AlignTop);
    headerRow->addLayout(nameBox, 1);
    layout->addLayout(headerRow);

    auto *tabs = new QTabWidget(this);

    // --- About tab ---
    auto *aboutPage = new QWidget(tabs);
    auto *aboutLayout = new QVBoxLayout(aboutPage);
    auto *info = new QLabel(aboutPage);
    info->setWordWrap(true);
    info->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    info->setText(QStringLiteral(
        "<p><b>%1</b><br>%2</p>"
        "<p><b>%3</b><br>%4</p>"
        "<p><b>%5</b><br>%6</p>"
        "<p><b>%7</b><br>%8</p>")
        .arg(tr("Developer:"), tr("OpentiaOS Developer"),
             tr("Email:"), QStringLiteral("<a href='mailto:opentiaos@gmail.com'>opentiaos@gmail.com</a>"),
             tr("Website:"), tr("Not provided"),
             tr("Technology:"), tr("Built with Qt and Qt WebEngine.")));

    auto *versionLabel = new QLabel(
        tr("Version %1 · Qt %2 · Qt WebEngine (Chromium %3)")
            .arg(QStringLiteral(POLECAT_VERSION),
                 QString::fromLatin1(qVersion()),
                 QString::fromLatin1(qWebEngineVersion())),
        aboutPage);
    versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *buildLabel = new QLabel(
        tr("Build date: %1 · %2 %3")
            .arg(QStringLiteral(__DATE__),
                 QSysInfo::productType(),
                 QSysInfo::prettyProductName()),
        aboutPage);
    buildLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *licenseLabel = new QLabel(
        tr("Licensed under the GNU General Public License v3.0 or later. "
           "This program comes with absolutely no warranty."),
        aboutPage);
    licenseLabel->setWordWrap(true);

    auto *ack = new QGroupBox(tr("Open-source components"), aboutPage);
    auto *ackLayout = new QVBoxLayout(ack);
    auto *ackText = new QLabel(ack);
    ackText->setWordWrap(true);
    ackText->setText(tr(
        "This application uses Qt and Qt WebEngine, which embed the Chromium "
        "project. Chromium and Qt are © their respective contributors and are "
        "used under their own licences. Polecat Browser is not affiliated with, "
        "or endorsed by, Google, Mozilla, Microsoft or Apple."));
    ackLayout->addWidget(ackText);

    aboutLayout->addWidget(info);
    aboutLayout->addWidget(versionLabel);
    aboutLayout->addWidget(buildLabel);
    aboutLayout->addWidget(licenseLabel);
    aboutLayout->addWidget(ack);
    aboutLayout->addStretch(1);

    // --- Privacy tab ---
    auto *privacyPage = new QWidget(tabs);
    auto *privacyLayout = new QVBoxLayout(privacyPage);
    auto *privacy = new QLabel(privacyPage);
    privacy->setWordWrap(true);
    privacy->setText(tr(
        "<p><b>Privacy statement</b></p>"
        "<p>Polecat Browser does not include hidden telemetry, does not collect "
        "your browsing history for any remote service, and does not include "
        "advertisements.</p>"
        "<p>URLs are never sent to a remote server unless you explicitly enable "
        "a feature that requires it (for example, a search suggestion service). "
        "The browser does not silently install extensions, change your default "
        "search engine, or change your home page.</p>"
        "<p>Private browsing hides your activity from this device only. It does "
        "not make you anonymous online: your network, your employer and the "
        "websites you visit may still be able to see your activity.</p>"
        "<p>If content blocking is enabled, filter lists are read from disk and "
        "are not downloaded without your knowledge. Sources and update dates are "
        "shown in Privacy settings.</p>"));
    privacyLayout->addWidget(privacy);
    privacyLayout->addStretch(1);

    tabs->addTab(aboutPage, tr("About"));
    tabs->addTab(privacyPage, tr("Privacy"));

    layout->addWidget(tabs, 1);

    // Buttons.
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto *copyButton = buttons->addButton(tr("Copy diagnostic information"),
                                          QDialogButtonBox::ActionRole);
    connect(copyButton, &QPushButton::clicked, this, &AboutDialog::copyDiagnostics);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(buttons);
}

QString AboutDialog::diagnosticText() const
{
    return QStringLiteral(
        "Polecat Browser %1\n"
        "Qt %2\n"
        "Chromium (QtWebEngine) %3\n"
        "OS: %4 (%5)\n"
        "Kernel: %6\n"
        "CPU: %7\n"
        "Build date: %8\n"
        "---- recent log ----\n%9")
        .arg(QStringLiteral(POLECAT_VERSION),
             QString::fromLatin1(qVersion()),
             QString::fromLatin1(qWebEngineVersion()),
             QSysInfo::productType(),
             QSysInfo::prettyProductName(),
             QSysInfo::kernelVersion(),
             QSysInfo::currentCpuArchitecture(),
             QStringLiteral(__DATE__),
             Logging::diagnosticLog());
}

void AboutDialog::copyDiagnostics()
{
    QApplication::clipboard()->setText(diagnosticText());
}

} // namespace Polecat

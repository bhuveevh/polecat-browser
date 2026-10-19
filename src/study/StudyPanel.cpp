#include "study/StudyPanel.h"

#include "database/DatabaseManager.h"
#include "settings/SettingsManager.h"
#include "utils/Logging.h"
#include "utils/UrlUtils.h"

#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QFile>

namespace Polecat {

StudyPanel::StudyPanel(DatabaseManager *db, SettingsManager *settings, QWidget *parent)
    : QWidget(parent)
    , m_db(db)
    , m_settings(settings)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildNotesTab(), tr("Notes"));
    tabs->addTab(buildVocabularyTab(), tr("Vocabulary"));
    tabs->addTab(buildReadingListTab(), tr("Reading"));
    tabs->addTab(buildToolsTab(), tr("Tools"));

    layout->addWidget(tabs, 1);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &StudyPanel::onTick);

    loadNotes();
    loadVocabulary();
    loadReadingList();
}

QWidget *StudyPanel::buildNotesTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    m_notesList = new QListWidget(page);
    m_noteEditor = new QPlainTextEdit(page);
    m_noteEditor->setPlaceholderText(tr("Write a note…"));

    auto *buttons = new QHBoxLayout();
    auto *save = new QPushButton(tr("Save"), page);
    auto *del = new QPushButton(tr("Delete"), page);
    auto *exportBtn = new QPushButton(tr("Export…"), page);
    buttons->addWidget(save);
    buttons->addWidget(del);
    buttons->addWidget(exportBtn);
    buttons->addStretch(1);

    layout->addWidget(new QLabel(tr("Notes"), page));
    layout->addWidget(m_notesList, 1);
    layout->addWidget(m_noteEditor, 2);
    layout->addLayout(buttons);

    connect(m_notesList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0)
            return;
        auto *item = m_notesList->item(row);
        m_currentNoteId = item->data(Qt::UserRole).toLongLong();
        m_noteEditor->setPlainText(item->data(Qt::UserRole + 1).toString());
    });
    connect(save, &QPushButton::clicked, this, &StudyPanel::saveNote);
    connect(del, &QPushButton::clicked, this, &StudyPanel::deleteNote);
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(this, tr("Export notes"),
                                                          QStringLiteral("polecat-notes.md"),
                                                          tr("Markdown (*.md);;Text (*.txt)"));
        if (path.isEmpty())
            return;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
        QTextStream out(&f);
        out << "# Polecat Browser notes\n\n";
        QSqlQuery q(m_db->database());
        q.exec(QStringLiteral("SELECT title, body FROM notes ORDER BY modified DESC"));
        while (q.next())
            out << "## " << q.value(0).toString() << "\n\n" << q.value(1).toString() << "\n\n";
    });

    return page;
}

QWidget *StudyPanel::buildVocabularyTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    m_vocabList = new QListWidget(page);

    auto *form = new QFormLayout();
    m_wordEdit = new QLineEdit(page);
    m_meaningEdit = new QLineEdit(page);
    form->addRow(tr("Word"), m_wordEdit);
    form->addRow(tr("Meaning"), m_meaningEdit);

    auto *add = new QPushButton(tr("Add word"), page);
    connect(add, &QPushButton::clicked, this, &StudyPanel::addVocabularyEntry);
    connect(m_wordEdit, &QLineEdit::returnPressed, this, &StudyPanel::addVocabularyEntry);

    layout->addWidget(new QLabel(tr("Saved words"), page));
    layout->addWidget(m_vocabList, 1);
    layout->addLayout(form);
    layout->addWidget(add);

    return page;
}

QWidget *StudyPanel::buildReadingListTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    m_readingList = new QListWidget(page);
    auto *hint = new QLabel(tr("Save pages for later from the page menu."), page);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    layout->addWidget(m_readingList, 1);
    connect(m_readingList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        const QUrl url(item->data(Qt::UserRole).toString());
        if (url.isValid())
            emit openUrl(url);
    });
    return page;
}

QWidget *StudyPanel::buildToolsTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    // Study timer.
    auto *timerBox = new QWidget(page);
    auto *timerLayout = new QHBoxLayout(timerBox);
    m_minutes = new QSpinBox(timerBox);
    m_minutes->setRange(1, 180);
    m_minutes->setValue(m_settings->value(SettingsManager::kStudyTimerMinutes(), 25).toInt());
    m_minutes->setSuffix(tr(" min"));
    auto *start = new QPushButton(tr("Start focus"), timerBox);
    auto *stop = new QPushButton(tr("Stop"), timerBox);
    m_timerLabel = new QLabel(QStringLiteral("00:00"), timerBox);
    timerLayout->addWidget(m_minutes);
    timerLayout->addWidget(start);
    timerLayout->addWidget(stop);
    timerLayout->addWidget(m_timerLabel, 1);
    connect(start, &QPushButton::clicked, this, &StudyPanel::startTimer);
    connect(stop, &QPushButton::clicked, this, &StudyPanel::stopTimer);

    // Quick links.
    auto *links = new QListWidget(page);
    const QList<QPair<QString, QString>> quickLinks = {
        {tr("Wikipedia"), QStringLiteral("https://en.wikipedia.org")},
        {tr("Khan Academy"), QStringLiteral("https://www.khanacademy.org")},
        {tr("Project Gutenberg"), QStringLiteral("https://www.gutenberg.org")},
        {tr("Wiktionary (dictionary)"), QStringLiteral("https://en.wiktionary.org")},
        {tr("arXiv"), QStringLiteral("https://arxiv.org")},
    };
    for (const auto &pair : quickLinks) {
        auto *item = new QListWidgetItem(pair.first, links);
        item->setData(Qt::UserRole, pair.second);
    }
    connect(links, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit openUrl(QUrl(item->data(Qt::UserRole).toString()));
    });

    // Translation helper.
    auto *translateBox = new QWidget(page);
    auto *translateLayout = new QVBoxLayout(translateBox);
    m_translateInput = new QLineEdit(translateBox);
    m_translateInput->setPlaceholderText(tr("Text to translate (Hindi ⇄ English)"));
    auto *langRow = new QHBoxLayout();
    auto *direction = new QComboBox(translateBox);
    direction->addItem(tr("Hindi → English"));
    direction->addItem(tr("English → Hindi"));
    auto *translateBtn = new QPushButton(tr("Translate"), translateBox);
    langRow->addWidget(direction, 1);
    langRow->addWidget(translateBtn);
    translateLayout->addWidget(m_translateInput);
    translateLayout->addLayout(langRow);
    auto *note = new QLabel(tr("Translation opens a translation website in a new tab. "
                               "Polecat Browser itself does not send your text anywhere."),
                            translateBox);
    note->setWordWrap(true);
    translateLayout->addWidget(note);
    connect(translateBtn, &QPushButton::clicked, this, &StudyPanel::translate);

    layout->addWidget(new QLabel(tr("Focus timer"), page));
    layout->addWidget(timerBox);
    layout->addWidget(new QLabel(tr("Quick study links"), page));
    layout->addWidget(links);
    layout->addWidget(new QLabel(tr("Translate"), page));
    layout->addWidget(translateBox);
    layout->addStretch(1);

    return page;
}

void StudyPanel::loadNotes()
{
    if (!m_db || !m_db->isOpen())
        return;
    m_notesList->clear();
    QSqlQuery q(m_db->database());
    q.exec(QStringLiteral("SELECT id, title, body FROM notes ORDER BY modified DESC"));
    while (q.next()) {
        auto *item = new QListWidgetItem(q.value(1).toString(), m_notesList);
        item->setData(Qt::UserRole, q.value(0).toLongLong());
        item->setData(Qt::UserRole + 1, q.value(2).toString());
    }
}

void StudyPanel::saveNote()
{
    if (!m_db || !m_db->isOpen())
        return;
    const QString body = m_noteEditor->toPlainText();
    if (body.trimmed().isEmpty())
        return;
    const QString title = body.left(40).section(QLatin1Char('\n'), 0, 0);
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    QSqlQuery q(m_db->database());
    if (m_currentNoteId > 0) {
        q.prepare(QStringLiteral("UPDATE notes SET title = ?, body = ?, modified = ? WHERE id = ?"));
        q.addBindValue(title);
        q.addBindValue(body);
        q.addBindValue(now);
        q.addBindValue(m_currentNoteId);
    } else {
        q.prepare(QStringLiteral("INSERT INTO notes(url, title, body, created, modified) VALUES('', ?, ?, ?, ?)"));
        q.addBindValue(title);
        q.addBindValue(body);
        q.addBindValue(now);
        q.addBindValue(now);
    }
    if (q.exec()) {
        m_currentNoteId = -1;
        m_noteEditor->clear();
        loadNotes();
    } else {
        qCWarning(lcDb) << "note save failed:" << q.lastError().text();
    }
}

void StudyPanel::deleteNote()
{
    if (m_currentNoteId <= 0 || !m_db)
        return;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("DELETE FROM notes WHERE id = ?"));
    q.addBindValue(m_currentNoteId);
    q.exec();
    m_currentNoteId = -1;
    m_noteEditor->clear();
    loadNotes();
}

void StudyPanel::loadVocabulary()
{
    if (!m_db || !m_db->isOpen())
        return;
    m_vocabList->clear();
    QSqlQuery q(m_db->database());
    q.exec(QStringLiteral("SELECT word, meaning FROM vocabulary ORDER BY created DESC"));
    while (q.next()) {
        auto *item = new QListWidgetItem(
            QStringLiteral("%1 — %2").arg(q.value(0).toString(), q.value(1).toString()), m_vocabList);
    }
}

void StudyPanel::addVocabularyEntry()
{
    const QString word = m_wordEdit->text().trimmed();
    if (word.isEmpty() || !m_db)
        return;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("INSERT INTO vocabulary(word, meaning, language, created) VALUES(?, ?, '', ?)"));
    q.addBindValue(word);
    q.addBindValue(m_meaningEdit->text().trimmed());
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();
    m_wordEdit->clear();
    m_meaningEdit->clear();
    loadVocabulary();
}

void StudyPanel::addVocabulary(const QString &word)
{
    m_wordEdit->setText(word);
    m_meaningEdit->setFocus();
}

void StudyPanel::loadReadingList()
{
    if (!m_db || !m_db->isOpen())
        return;
    m_readingList->clear();
    QSqlQuery q(m_db->database());
    q.exec(QStringLiteral("SELECT title, url FROM reading_list ORDER BY added DESC"));
    while (q.next()) {
        auto *item = new QListWidgetItem(q.value(0).toString(), m_readingList);
        item->setData(Qt::UserRole, q.value(1).toString());
        item->setToolTip(q.value(1).toString());
    }
}

void StudyPanel::addReadingListItem(const QString &title, const QUrl &url)
{
    if (!m_db || !m_db->isOpen() || !url.isValid())
        return;
    QSqlQuery q(m_db->database());
    q.prepare(QStringLiteral("INSERT INTO reading_list(title, url, added, read) VALUES(?, ?, ?, 0)"));
    q.addBindValue(title.isEmpty() ? url.host() : title);
    q.addBindValue(url.toString());
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();
    loadReadingList();
}

void StudyPanel::startTimer()
{
    m_remaining = m_minutes->value() * 60;
    m_running = true;
    m_timer->start();
    m_settings->setValue(SettingsManager::kStudyTimerMinutes(), m_minutes->value());
}

void StudyPanel::stopTimer()
{
    m_running = false;
    m_timer->stop();
    m_timerLabel->setText(QStringLiteral("00:00"));
}

void StudyPanel::onTick()
{
    if (!m_running)
        return;
    if (m_remaining <= 0) {
        m_timer->stop();
        m_running = false;
        m_timerLabel->setText(tr("Done — take a break"));
        QMessageBox::information(this, tr("Focus session complete"),
                                 tr("Your focus session has finished. Take a short break."));
        return;
    }
    --m_remaining;
    m_timerLabel->setText(QStringLiteral("%1:%2")
                              .arg(m_remaining / 60, 2, 10, QLatin1Char('0'))
                              .arg(m_remaining % 60, 2, 10, QLatin1Char('0')));
}

void StudyPanel::translate()
{
    const QString text = m_translateInput->text().trimmed();
    if (text.isEmpty())
        return;
    // Open a translation site in a new tab. The user initiated this, so the
    // network request is explicit; Polecat itself sends nothing.
    const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(text));
    const QUrl url(QStringLiteral("https://translate.google.com/?sl=auto&tl=en&text=%1").arg(encoded));
    emit openUrl(url);
}

} // namespace Polecat

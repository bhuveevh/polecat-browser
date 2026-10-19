#pragma once

#include <QWidget>
#include <QUrl>

class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QLabel;
class QTimer;
class QSpinBox;

namespace Polecat {

class DatabaseManager;
class SettingsManager;

// StudyPanel groups Polecat's optional study tools: notes, vocabulary, reading
// list, a focus/study timer, quick educational links and a Hindi–English
// translation helper.
//
// Nothing here sends data anywhere by itself: the translation helper opens a
// translation website in a tab only when the user clicks Translate, so the
// network request is explicit and visible.
class StudyPanel : public QWidget
{
    Q_OBJECT

public:
    StudyPanel(DatabaseManager *db, SettingsManager *settings, QWidget *parent = nullptr);

public slots:
    void addReadingListItem(const QString &title, const QUrl &url);
    void addVocabulary(const QString &word);

signals:
    void openUrl(const QUrl &url);

private:
    QWidget *buildNotesTab();
    QWidget *buildVocabularyTab();
    QWidget *buildReadingListTab();
    QWidget *buildToolsTab();

    void loadNotes();
    void saveNote();
    void deleteNote();
    void loadVocabulary();
    void addVocabularyEntry();
    void loadReadingList();

    void startTimer();
    void stopTimer();
    void onTick();
    void translate();

    DatabaseManager *m_db = nullptr;
    SettingsManager *m_settings = nullptr;

    // Notes
    QListWidget *m_notesList = nullptr;
    QPlainTextEdit *m_noteEditor = nullptr;
    qint64 m_currentNoteId = -1;

    // Vocabulary
    QListWidget *m_vocabList = nullptr;
    QLineEdit *m_wordEdit = nullptr;
    QLineEdit *m_meaningEdit = nullptr;

    // Reading list
    QListWidget *m_readingList = nullptr;

    // Timer
    QSpinBox *m_minutes = nullptr;
    QLabel *m_timerLabel = nullptr;
    QTimer *m_timer = nullptr;
    int m_remaining = 0;
    bool m_running = false;

    // Translation
    QLineEdit *m_translateInput = nullptr;
};

} // namespace Polecat

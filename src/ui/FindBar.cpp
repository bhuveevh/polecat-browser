#include "ui/FindBar.h"

#include "utils/IconLoader.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QShowEvent>
#include <QToolButton>
#include <QWebEngineFindTextResult>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace Polecat {

FindBar::FindBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("findBar"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    // A modern search glyph instead of the old "Find:" text label.
    auto *label = new QLabel(this);
    label->setObjectName(QStringLiteral("findGlyph"));
    label->setPixmap(IconLoader::icon(QStringLiteral("find")).pixmap(16, 16));

    m_field = new QLineEdit(this);
    m_field->setPlaceholderText(tr("Find in page"));
    m_field->setAccessibleName(tr("Find in page"));
    m_field->setClearButtonEnabled(true);

    auto *caseBox = new QCheckBox(tr("Match case"), this);

    auto *prev = new QToolButton(this);
    prev->setIcon(IconLoader::icon(QStringLiteral("back")));
    prev->setToolTip(tr("Previous match"));
    prev->setAutoRaise(true);

    auto *next = new QToolButton(this);
    next->setIcon(IconLoader::icon(QStringLiteral("forward")));
    next->setToolTip(tr("Next match"));
    next->setAutoRaise(true);

    auto *close = new QToolButton(this);
    close->setIcon(IconLoader::icon(QStringLiteral("close")));
    close->setToolTip(tr("Close find bar"));
    close->setAutoRaise(true);

    m_status = new QLabel(this);
    m_status->setMinimumWidth(90);

    layout->addWidget(label);
    layout->addWidget(m_field, 1);
    layout->addWidget(m_status);
    layout->addWidget(caseBox);
    layout->addWidget(prev);
    layout->addWidget(next);
    layout->addWidget(close);

    connect(m_field, &QLineEdit::returnPressed, this, &FindBar::findNext);
    connect(m_field, &QLineEdit::textChanged, this, &FindBar::onTextChanged);
    connect(prev, &QToolButton::clicked, this, &FindBar::findPrevious);
    connect(next, &QToolButton::clicked, this, &FindBar::findNext);
    connect(close, &QToolButton::clicked, this, &FindBar::closeRequested);
    connect(caseBox, &QCheckBox::toggled, this, [this](bool on) {
        m_caseSensitive = on;
        if (!m_field->text().isEmpty())
            doFind(true);
    });
}

void FindBar::setTarget(QWebEngineView *view)
{
    m_view = view;
    if (m_view && m_view->page()) {
        connect(m_view->page(), &QWebEnginePage::findTextFinished, this,
                [this](const QWebEngineFindTextResult &result) {
                    const int n = result.numberOfMatches();
                    m_status->setText(n > 0 ? tr("%1 matches").arg(n) : tr("No matches"));
                });
    }
}

void FindBar::activate()
{
    show();
    m_field->setFocus(Qt::ShortcutFocusReason);
    m_field->selectAll();
}

void FindBar::repositionToParent(int topOffset)
{
    m_topOffset = topOffset;
    QWidget *p = parentWidget();
    if (!p)
        return;
    adjustSize();
    const int margin = 14;
    int x = p->width() - width() - margin;
    if (x < margin)
        x = margin;
    move(x, qMax(margin, topOffset));
    raise();
}

void FindBar::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    repositionToParent(m_topOffset);
}

void FindBar::deactivate()
{
    if (m_view && m_view->page())
        m_view->page()->findText(QString());
    hide();
}

void FindBar::findNext() { doFind(true); }
void FindBar::findPrevious() { doFind(false); }

void FindBar::onTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        if (m_view && m_view->page())
            m_view->page()->findText(QString());
        m_status->clear();
        return;
    }
    doFind(true);
}

void FindBar::doFind(bool forward)
{
    if (!m_view || !m_view->page())
        return;
    const QString text = m_field->text();
    if (text.isEmpty())
        return;

    QWebEnginePage::FindFlags flags;
    if (!forward)
        flags |= QWebEnginePage::FindBackward;
    if (m_caseSensitive)
        flags |= QWebEnginePage::FindCaseSensitively;

    m_view->page()->findText(text, flags);
}

} // namespace Polecat

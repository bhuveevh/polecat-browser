#pragma once

#include <QDialog>

namespace Polecat {

// AboutDialog shows application identity, version, licence, open-source
// acknowledgements and the privacy statement, plus a "copy diagnostic
// information" button.
//
// It deliberately does not invent a website URL and does not imply affiliation
// with any other browser vendor.
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

private slots:
    void copyDiagnostics();

private:
    QString diagnosticText() const;
};

} // namespace Polecat

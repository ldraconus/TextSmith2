#pragma once

#include <QDialog>
#include <QLineEdit>

#include <List.h>

#include "Exporter.h"

namespace Ui {
class ExportDialog;
}

class ExportDialog: public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(ExporterBase* exporter,
                          QWidget* parent = nullptr);
    ~ExportDialog();

    QString chapterTag() const;
    QString coverTag() const;
    QString filename() const;
    QString sceneTag() const;

private:
    ExporterBase*     mExporter;
    List<QLineEdit*>  mLine;
    QPushButton*      mOk;
    Ui::ExportDialog* mUi;

    QString getString(QLineEdit* field) const;
    void    loadFields();

public slots:
    void    apply();
    void    filenameChanged();
    void    getFilename();
};

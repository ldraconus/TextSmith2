#include "ExportDialog.h"
#include "ui_ExportDialog.h"

#include <QFileDialog>
#include <QComboBox>

#include "Main.h"
#include "Preferences.h"
#include "ui_Main.h"

#include <enumerate.h>

ExportDialog::ExportDialog(ExporterBase* exporter, QWidget* parent)
    : QDialog(parent)
    , mExporter(exporter)
    , mUi(new Ui::ExportDialog) {
    mUi->setupUi(this);

    mUi->getFilePushButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));

    auto& prefs = Main::ref().prefs();
    mUi->chapterTagLineEdit->setPlaceholderText(prefs.chapterTag());
    mUi->sceneTagLineEdit->setPlaceholderText(prefs.sceneTag());
    mUi->coverTagLineEdit->setPlaceholderText(prefs.coverTag());
    QFont font(prefs.uiFontFamily(), prefs.uiFontSize());
    prefs.applyFontToTree(this, font);
    updateGeometry();
    repaint();

    loadFields();

    auto* apply = mUi->buttonBox->button(QDialogButtonBox::Apply);
    connect(apply,                       &QPushButton::clicked,  this, &ExportDialog::apply);
    connect(mUi->exportFilenameLineEdit, &QLineEdit::textEdited, this, &ExportDialog::filenameChanged);
    connect(mUi->getFilePushButton,      &QPushButton::clicked,  this, &ExportDialog::getFilename);

    mOk = mUi->buttonBox->button(QDialogButtonBox::Ok);
    mOk->setDisabled(true);

    setWindowTitle(exporter->name() + " export");
}

ExportDialog::~ExportDialog() {
    delete mUi;
}

QString ExportDialog::chapterTag() const {
    return getString(mUi->chapterTagLineEdit);
}

QString ExportDialog::coverTag() const {
    return getString(mUi->coverTagLineEdit);
}

QString ExportDialog::filename() const {
    return getString(mUi->exportFilenameLineEdit);
}

QString ExportDialog::sceneTag() const {
    return getString(mUi->sceneTagLineEdit);
}

QString ExportDialog::getString(QLineEdit* field) const {
    QString text = field->text();
    if (text.isEmpty()) text = field->placeholderText();
    return text;
}

void ExportDialog::loadFields() {
    const auto& defaults = mExporter->collectMetadataDefaults();
    const auto& metaData = mExporter->metadataFields();
    bool create = mWidget.isEmpty();
    QLineEdit* lineEdit { nullptr };
    QComboBox* comboBox { nullptr };
    for (auto&& [i, meta]: enumerate(metaData)) {
        if (create) {
            auto* label = new QLabel(this);
            label->setText(meta.label);
            if (meta.getChoices != nullptr) {
                comboBox = new QComboBox(this);
                comboBox->setObjectName(meta.key);
                StringList choices = meta.getChoices();
                choices.insertAt(0, "");
                comboBox->addItems(choices.toQStringList());
                mUi->formLayout->addRow(label, comboBox);
                mWidget.append(comboBox);
            } else {
                lineEdit = new QLineEdit(this);
                lineEdit->setObjectName(meta.key);
                mUi->formLayout->addRow(label, lineEdit);
                mWidget.append(lineEdit);
            }
        } else {
            if (meta.getChoices != nullptr) comboBox = dynamic_cast<QComboBox*>(mWidget[i]);
            else lineEdit = dynamic_cast<QLineEdit*>(mWidget[i]);
        }
        if (meta.getChoices != nullptr) {
            if (comboBox) comboBox->setPlaceholderText(defaults[meta.key]);
            if (!meta.value.isEmpty() && comboBox) comboBox->setCurrentText(meta.value);
        } else {
            if (lineEdit) lineEdit->setPlaceholderText(defaults[meta.key]);
            if (!meta.value.isEmpty() && lineEdit) lineEdit->setText(meta.value);
        }
    }
}

void ExportDialog::apply() {
    QString chapter = mUi->chapterTagLineEdit->text();
    if (chapter.isEmpty()) chapter = mUi->chapterTagLineEdit->placeholderText();
    QString scene = mUi->sceneTagLineEdit->text();
    if (scene.isEmpty()) scene = mUi->sceneTagLineEdit->placeholderText();
    QString cover = mUi->coverTagLineEdit->text();
    if (cover.isEmpty()) cover = mUi->coverTagLineEdit->placeholderText();
    mExporter->setIds(Main::ref().vectorOfIds(Main::ref().ui()->treeWidget->currentItem(), { chapter, scene,cover }));
    auto& metaData = mExporter->metadataFields();
    int i = 0;
    for (auto& meta: metaData) {
        if (meta.getChoices != nullptr) {
            QComboBox* comboBox = dynamic_cast<QComboBox*>(mWidget[i]);
            if (!comboBox->currentText().isEmpty()) meta.value = comboBox->currentText();
        } else {
            QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(mWidget[i]);
            if (!lineEdit->text().isEmpty()) meta.value = lineEdit->text();
        }
    }
    loadFields();
}

void ExportDialog::filenameChanged() {
    mOk->setDisabled(mUi->exportFilenameLineEdit->text().isEmpty());
}

void ExportDialog::getFilename() {
    QString name = mExporter->name().toLower();
    QString ext =  mExporter->fileExtension().toLower();
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save the " + name + " file as?",
                                                    Main::ref().docDir(),
                                                    name + " files (*." + ext + ");;All Files (*.*)");
    if (filename.isEmpty()) return;
    mUi->exportFilenameLineEdit->setText(filename);
    filenameChanged();
}

#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"
#include "Main.h"

#include <QFontDialog>
#include <QTextToSpeech>

PreferencesDialog* PreferencesDialog::sPreferencesDialog = nullptr;

PreferencesDialog::PreferencesDialog(Preferences& prefs, QWidget* parent)
    : QDialog(parent)
    , mPrefs(&prefs)
    , mSpeech(new QTextToSpeech())
    , mUi(new Ui::PreferencesDialog) {
    sPreferencesDialog = this;

    mUi->setupUi(this);

    loadAvailableVoices();
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    setTextEditFont(font);
    mUi->autoSaveCheckBox->setCheckState(mPrefs->autoSave() ? Qt::Checked : Qt::Unchecked);
    autoSaveChanged(mUi->autoSaveCheckBox->checkState());
    mUi->novelFontPushButton->setFont(font);
    mUi->novelFontPushButton->setText(QString("%1:%2").arg(mPrefs->fontFamily()).arg(mPrefs->fontSize()));
    mUi->displayThemeComboBox->setCurrentIndex(mPrefs->theme());
    theme(mPrefs->theme());

    setupConnections();
}

PreferencesDialog::~PreferencesDialog() {
    mPrefs->setAutoSave(mUi->autoSaveCheckBox->isChecked());
    delete mUi;
    delete mSpeech;
}

void PreferencesDialog::autoSaveChanged(Qt::CheckState) {
    bool state = mUi->autoSaveCheckBox->isChecked();
    mUi->autoSaveIntervalSpinBox->setEnabled(state);
}

void PreferencesDialog::saveChanges() {
    mPrefs->setAutoSave(mUi->autoSaveCheckBox->isChecked());
    mPrefs->setAutoSaveInterval(mUi->autoSaveIntervalSpinBox->value());
    auto font = mUi->novelFontPushButton->font();
    mPrefs->setFontFamily(font.family());
    mPrefs->setFontSize(font.pointSize());
    mPrefs->setTheme(mUi->displayThemeComboBox->currentIndex());
    mPrefs->setTypingSounds(mUi->typewriteSoundsCheckBox->isChecked());
    if (mUi->readAloudVoiceComboBox->isEnabled()) mPrefs->setVoice(mUi->readAloudVoiceComboBox->currentIndex());
}

void PreferencesDialog::theme(int idx) {
    switch (idx) {
    case 0: mPrefs->setLightTheme();  break;
    case 1: mPrefs->setDarkTheme();   break;
    case 2: mPrefs->setSystemTheme(); break;
    }
}

void PreferencesDialog::font() {
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok, mUi->novelFontPushButton->font(), this, "Pick a font to use for your novel");
    if (!ok) return;

    Main::ref().busy();
    mUi->novelFontPushButton->setFont(font);
    mUi->novelFontPushButton->setText(QString("%1:%2").arg(font.family()).arg(font.pointSize()));

    setTextEditFont(font);
}

void PreferencesDialog::loadAvailableVoices() {
    if (mSpeech == nullptr || mSpeech->state() == QTextToSpeech::Error) {
        mUi->readAloudVoiceComboBox->clear();
        mUi->readAloudVoiceComboBox->setDisabled(true);
        return;
    }

    auto voices = mSpeech->availableVoices();
    if (voices.isEmpty()) {
        mUi->readAloudVoiceComboBox->clear();
        mUi->readAloudVoiceComboBox->setDisabled(true);
        return;
    }
    StringList availableVoices;
    for (auto& voice: voices) availableVoices << voice.name();
    availableVoices.sort();
    mUi->readAloudVoiceComboBox->addItems(availableVoices.toQStringList());
    mUi->readAloudVoiceComboBox->setCurrentIndex(mPrefs->voice());
}

void PreferencesDialog::setTextEditFont(QFont& font) {
    mUi->textEdit->setCurrentFont(font);
    QTextDocument* doc = mUi->textEdit->document();
    Main::changeDocumentFont(doc, font);
}

void PreferencesDialog::setupConnections() {
    connect(this,                      &PreferencesDialog::accepted,    this, &PreferencesDialog::saveChanges);
    connect(mUi->autoSaveCheckBox,     &QCheckBox::checkStateChanged,   this, &PreferencesDialog::autoSaveChanged);
    connect(mUi->novelFontPushButton,  &QPushButton::clicked,           this, &PreferencesDialog::font);
    connect(mUi->displayThemeComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::theme);
}

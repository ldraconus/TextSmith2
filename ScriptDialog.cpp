#include "ScriptDialog.h"
#include "ui_ScriptDialog.h"

ScriptDialog::ScriptDialog(QWidget *parent)
    : QDialog(parent)
    , mUi(new Ui::ScriptDialog) {
    mUi->setupUi(this);

    connect(mUi->allActivePushButton,    &QPushButton::clicked, this, &ScriptDialog::allActive);
    connect(mUi->allInactivePushButton,  &QPushButton::clicked, this, &ScriptDialog::allInactive);
    connect(mUi->makeActivePushButton,   &QPushButton::clicked, this, &ScriptDialog::makeActive);
    connect(mUi->makeInactivePushButton, &QPushButton::clicked, this, &ScriptDialog::makeInactive);

    connect(mUi->activeListWidget,    &QListWidget::itemClicked,       this, &ScriptDialog::activeItemClicked);
    connect(mUi->activeListWidget,    &QListWidget::itemDoubleClicked, this, &ScriptDialog::activeItemDoubleClicked);
    connect(mUi->availableListWidget, &QListWidget::itemClicked,       this, &ScriptDialog::availableItemClicked);
    connect(mUi->availableListWidget, &QListWidget::itemDoubleClicked, this, &ScriptDialog::availableItemDoubleClicked);
}

ScriptDialog::~ScriptDialog() {
    delete mUi;
}

void ScriptDialog::setActiveScripts(const StringList &list) {
    QListWidget* active = mUi->activeListWidget;
    active->clear();
    for (const auto& activeScript: list) active->addItem(activeScript);
}

void ScriptDialog::setAvailableScripts(const StringList &list) {
    QListWidget* available = mUi->availableListWidget;
    available->clear();
    for (const auto& availableScript: list) available->addItem(availableScript);
}

StringList ScriptDialog::activeScripts() const {
    QListWidget* active = mUi->activeListWidget;
    StringList scripts;
    auto num = active->count();
    for (auto i = 0; i < num; ++i) scripts += active->item(i)->text();
    return scripts;
}

void ScriptDialog::activeItemClicked(QListWidgetItem*) {
    setButtonStates();
}

void ScriptDialog::activeItemDoubleClicked(QListWidgetItem*) {
    if (mUi->activeListWidget->selectedItems().count() == 0) return;
    auto* item = mUi->activeListWidget->selectedItems()[0];
    mUi->activeListWidget->removeItemWidget(item);
    setButtonStates();
}

void ScriptDialog::allActive() {
    mUi->activeListWidget->clear();
    int num = mUi->availableListWidget->count();
    for (auto i = 0; i < num; ++i) mUi->activeListWidget->addItem(mUi->availableListWidget->item(i)->text());
    setButtonStates();
}

void ScriptDialog::allInactive() {
    mUi->activeListWidget->clear();
    setButtonStates();
}

void ScriptDialog::availableItemClicked(QListWidgetItem*) {
    setButtonStates();
}

void ScriptDialog::availableItemDoubleClicked(QListWidgetItem* item) {
    int num = mUi->activeListWidget->count();
    for (auto i = 0; i < num; ++i) {
        if (item->text() == mUi->activeListWidget->item(i)->text()) return;
    }
    mUi->activeListWidget->addItem(item->text());
    setButtonStates();
}

void ScriptDialog::makeActive() {
    if (mUi->availableListWidget->selectedItems().count() == 0) return;
    QString text = mUi->availableListWidget->selectedItems()[0]->text();
    int num = mUi->activeListWidget->count();
    for (auto i = 0; i < num; ++i) {
        if (text == mUi->activeListWidget->item(i)->text()) return;
    }
    mUi->activeListWidget->addItem(text);
    setButtonStates();
}

void ScriptDialog::makeInactive() {
    if (mUi->activeListWidget->selectedItems().count() == 0) return;
    auto* text = mUi->availableListWidget->selectedItems()[0];
    mUi->activeListWidget->removeItemWidget(text);
    setButtonStates();
}

void ScriptDialog::setButtonStates() {
    if (!mUi->activeListWidget->count()) {
        mUi->allInactivePushButton->setDisabled(true);
        mUi->makeInactivePushButton->setDisabled(true);
    } else {
        mUi->allInactivePushButton->setEnabled(true);
        mUi->makeInactivePushButton->setEnabled(mUi->activeListWidget->selectedItems().count());
    }

    if (mUi->activeListWidget->count() == mUi->availableListWidget->count()) {
        mUi->allActivePushButton->setDisabled(true);
        mUi->makeActivePushButton->setDisabled(true);
    } else {
        mUi->allActivePushButton->setEnabled(true);
        if (mUi->availableListWidget->selectedItems().count()) {
            QString selected = mUi->availableListWidget->selectedItems()[0]->text();
            int num = mUi->activeListWidget->count();
            for (auto i = 0; i < num; ++i) {
                if (selected == mUi->activeListWidget->item(i)->text()) {
                    mUi->makeActivePushButton->setDisabled(true);
                    return;
                }
            }
            mUi->makeActivePushButton->setEnabled(true);
        } else mUi->makeActivePushButton->setDisabled(true);
    }
}

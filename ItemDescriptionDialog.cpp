#include "ItemDescriptionDialog.h"
#include "ui_ItemDescriptionDialog.h"

#include <QKeyEvent>
#include <QPushButton>

#include "Novel.h"

ItemDescriptionDialog::ItemDescriptionDialog(Item* item, QWidget* parent)
    : QDialog(parent)
    , mItem(item)
    , mUi(new Ui::ItemDescriptionDialog) {
    mUi->setupUi(this);

    mUi->nameLineEdit->setText(item->name());
    addTags(mItem->tags());

    setupConnections();

    noDefault(mUi->buttonBox->button(QDialogButtonBox::Ok));
    noDefault(mUi->buttonBox->button(QDialogButtonBox::Cancel));
}

ItemDescriptionDialog::~ItemDescriptionDialog() {
    delete mUi;
}

void ItemDescriptionDialog::keyPressEvent(QKeyEvent* event) {
    // Swallow Return/Enter so it doesn't trigger accept()
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        event->ignore();
        return;
    }
    QDialog::keyPressEvent(event);
}

void ItemDescriptionDialog::doButtonPressed() {
    removeButton(dynamic_cast<QPushButton*>(sender()));
}

void ItemDescriptionDialog::doEdited(const QString& t) {
    mItem->setName(t);
}

void ItemDescriptionDialog::doReturnPressed() {
    QLineEdit* edit = mUi->tagLineEdit;
    QString text = edit->text();
    if (text.isEmpty()) return;

    edit->setText("");

    mItem->addTag(text);
    for (auto i = 0; i < mButtons.size(); ++i) delete mButtons[i];
    mButtons.clear();

    addTags(mItem->tags());
}

void ItemDescriptionDialog::addButton(int num, const QString& tag) {
    QPushButton* button = new QPushButton(tag, mUi->tagFrame);

    int x = (num % 3) * button->geometry().width() + (num % 3 + 1) * 6;
    int y = (num / 3) * button->geometry().height() + (num / 3 + 1) * 6;
    QRect g = button->geometry();
    g.setX(x);
    g.setY(y);
    button->setGeometry(g.x(), g.y(), g.x() + g.width(), g.y() + g.height());

    connect(button, &QPushButton::clicked, this, &ItemDescriptionDialog::buttonPressedAction);
    mButtons.append(button);
    button->setEnabled(true);
    button->setVisible(true);
    repaint();
}

void ItemDescriptionDialog::addTags(const StringList &tags) {
    int i = 0;
    for (const auto& tag: tags) addButton(i++, tag);
}

void ItemDescriptionDialog::noDefault(QPushButton* btn) {
    btn->setDefault(false);
    btn->setAutoDefault(false);
}

void ItemDescriptionDialog::removeButton(const QPushButton* button) {
    mItem->clearTag(button->text());

    for (auto i = 0; i < mButtons.size(); ++i) delete mButtons[i];;
    mButtons.clear();

    addTags(mItem->tags());
}

void ItemDescriptionDialog::setupConnections() {
    connect(mUi->nameLineEdit, &QLineEdit::textEdited,    this, &ItemDescriptionDialog::editedAction);
    connect(mUi->nameLineEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
    connect(mUi->tagLineEdit,  &QLineEdit::returnPressed, this, &ItemDescriptionDialog::returnPressedAction);

    connect(mUi->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mUi->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

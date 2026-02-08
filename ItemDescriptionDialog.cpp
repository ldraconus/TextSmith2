#include "ItemDescriptionDialog.h"
#include "ui_ItemDescriptionDialog.h"
#include "Main.h"

#include <QKeyEvent>
#include <QPushButton>

#include "Novel.h"

static constexpr auto AreaWidth = 382;

ItemDescriptionDialog::ItemDescriptionDialog(Item* item, QWidget* parent)
    : QDialog(parent)
    , mItem(item)
    , mUi(new Ui::ItemDescriptionDialog) {
    mUi->setupUi(this);

    QFont font(Main::ref().prefs().uiFontFamily(), Main::ref().prefs().uiFontSize());
    Main::ref().prefs().applyFontToTree(this, font);

    mUi->nameLineEdit->setText(item->name());
    mUi->nameLineEdit->selectAll();

    addTags(font, mItem->tags());

    setupConnections();

    noDefault(mUi->buttonBox->button(QDialogButtonBox::Ok));
    noDefault(mUi->buttonBox->button(QDialogButtonBox::Cancel));

    Main::ref().prefs().applyFontToTree(this, font);
    updateGeometry();
    repaint();
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
    QPushButton* btn = dynamic_cast<QPushButton*>(sender());
    removeButton(btn);
}

void ItemDescriptionDialog::doEdited(const QString& t) {
    mItem->setName(t);
}

void ItemDescriptionDialog::doReturnPressed() {
    QLineEdit* edit = mUi->tagLineEdit;
    QString text = edit->text();
    if (text.isEmpty()) {
        accept();
        return;
    }

    edit->setText("");

    mItem->addTag(text);
    mUi->tagFrame->setUpdatesEnabled(false); // Freeze painting
    for (auto i = 0; i < mButtons.size(); ++i) mButtons[i]->deleteLater();
    mButtons.clear();
    mUi->tagFrame->setUpdatesEnabled(true); // Allow painting

    QFont font(Main::ref().prefs().uiFontFamily(), Main::ref().prefs().uiFontSize());
    addTags(font, mItem->tags());
}

void ItemDescriptionDialog::addButton(const QFont& font, const QString& tag) {
    const int BTN_HEIGHT = 30;
    const int SPACING = 3;         // Space between buttons
    const int PADDING = 20;         // Internal padding for text inside the button

    int max_container_width = 382;
    int base_btn_width = max_container_width / 3 - SPACING * 2;  // Width of a "single" button
    // 1. Calculate the required width based on text
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(tag); // Use fm.width(text) for older Qt versions

    int buttonWidth = base_btn_width;

    // Logic: Span 2 or 3 spaces if text is too wide
    int oneUnit = base_btn_width;
    int twoUnits = (base_btn_width * 2) + SPACING;
    int threeUnits = (base_btn_width * 3) + (SPACING * 2);

    if (textWidth + PADDING > oneUnit) {
        if (textWidth + PADDING > twoUnits) {
            buttonWidth = threeUnits; // Max cap
        } else {
            buttonWidth = twoUnits;
        }
    }

    // 2. Check if we need to wrap to the next line
    // If adding this button exceeds the container width...
    if (mX + buttonWidth > max_container_width) {
        mX = 0; // Reset X (or set to your left margin)
        mY += BTN_HEIGHT + SPACING; // Move Y down
    }

    // Optional: Check if we have run out of vertical space
    // if (currentY + BTN_HEIGHT > 200) { return; }

    // 3. Create and position the button
    QPushButton* btn = new QPushButton(tag, mUi->tagFrame);
    connect(btn, &QPushButton::clicked, this, &ItemDescriptionDialog::buttonPressedAction);

    btn->setGeometry(mX, mY, buttonWidth, BTN_HEIGHT);
    btn->show();

    // 4. Update the X position for the next button
    // Move X past this button + spacing
    mX += buttonWidth + SPACING;

    mButtons.append(btn);
}

void ItemDescriptionDialog::addTags(const QFont& font, const StringList &tags) {
    mX = mY = 0;
    for (const auto& tag: tags) addButton(font, tag);
}

void ItemDescriptionDialog::noDefault(QPushButton* btn) {
    btn->setDefault(false);
    btn->setAutoDefault(false);
}

void ItemDescriptionDialog::removeButton(const QPushButton* button) {
    mItem->clearTag(button->text());

    mUi->tagFrame->setUpdatesEnabled(false); // Freeze painting
    for (auto i = 0; i < mButtons.size(); ++i) mButtons[i]->deleteLater();
    mUi->tagFrame->setUpdatesEnabled(true); // Allow painting
    mButtons.clear();

    QFont font(Main::ref().prefs().uiFontFamily(), Main::ref().prefs().uiFontSize());
    addTags(font, mItem->tags());
}

void ItemDescriptionDialog::setupConnections() {
    connect(mUi->nameLineEdit, &QLineEdit::textEdited,    this, &ItemDescriptionDialog::editedAction);
    connect(mUi->nameLineEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
    connect(mUi->tagLineEdit,  &QLineEdit::returnPressed, this, &ItemDescriptionDialog::returnPressedAction);

    connect(mUi->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mUi->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

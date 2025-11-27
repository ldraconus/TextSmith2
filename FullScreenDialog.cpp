#include "FullScreenDialog.h"
#include "ui_FullScreenDialog.h"

#include <QKeyEvent>

#include "Main.h"

FullScreenDialog::FullScreenDialog(Preferences& prefs, QWidget *parent)
    : QDialog(parent)
    , mPrefs(&prefs)
    , mUi(new Ui::FullScreenDialog) {
    mUi->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_TranslucentBackground, false);
}

FullScreenDialog::~FullScreenDialog() {
    delete mUi;
}

QString FullScreenDialog::html() {
    return mUi->textEdit->toHtml();
}

qlonglong FullScreenDialog::position() {
    return mUi->textEdit->textCursor().position();
}

void FullScreenDialog::setFont(const QFont &font) {
    mUi->textEdit->document()->setDefaultFont(font);
}

void FullScreenDialog::setHtml(const QString &html) {
    mUi->textEdit->setHtml(html);
}

void FullScreenDialog::setPosition(qlonglong pos) {
    auto cursor = mUi->textEdit->textCursor();
    cursor.setPosition(pos);
    mUi->textEdit->setTextCursor(cursor);
    mUi->textEdit->ensureCursorVisible();
}

void FullScreenDialog::keyPressEvent(QKeyEvent* evt) {
    QTextBlockFormat block;
    QTextCursor cursor;
    QTextCharFormat format;
    QTextDocument* doc { nullptr };
    qlonglong pos { 0 };

    switch (evt->key()) {
    case Qt::Key_Escape:
    case Qt::Key_F11:
        mCtrlJSeen = false;
        done(QDialog::Accepted);
        return;
    default:
        if (evt->modifiers() == Qt::ControlModifier) {
            if (mCtrlJSeen) {
                mCtrlJSeen = false;
                switch (evt->key()) {
                case Qt::Key_C:
                    cursor = mUi->textEdit->textCursor();
                    block = cursor.blockFormat();
                    block.setAlignment(Qt::AlignCenter);
                    cursor.setBlockFormat(block);
                    mUi->textEdit->setTextCursor(cursor);
                    return;
                case Qt::Key_F:
                    cursor = mUi->textEdit->textCursor();
                    block = cursor.blockFormat();
                    block.setAlignment(Qt::AlignJustify);
                    cursor.setBlockFormat(block);
                    mUi->textEdit->setTextCursor(cursor);
                    return;
                case Qt::Key_I:
                    cursor = mUi->textEdit->textCursor();
                    block = cursor.blockFormat();
                    block.setIndent(block.indent() + 1);
                    cursor.setBlockFormat(block);
                    mUi->textEdit->setTextCursor(cursor);
                    return;
                case Qt::Key_L:
                    cursor = mUi->textEdit->textCursor();
                    block = cursor.blockFormat();
                    block.setAlignment(Qt::AlignLeft);
                    cursor.setBlockFormat(block);
                    mUi->textEdit->setTextCursor(cursor);
                    return;
                case Qt::Key_O:
                    cursor = mUi->textEdit->textCursor();
                    block = cursor.blockFormat();
                    block.setIndent(block.indent() - 1);
                    cursor.setBlockFormat(block);
                    mUi->textEdit->setTextCursor(cursor);
                    return;
                case Qt::Key_R:
                    cursor = mUi->textEdit->textCursor();
                    block = cursor.blockFormat();
                    block.setAlignment(Qt::AlignRight);
                    cursor.setBlockFormat(block);
                    mUi->textEdit->setTextCursor(cursor);
                    return;
                }
            }
            switch (evt->key()) {
            case Qt::Key_B:
                cursor = mUi->textEdit->textCursor();
                format.setFontWeight(cursor.charFormat().fontWeight() == QFont::Bold ? 0 : QFont::Bold);
                cursor.mergeCharFormat(format);
                mUi->textEdit->setTextCursor(cursor);
                return;
            case Qt::Key_C:
                mUi->textEdit->copy();
                return;
            case Qt::Key_I:
                cursor = mUi->textEdit->textCursor();
                format.setFontItalic(!cursor.charFormat().fontItalic());
                cursor.mergeCharFormat(format);
                mUi->textEdit->setTextCursor(cursor);
                return;
            case Qt::Key_J:
            case Qt::Key_S:
                Main::ref().saveAction();
                return;
            case Qt::Key_U:
                cursor = mUi->textEdit->textCursor();
                format.setFontUnderline(!cursor.charFormat().fontUnderline());
                cursor.mergeCharFormat(format);
                mUi->textEdit->setTextCursor(cursor);
                return;
            case Qt::Key_V:
                mUi->textEdit->paste();
                doc = mUi->textEdit->document();
                cursor = mUi->textEdit->textCursor();
                pos = cursor.position();
                Main::changeDocumentFont(doc, doc->defaultFont());
                cursor.setPosition(pos);
                mUi->textEdit->setTextCursor(cursor);
                mUi->textEdit->ensureCursorVisible();
                mUi->textEdit->paste();
                return;
            case Qt::Key_X:
                mUi->textEdit->cut();
                return;
            }
        }
        QDialog::keyPressEvent(evt);
        return;
    }
}

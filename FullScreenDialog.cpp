#include "FullScreenDialog.h"
#include "ui_FullScreenDialog.h"

#include <QAbstractTextDocumentLayout>
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

    switch (mPrefs->theme()) {
    case 0: setLight();  break;
    case 1: setDark();   break;
    case 2: setSystem(); break;
    }

    mUi->label->setVisible(false);
}

FullScreenDialog::~FullScreenDialog() {
    delete mUi;
}

QString FullScreenDialog::html() {
    return mUi->textEdit->toHtml();
}

void FullScreenDialog::imageFix() {
    Main::ref().removeEmptyFirstBlock(mUi->textEdit);
    mUi->textEdit->document()->setTextWidth(mUi->textEdit->viewport()->width());
    mUi->textEdit->document()->documentLayout()->update();
    mUi->textEdit->update();
}

Json5Array FullScreenDialog::other() {
    Json5Array arr;
    QList<int> otr = mUi->splitter->sizes();
    for (auto& size: otr) arr.append(qlonglong(size));
    return arr;
}

qlonglong FullScreenDialog::position() {
    return mUi->textEdit->textCursor().position();
}

void FullScreenDialog::setFont(const QFont &font) {
    mUi->textEdit->document()->setDefaultFont(font);
}

void FullScreenDialog::setHtml(const QString &html) {
    TextEdit* text = mUi->textEdit;
    Preferences& prefs = Main::ref().prefs();
    QFont font(prefs.fontFamily(), prefs.fontSize());
    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 2 * lineHeight;
    QTextBlockFormat format;
    format.setTextIndent(indent);
    format.setBottomMargin(lineHeight);
    QTextCursor cursor(text->document());
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    text->setTextCursor(cursor);
    text->document()->setDefaultFont(font);
}

void FullScreenDialog::setOther(Json5Array& other) {
    QList<int> otr;
    for (auto& size: other) otr.append(size.toInt());
    mUi->splitter->setSizes(otr);
}

void FullScreenDialog::setPosition(qlonglong pos) {
    auto cursor = mUi->textEdit->textCursor();
    cursor.setPosition(pos);
    mUi->textEdit->setTextCursor(cursor);
    mUi->textEdit->ensureCursorVisible();
}

void FullScreenDialog::setSoundPool(SoundPool* soundPool) {
    mUi->textEdit->setSoundPool(soundPool);
}

void FullScreenDialog::keyPressEvent(QKeyEvent* evt) {
    QString report;
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
                mCtrlJSeen = true;
                return;
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
            case Qt::Key_W:
                Main::ref().novel().setHtml(Main::ref().currentNode(), mUi->textEdit->toHtml());
                Main::ref().wordCounts();
                report = QString("Total: %1 | Current Item %2 | Since Opening %3 | Since Last Count %4")
                             .arg(Main::ref().wordCount().total())
                             .arg(Main::ref().wordCount().currentItem())
                             .arg(Main::ref().wordCount().sinceOpened())
                             .arg(Main::ref().wordCount().sinceLastCounted());
                mUi->label->setText(report);
                mUi->label->setVisible(true);
                Main::ref().wordCount().setSinceLastCounted(0);
                mTimer.setSingleShot(true);
                connect(&mTimer, &QTimer::timeout, this, [this]() {
                    mUi->label->setVisible(false);
                });
                mTimer.start(30 * 1000);
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

void FullScreenDialog::setDark() {
    mUi->textEdit->setStyleSheet("QTextEdit { "
                                 "  background-color: lightgray;"
                                 "  color: black;"
                                 "}");
    mUi->label->setStyleSheet("QLabel {"
                              "  background-color: #888888;"
                              "  color: black;"
                              "}");
}

void FullScreenDialog::setLight() {
    mUi->textEdit->setStyleSheet("QTextEdit { "
                                 "  background-color: white;"
                                 "  color: black;"
                                 "}");
    mUi->label->setStyleSheet("QLabel {"
                              "  background-color: lightgray;"
                              "  color: black;"
                              "}");
}

void FullScreenDialog::setSystem() {
    if (mPrefs->isDark()) setDark();
    else setLight();
}

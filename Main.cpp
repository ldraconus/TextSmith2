#include "Main.h"
#include "ui_Main.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCompleter>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QScreen>
#include <QShortcut>
#include <QShowEvent>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTextBlock> // 531-495-6412
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextToSpeech>
#include <QThread>
#include <QTreeWidget>
#include <QWindow>

#include "FullScreenDialog.h"
#include "ItemDescriptionDialog.h"
#include "PreferencesDialog.h"

constexpr qlonglong second = 1000;

Main* Main::sMain = nullptr;

class EditItemCommand
    : public QUndoCommand {
public:
    EditItemCommand(QTreeWidgetItem* branch,
                    Item newItem,
                    Item oldItem)
        : mBranch(branch)
        , mNewItem(newItem)
        , mOldItem(oldItem) {
        setText("Insert Tree Item");
    }

    void undo() override {
        auto id = mOldItem.id();
        Item& item = Main::ref().novel().findItem(id);
        item.setName(mOldItem.name());
        item.setTags(mOldItem.tags());
        mBranch->setText(0, mOldItem.name());
    }

    void redo() override {
        auto id = mNewItem.id();
        Item& item = Main::ref().novel().findItem(id);
        item.setName(mNewItem.name());
        item.setTags(mNewItem.tags());
        mBranch->setText(0, mNewItem.name());
    }

private:
    QTreeWidgetItem* mBranch;
    Item             mNewItem;
    Item             mOldItem;
};

class InsertItemCommand
    : public QUndoCommand {
public:
    InsertItemCommand(TreeWidget* tree,
                      QTreeWidgetItem* parent,
                      int index,
                      QTreeWidgetItem* branch,
                      Item& item)
        : mTree(tree)
        , mParent(parent)
        , mIndex(index)
        , mBranch(branch)
        , mItem(item) {
        setText("Insert Tree Item");
    }

    void undo() override {
        if (mParent)
            mParent->removeChild(mBranch);
        else
            mTree->takeTopLevelItem(mTree->indexOfTopLevelItem(mBranch));
        Main::ref().novel().deleteItem(mItem.id());
    }

    void redo() override {
        if (mParent)
            mParent->insertChild(mIndex, mBranch);
        else
            mTree->insertTopLevelItem(mIndex, mBranch);
        Main::ref().novel().addItem(mItem);
    }

private:
    TreeWidget*      mTree;
    QTreeWidgetItem* mParent;
    int              mIndex;
    QTreeWidgetItem* mBranch;
    Item             mItem;
};

class MoveItemCommand
    : public QUndoCommand {
public:
    MoveItemCommand(QTreeWidgetItem* grandParent,
                    QTreeWidgetItem* parent,
                    int oldIndex,
                    int newIndex)
        : mGrandParent(grandParent)
        , mParent(parent)
        , mOldIndex(oldIndex)
        , mNewIndex(newIndex) {
        setText("Move Tree Item");
    }

    void redo() override {
        if (mGrandParent != nullptr) {
            auto* branch = mParent->takeChild(mOldIndex);
            mGrandParent->insertChild(mNewIndex, branch);
        } else {
            auto* branch = mParent->takeChild(mOldIndex);
            mParent->insertChild(mNewIndex, branch);
        }
    }

    void undo() override {
        if (mGrandParent != nullptr) {
            auto* branch = mGrandParent->takeChild(mNewIndex);
            mParent->insertChild(mOldIndex, branch);
        } else {
            auto* branch = mParent->takeChild(mNewIndex);
            mParent->insertChild(mOldIndex, branch);
        }
    }

private:
    QTreeWidgetItem* mGrandParent;
    QTreeWidgetItem* mParent;
    int              mOldIndex;
    int              mNewIndex;
};

class RemoveItemCommand
    : public QUndoCommand {
public:
    RemoveItemCommand(TreeWidget* tree,
                      QTreeWidgetItem* parent,
                      int index,
                      QTreeWidgetItem* branch,
                      Item& item)
        : mTree(tree)
        , mParent(parent)
        , mIndex(index)
        , mBranch(branch)
        , mItem(item) {
        setText("Remove Tree Item");
    }

    void redo() override {
        if (mParent)
            mParent->removeChild(mBranch);
        else
            mTree->takeTopLevelItem(mTree->indexOfTopLevelItem(mBranch));
        Main::ref().novel().deleteItem(mItem.id());
    }

    void undo() override {
        if (mParent)
            mParent->insertChild(mIndex, mBranch);
        else
            mTree->insertTopLevelItem(mIndex, mBranch);
        Main::ref().novel().addItem(mItem);
    }

private:
    TreeWidget*      mTree;
    QTreeWidgetItem* mParent;
    int              mIndex;
    QTreeWidgetItem* mBranch;
    Item             mItem;
};

void Main::closeEvent(QCloseEvent* ce) {
    if (mNovel.isChanged()) {
        mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave novel before exiting?",
                         [this]() { doSave(); },
                         [this]() { clearChanged(); },
                         [this]() { doNothing(); },
                         "Warning!");
        if (mNovel.isChanged()) {
            ce->ignore();
            return;
        }
    }
    mPrefs.setWindowLocation(geometry());
    ce->accept();
}

void Main::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    fitWindow();
}

void Main::doAboutToShowFileMenu() {
    mUi->actionSave->setEnabled(mNovel.isChanged());
}

void Main::doAboutToShowEditMenu() {
    if (focusWidget() == mUi->textEdit) {
        mUi->actionUndo->setEnabled(mUi->textEdit->document()->isUndoAvailable());
        mUi->actionRedo->setEnabled(mUi->textEdit->document()->isRedoAvailable());
        mUi->actionCopy->setEnabled(mUi->textEdit->textCursor().hasSelection());
        mUi->actionCut->setEnabled(mUi->textEdit->textCursor().hasSelection());
        mUi->actionPaste->setEnabled(mUi->textEdit->canPaste());
    } else {
        mUi->actionUndo->setEnabled(mUi->treeWidget->canUndo());
        mUi->actionRedo->setEnabled(mUi->treeWidget->canRedo());
        mUi->actionCopy->setEnabled(true);
        mUi->actionCut->setEnabled(mUi->treeWidget->currentItem()->parent() != nullptr);
        mUi->actionPaste->setEnabled(mUi->treeWidget->canPaste());
    }
}

void Main::doAboutToShowNovelMenu() {
    mUi->actionRemove_Item->setEnabled(!(mUi->treeWidget->currentItem() == mUi->treeWidget->topLevelItem(0)));
    mUi->actionMove_an_Item_Up->setEnabled(!nothingAbove());
    mUi->actionMove_Current_Item_Down->setEnabled(!nothingBelow());
    mUi->actionMove_Current_Item_Out->setEnabled(!nothingAbove());
}

void Main::doAboutToShowViewMenu() {
    mUi->actionRead_To_Me->setEnabled(mSpeechAvailable);
}

void Main::doAboutToShowHelpMenu() {
}

void Main::doAddItem() {
    Item item;
    item.newHtml();
    item.setName("");
    ItemDescriptionDialog dlg(&item, this);
    if (dlg.exec() == QDialog::Rejected) return;
    mNovel.addItem(item);
    QTreeWidgetItem* branch = mUi->treeWidget->currentItem();
    auto twItem = new QTreeWidgetItem;
    twItem->setText(0, item.name());
    twItem->setData(0, Qt::UserRole, item.id());
    mUi->treeWidget->saveUndo(new InsertItemCommand(mUi->treeWidget, branch, branch->childCount(), twItem, item));
    branch->addChild(twItem);
    branch->setExpanded(true);
    changed();
}

void Main::doBold() {
    auto cursor = mUi->textEdit->textCursor();
    QTextCharFormat format;
    if (cursor.charFormat().fontWeight() == QFont::Bold) {
        format.setFontWeight(QFont::Normal);
        setIcon(mUi->boldToolButton, false);
        setMenu(mUi->actionBold, false);
    } else {
        format.setFontWeight(QFont::Bold);
        setIcon(mUi->boldToolButton, true);
        setMenu(mUi->actionBold, true);
    }
    cursor.mergeCharFormat(format);
    mUi->textEdit->setTextCursor(cursor);
    changed();
}

void Main::doCenterJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignCenter);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
    justifyButtons();
    changed();
}

void Main::doCloseAll() {
    workTree(mUi->treeWidget->topLevelItem(0), false);
    mUi->treeWidget->setCurrentItem(mUi->treeWidget->topLevelItem(0));
}

void Main::doCopy() {
    QWidget *w = QApplication::focusWidget();

    if (w == mUi->textEdit || mUi->textEdit->isAncestorOf(w)) {
        auto cursor = mUi->textEdit->textCursor();
        if (cursor.hasSelection()) mUi->textEdit->copy();
        return;
    }

    if (w == mUi->treeWidget || mUi->treeWidget->isAncestorOf(w)) {
        copyTreeItem();
        return;
    }
}

void Main::doCursorPositionChanged() {
    auto cursor = mUi->textEdit->textCursor();
    setIcon(mUi->boldToolButton,         cursor.charFormat().fontWeight() == QFont::Bold);
    setMenu(mUi->actionBold,             cursor.charFormat().fontWeight() == QFont::Bold);
    setIcon(mUi->underlineToolButton,    cursor.charFormat().fontUnderline());
    setMenu(mUi->actionUnderline,        cursor.charFormat().fontUnderline());
    setIcon(mUi->italicToolButton,       cursor.charFormat().fontItalic());
    setMenu(mUi->actionItalic,           cursor.charFormat().fontItalic());
    justifyButtons();
}

void Main::doCut() {
    QWidget *w = QApplication::focusWidget();

    if (w == mUi->textEdit || mUi->textEdit->isAncestorOf(w)) {
        auto cursor = mUi->textEdit->textCursor();
        if (cursor.hasSelection()) {
            mUi->textEdit->cut();
            changed();
        }
        return;
    }

    if (w == mUi->treeWidget || mUi->treeWidget->isAncestorOf(w)) {
        cutTreeItem();
        changed();
        return;
    }
}

void Main::doEditItem() {
    Item& current = mNovel.findItem(mCurrentNode);
    Item item(current);
    Item save(current);
    ItemDescriptionDialog dlg(&item, this);
    if (dlg.exec() == QDialog::Rejected) return;
    current.setName(item.name());
    current.setTags(item.tags());
    QTreeWidgetItem* branch = mUi->treeWidget->currentItem();
    branch->setText(0, current.name());
    mUi->treeWidget->saveUndo(new EditItemCommand(branch, save, current));
    changed();
}

void Main::doExit() {
    close();
}

void Main::doFindNext() {
}

void Main::doFindReplace() {
}

void Main::doFullJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignJustify);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
    justifyButtons();
    changed();
}

void Main::doFullScreen() {
    FullScreenDialog dlg(mPrefs, this);
    dlg.setHtml(mUi->textEdit->toHtml());
    dlg.setFont(mUi->textEdit->document()->defaultFont());
    dlg.setPosition(mUi->textEdit->textCursor().position());
    Json5Array arr = mPrefs.otherSplitter();
    if (arr.size() >= 2) dlg.setOther(arr);
    dlg.showFullScreen();
    dlg.exec();
    Json5Array other = dlg.other();
    mPrefs.setOtherSplitter(other);
    setHtml(dlg.html());
    setPosition(dlg.position());
    doCursorPositionChanged();
}

void Main::doHighlight(const QString& text, qlonglong start) {
    auto cursor = mUi->textEdit->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(start + text.length(), QTextCursor::KeepAnchor);
    mUi->textEdit->setTextCursor(cursor);}

void Main::doIndent() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setIndent(block.indent() + 1);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
    changed();
}

void Main::doItalic() {
    auto cursor = mUi->textEdit->textCursor();
    QTextCharFormat format;
    if (!cursor.charFormat().fontItalic()) {
        format.setFontItalic(true);
        setIcon(mUi->italicToolButton, true);
        setMenu(mUi->actionItalic, true);
    } else {
        format.setFontItalic(false);
        setIcon(mUi->italicToolButton, false);
        setMenu(mUi->actionItalic, false);
    }
    cursor.mergeCharFormat(format);
    mUi->textEdit->setTextCursor(cursor);
    changed();
}

void Main::doItemChanged(QTreeWidgetItem* current) {
    if (current == nullptr) return;
    auto& oldItem = mNovel.findItem(mCurrentNode);
    QString html = mUi->textEdit->toHtml();
    oldItem.setHtml(html);
    oldItem.count();
    oldItem.setPosition(mUi->textEdit->textCursor().position());
    mCurrentNode = current->data(0, Qt::UserRole).toLongLong();
    auto& item = mNovel.findItem(mCurrentNode);
    mPosition = item.position();
    setHtml(item.html());
    doCursorPositionChanged();
    changed();
}

void Main::doLeftJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignLeft);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
    justifyButtons();
    changed();
}

void Main::doLowercase() {
    auto cursor = mUi->textEdit->textCursor();
    auto text = cursor.selectedText();
    if (text.isEmpty()) return;
    busy();
    text = text.toLower();
    replaceText(cursor, text);
    ready();
    changed();
}

void Main::doMoveDown() {
    if (nothingBelow()) return;
    auto* root = mUi->treeWidget->topLevelItem(0);
    auto* treeItem = findItem(root, mCurrentNode);
    auto* parent = treeItem->parent();
    auto oldIdx = parent->indexOfChild(treeItem);
    auto idxOther = oldIdx + 1;
    auto* otherTreeItem = parent->child(idxOther);
    QTreeWidgetItem* grandParent = nullptr;
    if (oldIdx == parent->childCount() - 1) {
        if (parent == mUi->treeWidget->topLevelItem(0)) return;
        parent->takeChild(oldIdx);
        grandParent = parent->parent();
        idxOther = grandParent->indexOfChild(parent) + 1;
        grandParent->insertChild(idxOther, otherTreeItem);
    } else {
        parent->takeChild(oldIdx);
        parent->insertChild(idxOther, treeItem);
    }
    mUi->treeWidget->setCurrentItem(treeItem);
    mUi->treeWidget->saveUndo(new MoveItemCommand(grandParent, parent, oldIdx, idxOther));
}

void Main::doMoveOut() {
    if (parentIsRoot()) return;
    auto* root = mUi->treeWidget->topLevelItem(0);
    auto* treeItem = findItem(root, mCurrentNode);
    auto* parent = treeItem->parent();
    auto idx = parent->indexOfChild(treeItem);
    if (parent == mUi->treeWidget->topLevelItem(0)) return;
    parent->takeChild(idx);
    auto* grandParent = parent->parent();
    auto parentIdx = grandParent->indexOfChild(parent);
    grandParent->insertChild(parentIdx, treeItem);
    mUi->treeWidget->saveUndo(new MoveItemCommand(grandParent, parent, idx, parentIdx));
    mUi->treeWidget->setCurrentItem(treeItem);
}

void Main::doMoveUp() {
    if (nothingAbove()) return;
    auto* root = mUi->treeWidget->topLevelItem(0);
    auto* treeItem = findItem(root, mCurrentNode);
    auto* parent = treeItem->parent();
    auto idx = parent->indexOfChild(treeItem);
    auto idxOther = idx - 1;
    QTreeWidgetItem* grandParent = nullptr;
    if (idx == 0) {
        if (parent == mUi->treeWidget->topLevelItem(0)) return;
        grandParent = parent->parent();
        parent->takeChild(idx);
        idxOther = grandParent->indexOfChild(parent);
        grandParent->insertChild(idxOther, treeItem);
    } else {
        parent->takeChild(idx);
        parent->insertChild(idxOther, treeItem);
    }
    mUi->treeWidget->saveUndo(new MoveItemCommand(grandParent, parent, idx, idxOther));
    mUi->treeWidget->setCurrentItem(treeItem);
}

void Main::doNew() {
    if (mNovel.isChanged()) {
        if (mNovel.isChanged()) {
            mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave this novel before starting a new Novel?",
                             [this]() { doSave(); },
                             [this]() { clearChanged(); },
                             [this]() { doNothing(); },
                             "Warning!");
            if (mNovel.isChanged()) return;
        }
    }

    mNovel.clear();
    mPosition = 0;
    mState.clear();
    mCurrentNode = mNovel.root();
    mUi->treeWidget->clearUndo();
    clearChanged();
    update();
    mWordCount.setCurrentItem(0);
    mWordCount.setTotal(0);
    mWordCount.setSinceOpened(0);
    mWordCount.setSinceLastCounted(0);
    doCursorPositionChanged();
}

void Main::doOpenAll() {
    workTree(mUi->treeWidget->topLevelItem(0), true);
    mUi->treeWidget->setCurrentItem(mUi->treeWidget->topLevelItem(0));
}

void Main::doOpen() {
    if (mNovel.isChanged()) {
        mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave this novel before working a different Novel?",
                         [this]() { doSave(); },
                         [this]() { clearChanged(); },
                         [this]() { doNothing(); },
                         "Warning!");
        if (mNovel.isChanged()) return;
    }

    QString filename;
    if (mNovel.filename().isEmpty()) filename = QFileDialog::getOpenFileName(this, "Open Which Novel?", mDocDir,
                                                                             "Novels (*.novel);;All Files (*.*)");
    else filename = mNovel.filename();

    QFileInfo info(filename);
    mDocDir = info.absolutePath();

    busy();
    mNovel.clear();
    mPosition = 0;
    mState.clear();
    mNovel.setFilename(filename);
    mNovel.open();
    mUi->treeWidget->clearUndo();
    clearChanged();
    Json5Object obj = mNovel.extra();
    if (obj.contains("Prefs")) {
        Json5Object prefs = Item::hasObj(obj, "Prefs", {} );
        mPrefs.read(prefs);
        mCurrentNode = Item::hasNum(obj, "Current", -1);
        mPosition = Item::hasNum(obj, "Position", 0);
        Json5Array array = Item::hasArr(obj, "State");
        mState.clear();
        for (auto& state: array) {
            if (!state.isArray()) continue;
            auto& item = state.toArray();
            int id = Item::hasNum(item, 0);
            bool expanded = Item::hasBool(item, 1);
            mState[id] = expanded;
        }
        Json5Object images = Item::hasObj(obj, "Images", {});
        for (auto& image: images) {
            if (!image.second.isString()) continue;
            QUrl url = image.first;
            if (url.isEmpty()) continue;
            QString str = image.second.toString();
            if (str.isEmpty()) continue;
            QByteArray data = QByteArray::fromBase64(str.toUtf8());
            if (data.isEmpty()) continue;
            QImage img;
            img.loadFromData(data);
            mUi->textEdit->addInternalImage(url, img);
        }
    } else if (obj.contains("v1")) {
        Json5Object prefs = Item::hasObj(obj, "Prefs", {} );
        mPrefs.read(prefs);
        mCurrentNode = mNovel.root();
        mPosition = 0;
        mState.clear();
        bool state = Item::hasBool(obj, "State", false);
        Json5Array ids = Item::hasArr(obj, "Ids", {});
        for (auto& id: ids) mState[id.toInt()] = state;
    }

    update();
    doCursorPositionChanged();
    auto total = mNovel.countAll();
    mWordCount.setSinceLastCounted(0);
    mWordCount.setSinceOpened(0);
    mWordCount.setTotal(total);
    ready();
}

void Main::doOutdent() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setIndent(block.indent() - 1);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
    changed();
}

void Main::doPaste() {
    QWidget *w = QApplication::focusWidget();

    if (w == mUi->textEdit || mUi->textEdit->isAncestorOf(w)) {
        mUi->textEdit->paste();
        auto* doc = mUi->textEdit->document();
        auto pos = mUi->textEdit->textCursor().position();
        changeDocumentFont(doc, doc->defaultFont());
        setPosition(pos);
        mUi->textEdit->ensureCursorVisible();
        changed();
        return;
    }

    if (w == mUi->treeWidget || mUi->treeWidget->isAncestorOf(w)) {
        pasteTreeItem();
        return;
    }
}

void Main::doPreferences() {
    Preferences prefs(mPrefs);
    PreferencesDialog dlg(mPrefs, this);
    if (dlg.exec() == QDialog::Rejected) mPrefs = prefs;
    updateFromPrefs();
}

void Main::doReadToMe() {
    if (!mSpeechAvailable) return;

    QTextCursor cursor = mUi->textEdit->textCursor();
    if (cursor.hasSelection()) mTextToSpeak = cursor.selectedText();
    else mTextToSpeak = mUi->textEdit->toPlainText();

    if (mTextToSpeak.size() == 0) return;

    mStopButton->setVisible(true);
    mSavedCursor = cursor;

    mSpeech.setVoice(mPrefs.voice());
    QThread* thread = QThread::create([this]() {
        mSpeech.speak(mTextToSpeak);
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void Main::doRedo() {
    if (focusWidget() == mUi->textEdit) mUi->textEdit->redo();
    else mUi->treeWidget->undo();
}

void Main::doRemoveItem() {
    if (mCurrentNode == mNovel.root()) return;

    auto node = mCurrentNode;
    auto nvlItem = mNovel.findItem(node);

    mNovel.deleteItem(mCurrentNode);

    auto* item = mUi->treeWidget->currentItem();
    auto* parent = item->parent();
    auto idx = parent->indexOfChild(item);
    auto* next = parent->childCount() > idx + 1 ? parent->child(idx + 1) : nullptr;
    auto* prev = idx != 0 ? parent->child(idx - 1) : nullptr;
    if (!next) {
        if (prev) {
            mUi->treeWidget->setCurrentItem(prev);
            mCurrentNode = prev->data(0, Qt::UserRole).toLongLong();
        } else {
            mUi->treeWidget->setCurrentItem(parent);
            mCurrentNode = parent->data(0, Qt::UserRole).toLongLong();
        }
    } else {
        mUi->treeWidget->setCurrentItem(next);
        mCurrentNode = next->data(0, Qt::UserRole).toLongLong();
    }
    mUi->treeWidget->saveUndo(new RemoveItemCommand(mUi->treeWidget, parent, idx, item, nvlItem));
    parent->removeChild(item);
    changed();
}

void Main::doRightJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignRight);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
    justifyButtons();
    changed();
}

void Main::doSave() {
    if (mSaving.exchange(true)) return;

    mNovel.setHtml(mCurrentNode, mUi->textEdit->toHtml());
    Map<qlonglong, bool> byId;
    mapTree(byId, mUi->treeWidget->topLevelItem(0));
    save(mNovel, byId, mUi->textEdit->textCursor().position(), geometry());
    mSaving = false;
}

bool Main::doSaveAs() {
    QString filename = QFileDialog::getSaveFileName(this, "Save the Novel as?", mDocDir, "Novels (*.novel);;All Files (*.*)");
    if (filename.isEmpty()) return false;
    mNovel.setFilename(filename);
    changed();
    doSave();
    return true;
}

void Main::doStop(bool stopped) {
    QTextCursor cursor(mUi->textEdit->document());
    if (mSavedCursor.hasSelection()) {
        auto cursor = mUi->textEdit->textCursor();
        cursor.setPosition(mSavedCursor.selectionStart());
        cursor.setPosition(mSavedCursor.selectionEnd());
        mUi->textEdit->setTextCursor(cursor);
    } else setPosition(mSavedCursor.position());
    mStopButton->setVisible(false);
    if (!stopped) mSpeech.stop();
}

void Main::doTextChanged() {
    changed();
}

void Main::doUnderline() {
    auto cursor = mUi->textEdit->textCursor();
    QTextCharFormat format;
    if (!cursor.charFormat().fontUnderline()) {
        format.setFontUnderline(true);
        setIcon(mUi->underlineToolButton, true);
        setMenu(mUi->actionUnderline, true);
    } else {
        format.setFontUnderline(false);
        setIcon(mUi->underlineToolButton, false);
        setMenu(mUi->actionUnderline, false);
    }
    cursor.mergeCharFormat(format);
    mUi->textEdit->setTextCursor(cursor);
    changed();
}

void Main::doUndo() {
    if (focusWidget() == mUi->textEdit) mUi->textEdit->undo();
    else mUi->treeWidget->undo();
}

void Main::doUppercase() {
    auto cursor = mUi->textEdit->textCursor();
    auto text = cursor.selectedText();
    if (text.isEmpty()) return;
    busy();
    text = text.toUpper();
    replaceText(cursor, text);
    ready();
    changed();
}

void Main::doWordCount() {
    mNovel.setHtml(mCurrentNode, mUi->textEdit->toHtml());
    wordCounts();
    auto report = QString("Total: %1 | Current Item %2 | Since Opening %3 | Since Last Count %4")
                      .arg(mWordCount.total())
                      .arg(mWordCount.currentItem())
                      .arg(mWordCount.sinceOpened())
                      .arg(mWordCount.sinceLastCounted());
    mUi->statusbar->showMessage(report, 30 * 1000);
}

void Main::buildTree(const TreeNode& branch, QTreeWidgetItem* itemBranch, Map<qlonglong, bool>& byId) {
    auto twItem = new QTreeWidgetItem();
    auto& item = mNovel.findItem(branch.id());
    twItem->setText(0, item.name());
    twItem->setData(0, Qt::UserRole, item.id());

    if (itemBranch == nullptr) mUi->treeWidget->addTopLevelItem(twItem);
    else itemBranch->addChild(twItem);

    const auto& children = const_cast<TreeNode&>(branch).branches();
    for (auto& child: children) buildTree(child, twItem, byId);

    if (byId.contains(item.id())) twItem->setExpanded(byId[item.id()]);
    else twItem->setExpanded(true);
}

void Main::buildTreeMimeData(const QList<QTreeWidgetItem*>& branch, QMimeData* mimeData) {
    Json5Object obj = treeOfItems(branch[0]); // single selection mode in tree!
    QString str = obj.toJson5(Json5Object::Compress);
    QByteArray data = str.toUtf8();
    mimeData->setData("application/json5", data);
    QTextEdit document;
    auto list = vectorOfItems(branch[0]); // single selection mode in tree!
    auto cursor = document.textCursor();
    bool first = true;
    Item* prev = nullptr;
    for (auto& item: list) {
        if (first) first = false;
        else {
            QString text = prev->toPlainText();
            if (text.last(1) != "\n") cursor.insertText("\n");
        }
        cursor.insertHtml(item->html());
        cursor.movePosition(QTextCursor::End);
        prev = item;
    }
    mimeData->setHtml(document.toHtml());
    mimeData->setText(document.toPlainText());
}

bool Main::canPasteMimeData(const QMimeData* mimeData) {
    return mimeData->hasFormat("application/json5") || mimeData->hasHtml() || mimeData->hasText();
}

QString Main::checked(const QString& path) {
    StringList parts(path.split("."));
    parts[0] += "Ck";
    return parts.join(".");
}

QTreeWidgetItem* Main::findItem(QTreeWidgetItem* tree, qlonglong node) {
    if (tree == nullptr) return nullptr;
    if (auto id = tree->data(0, Qt::UserRole).toLongLong(); id == node) return tree;
    else if (tree->childCount()) {
        for (auto i = 0; i < tree->childCount(); ++i) {
            if (auto* found = findItem(tree->child(i), node); found != nullptr) return found;
        }
    }
    return nullptr;
}

void Main::fitWindow() {
    if (windowHandle() == nullptr) return;

    QRect geom = mPrefs.windowLocation();
    QScreen* windowScreen = nullptr;
    if (geom.isValid()) {
        for (auto* s: QGuiApplication::screens()) {
            if (s->geometry().intersects(geom)) {
                windowScreen = s;
                break;
            }
        }
    }
    bool needsCenter = false;
    if ((needsCenter = windowScreen == nullptr)) windowScreen = QGuiApplication::primaryScreen();

    if (!geom.isValid()) geom.setSize({ 1024, 800 });

    QRect screenGeom = windowScreen->availableGeometry();
    if (!windowScreen->geometry().contains(geom.topLeft())) geom.moveTo(screenGeom.topLeft());
    if (!windowScreen->geometry().contains(geom.bottomRight())) geom.setSize(screenGeom.size());
    if (needsCenter) {
        QPoint centerPos = screenGeom.center() - QPoint(geom.width() / 2, geom.height() / 2);
        geom.moveTopLeft(centerPos);
    }
    geom = geom.intersected(screenGeom);

    move(geom.topLeft());
    resize(geom.size());

    QRect frame = frameGeometry();
    if (!frame.contains(geom)) geom = geom.intersected(frame);

    move(geom.topLeft());
    resize(geom.size());

    mPrefs.setWindowLocation(geom);
}

void Main::justifyButtons() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    setIcon(mUi->leftJustifyToolButton,    block.alignment() == Qt::AlignLeft);
    setMenu(mUi->actionLeft_Justify,       block.alignment() == Qt::AlignLeft);
    setIcon(mUi->centerToolButton,         block.alignment() == Qt::AlignCenter);
    setMenu(mUi->actionCenter,             block.alignment() == Qt::AlignCenter);
    setIcon(mUi->rightJustifyToolButton,   block.alignment() == Qt::AlignRight);
    setMenu(mUi->actionRight_Justify,      block.alignment() == Qt::AlignRight);
    setIcon(mUi->fullJustifyToolButton,    block.alignment() == Qt::AlignJustify);
    setMenu(mUi->actionFull_Justification, block.alignment() == Qt::AlignJustify);
}

void Main::copyTreeItem() {
    auto *tree = mUi->treeWidget;
    QTreeWidgetItem* item = tree->currentItem();
    if (!item) return;

    QList<QTreeWidgetItem*> items = { item };
    QMimeData* mime = tree->itemMimeData(items);

    QApplication::clipboard()->setMimeData(mime);
}

void Main::cutTreeItem() {
    auto *tree = mUi->treeWidget;
    QTreeWidgetItem* branch = tree->currentItem();
    if (!branch) return;

    auto index = branch->parent()->indexOfChild(branch);
    auto id = branch->data(0, Qt::UserRole).toLongLong();
    auto item = mNovel.findItem(id);
    mNovel.deleteItem(id);
    copyTreeItem();
    mUi->treeWidget->saveUndo(new RemoveItemCommand(mUi->treeWidget, branch->parent(), index, branch, item));
    delete branch;
}

void Main::mapTree(Map<qlonglong, bool>& byId, QTreeWidgetItem* item) {
    auto id = item->data(0, Qt::UserRole).toLongLong();
    bool open = item->isExpanded();
    byId[id] = open;
    for (int i = 0; i < item->childCount(); ++i) mapTree(byId, item->child(i));
}

bool Main::nothingAbove() {
    auto* item = findItem(mUi->treeWidget->topLevelItem(0), mCurrentNode);
    if (item == nullptr) return true;
    auto* parent = item->parent();
    if (parent == nullptr) return true;
    if (parent == mUi->treeWidget->topLevelItem(0)) return true;
    return false;
}

bool Main::nothingBelow() {
    auto* item = findItem(mUi->treeWidget->topLevelItem(0), mCurrentNode);
    if (item == nullptr) return true;
    auto* parent = item->parent();
    if (parent == nullptr) return true;
    if (parent == mUi->treeWidget->topLevelItem(0)) return true;
    auto itemIdx = parent->indexOfChild(item);
    if (itemIdx < parent->childCount() ) return false;
    auto* grandparent = parent->parent();
    if (grandparent == nullptr) return true;
    auto parentIndex = grandparent->indexOfChild(parent);
    if (parentIndex < grandparent->childCount()) return false;
    return true;
}

void Main::replaceText(QTextCursor cursor, const QString& text) {
    auto position = cursor.selectionStart();
    for (const auto& ch: text) {
        if (ch != '\n') {
            cursor.setPosition(position + 1);
            cursor.insertText(ch);
            cursor.setPosition(position);
            cursor.deleteChar();
        }
        ++position;
    }
}

bool Main::parentIsRoot() {
    auto* item = findItem(mUi->treeWidget->topLevelItem(0), mCurrentNode);
    if (item == nullptr) return true;
    auto* parent = item->parent();
    if (parent == nullptr) return true;
    if (parent == mUi->treeWidget->topLevelItem(0)) return true;
    return false;
}

void Main::pasteTreeItem() {
    auto *tree = mUi->treeWidget;
    const QMimeData* data = QApplication::clipboard()->mimeData();
    if (data) tree->insertFromMimeData(data);
}

bool Main::receiveTreeMimeData(QDropEvent* de, const QMimeData* mimeData) {
    auto* tree = mUi->treeWidget;
    Item item;
    TextEdit edit(nullptr);

    QTreeWidgetItem* parent = nullptr;
    int row = 0;
    QTreeWidgetItem* targetItem = nullptr;
    if (de == nullptr) targetItem = mUi->treeWidget->currentItem();
    else {
        QModelIndex idx = tree->indexAt(de->position().toPoint());
        if (idx.isValid()) targetItem = tree->itemFromIndex(idx);
    }

    if (targetItem != nullptr) {
        parent = targetItem->parent();

        if (parent == nullptr) parent = mUi->treeWidget->topLevelItem(0);
        row = parent->indexOfChild(targetItem);
        if (row == -1) row = 0;
    } else {
        parent = tree->topLevelItem(0);
        row = parent->childCount();
    }

    bool invalidJson = false;
    bool invalidText = true;
    QTreeWidgetItem* newBranch = nullptr;
    while (true) {
        if (mimeData->hasFormat("application/json5")) {
            QByteArray data = mimeData->data("application/json5");
            QString text(data);
            Json5Document doc(text);
            if (doc.top().isObject()) invalidJson = (newBranch = buildTreeFromJson(doc.top().toObject())) == nullptr;
            else invalidJson = true;
            break;
        } else if (mimeData->hasHtml() || mimeData->hasText()) {
            if (mimeData->hasHtml()) {
                QString html = mimeData->html();
                edit.setHtml(html);
            } else {
                QString text = mimeData->text();
                edit.setText(text);
            }
            invalidText = false;
            setupHtml(edit);
            Item item;
            item.setHtml(edit.toHtml());
            auto id = item.id();
            mNovel.addItem(item);
            QTreeWidgetItem* newBranch = new QTreeWidgetItem();
            newBranch->setText(0, item.name());
            newBranch->setData(0, Qt::UserRole, id);
            break;
        } else break;
    }
    if (invalidJson && invalidText) return false;

    if (parent != nullptr) {
        parent->insertChild(row, newBranch);
        mUi->treeWidget->saveUndo(new InsertItemCommand(mUi->treeWidget, parent, row, newBranch, item));
        changed();
    }

    return true;
}

void Main::removeEmptyFirstBlock(TextEdit* text) {
    QTextCursor c(text->document());
    c.movePosition(QTextCursor::Start);

    QTextBlock first = c.block();
    bool hasFragments = false;
    for (QTextBlock::iterator it = first.begin(); !(it.atEnd()); ++it) {
        QTextFragment frag = it.fragment();
        if (frag.isValid()) {
            hasFragments = true;
            break;
        }
    }

    bool trulyEmpty = !hasFragments && first.text().isEmpty();

    if (trulyEmpty) c.deleteChar();
}

void Main::save(Novel& novel, Map<qlonglong, bool>& byId, qlonglong pos, const QRect& geom, bool noUi) {
    if ((novel.filename().isEmpty() && noUi) || !novel.isChanged()) return;
    if (mNovel.filename().isEmpty() && !doSaveAs()) return;
    QFileInfo info(mNovel.filename());
    QString name = info.fileName();
    mDocDir = info.absolutePath();
    Json5Object extra;
    extra["Current"] = mCurrentNode;
    extra["Position"] = qlonglong(pos);
    mPrefs.setWindowLocation(geom);
    extra["Prefs"] = mPrefs.write();
    Json5Object images;
    auto arr = mUi->textEdit->serializeInternalImagesToJson();
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        QJsonObject from(it->toObject());
        QString url = from["url"].toString();
        QString data = from["data"].toString();
        images[url] = data;
    }
    extra["Images"] = images;
    mState = byId;
    Json5Array state;
    for (const auto& idState: byId) {
        Json5Array item;
        item.append(idState.first);
        item.append(idState.second);
        state.append(item);
    }
    extra["State"] = state;
    mNovel.setExtra(extra);
    TreeNode tree = saveTree(mUi->treeWidget->topLevelItem(0));
    mNovel.setBranches(tree);
    if (!mNovel.save() && !noUi) mMsg.OK("Unable to save the file.\n\nTry and save it under a different name\nor save it to a different directory.",
                                         [this]() { doNothing(); },
                                         "Something unexpected has happened");
}

TreeNode Main::saveTree(QTreeWidgetItem* node) {
    TreeNode part(node->data(0, Qt::UserRole).toLongLong());
    for (auto i = 0; i < node->childCount(); ++i) part.addBranch(saveTree(node->child(i)));
    return part;
}

void Main::setHtml(const QString& html) {
    mUi->textEdit->setHtml(html);
    removeEmptyFirstBlock(mUi->textEdit);

    mUi->textEdit->document()->setTextWidth(mUi->textEdit->viewport()->width());
    mUi->textEdit->document()->documentLayout()->update();
    mUi->textEdit->update();

    // Ensure cursor is visible
    auto maxPosition = mUi->textEdit->toPlainText().size();
    if (mPosition > maxPosition) mPosition = maxPosition;
    setPosition(mPosition);

    // Reset cursor formatting so it doesn't inherit image-drop formatting
    QTextCursor c = mUi->textEdit->textCursor();
    QTextCharFormat fmt;

    switch (mPrefs.theme()) {
    case 0: fmt.setForeground(QBrush(Qt::black)); break;
    case 1: fmt.setForeground(QBrush(Qt::white)); break;
    case 2:
        if (mPrefs.isDark()) fmt.setForeground(QBrush(Qt::white));
        else                 fmt.setForeground(QBrush(Qt::black));
        break;
    }

    fmt.setFontPointSize(mUi->textEdit->font().pointSizeF());
    c.setCharFormat(fmt);
    mUi->textEdit->setTextCursor(c);
}

void Main::setIcon(QToolButton* button, bool isChecked) {
    QString path = Main::ref().getIconPath(button->objectName());
    if (mPrefs.wasDark() != mPrefs.isDark()) path = mPrefs.newPath(path);
    if (isChecked) path = checked(path);
    QPixmap pm(path);
    QIcon icon;
    icon.addFile(path, QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    button->setIcon(icon);
}

void Main::setMenu(QAction* menuItem, bool isChecked) {
    menuItem->setChecked(isChecked);
}

void Main::setPosition(qlonglong pos) {
    auto cursor = mUi->textEdit->textCursor();
    cursor.setPosition(pos);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::update() { // new, open
    QTreeWidget* tree = mUi->treeWidget;
    tree->clear();
    buildTree(mNovel.branches(), nullptr, mState);
    updateHtml();
    updateFromPrefs();

    mUi->textEdit->setFocus();
}

void Main::updateFromPrefs() {
    if (mPrefs.autoSave()) mTimer.start(mPrefs.autoSaveIntyerval() * second);
    else mTimer.stop();

    QFont font(mPrefs.fontFamily(), mPrefs.fontSize());
    mUi->textEdit->setCurrentFont(font);
    mUi->textEdit->document()->setDefaultFont(font);
    changeDocumentFont(mUi->textEdit->document(), QFont(font));
    setPosition(mPrefs.position());
    mUi->textEdit->ensureCursorVisible();
    changeNovelFont(QFont(font));

    switch (mPrefs.theme()) {
    case 0: mPrefs.setLightTheme();  break;
    case 1: mPrefs.setDarkTheme();   break;
    case 2: mPrefs.setSystemTheme(); break;
    }

    if (mPrefs.mainSplitter().size() != 0) {
        List<int> arr;
        for (auto& m: mPrefs.mainSplitter()) arr.append(m.toInt());
        mUi->splitter->setSizes(arr.toQList());
    }

    mUi->actionRead_To_Me->setEnabled(mPrefs.voice() >= 0);
}

void Main:: updateHtml() { // update, and post-fullscreen
    QTreeWidget* tree = mUi->treeWidget;
    QTreeWidgetItem* branch = findItem(tree->topLevelItem(0), mCurrentNode);
    if (branch != nullptr) {
        Item& item = mNovel.findItem(mCurrentNode);
        setHtml(item.html());

        tree->setCurrentItem(branch);
    }
}

void Main::workTree(QTreeWidgetItem* item, bool expand) {
    item->setExpanded(expand);
    for (int i = 0; i < item->childCount(); ++i) workTree(item->child(i), expand);
}

Main::Main(QApplication* app, QWidget* parent)
    : QMainWindow(parent)
    , mApp(app)
    , mAppDir(QApplication::applicationDirPath())
    , mDocDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    , mLocalDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
    , mTimer(this)
    , mUi(new Ui::Main) {
    sMain = this;
    mUi->setupUi(this);

    mUi->treeWidget->setDragEnabled(true);
    mUi->treeWidget->setAcceptDrops(true);
    mUi->treeWidget->setDropIndicatorShown(true);
    mUi->treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    mUi->treeWidget->setMimeDataBuilder(std::bind(&Main::buildTreeMimeData, this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2));
    mUi->treeWidget->setMimeDataReceiver(std::bind(&Main::receiveTreeMimeData, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));
    mUi->treeWidget->setMimeCanPaste(std::bind(&Main::canPasteMimeData, this,
                                               std::placeholders::_1));

    mUi->textEdit->setAcceptDrops(true);

    QTextToSpeech* speech = new QTextToSpeech();
    mSpeechAvailable = !(speech == nullptr || speech->state() == QTextToSpeech::Error || speech->availableVoices().isEmpty());
    delete speech;

    mCurrentNode = mNovel.root();

    setupIcons();

    QDir().mkpath(mAppDir);
    QDir().mkpath(mDocDir);
    QDir().mkpath(mDocDir + "/TextSmith");
    QDir().mkpath(mLocalDir);

    mUi->actionRead_To_Me->setEnabled(mSpeechAvailable);
    if (mSpeechAvailable) {
        mStopButton = new QToolButton(mUi->statusbar);
        mStopButton->setText("■");
        mStopButton->setVisible(false);
        mUi->statusbar->addPermanentWidget(mStopButton);
    }

    setupActions();
    setupConnections();
    setupTabOrder();

    mPrefsLoaded = mPrefs.load();

    mUi->treeWidget->setColumnCount(1);
    mUi->textEdit->setTabChangesFocus(true);

    StringList arguments { app->arguments() };
    if (arguments.size() > 1) {
        mNovel.setFilename(arguments[1]);
        clearChanged();
        doOpen();
    } else {
        doNew();
        Item::resetLastID(1);
        clearChanged();
    }
}

Main::~Main() {
    Json5Array array;
    for (auto size: mUi->splitter->sizes()) array.append(qlonglong(size));
    mPrefs.setMainSplitter(array);
    delete mUi;
}

QTreeWidgetItem* Main::buildTreeFromJson(Json5Object& obj) {
    Item item;
    item.setHtml(Item::hasStr(obj, "Html"));
    item.setName(Item::hasStr(obj, "Name"));
    item.setPosition(Item::hasNum(obj, "Position", 0));
    if (Json5Array arr = Item::hasArr(obj, "Tags", {}); !arr.empty()) {
        for (auto& tag: arr) {
            if (tag.isString()) item.addTag(tag.toString());
        }
    }
    TextEdit html;
    html.setHtml(item.html());
    setupHtml(html);
    item.setHtml(html.toHtml());
    QTreeWidgetItem* branch = new QTreeWidgetItem();
    branch->setText(0, item.name());
    branch->setData(0, Qt::UserRole, item.id());
    mNovel.addItem(item);
    Json5Array arr = Item::hasArr(obj, "Children", {});
    for (auto& child: arr) {
        Json5Object data = child.isObject() ? child.toObject() : Json5Object();
        if (data.isEmpty()) continue;
        QTreeWidgetItem* sprout = buildTreeFromJson(data);
        if (sprout) branch->addChild(sprout);
    }
    return branch;
}

void Main::changeNovelFont(const QFont& font) {
    mNovel.changeFont(font);
}

void Main::setupHtml(TextEdit& text) {
    Preferences& prefs = Main::ref().prefs();
    QFont font(prefs.fontFamily(), prefs.fontSize());
    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 2 * lineHeight;
    QTextBlockFormat format;
    format.setTextIndent(indent);
    format.setBottomMargin(lineHeight);
    QTextCursor cursor(text.document());
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    text.setTextCursor(cursor);
    text.document()->setDefaultFont(font);
}

Json5Object Main::treeOfItems(QTreeWidgetItem* branch) {
    auto id = branch->data(0, Qt::UserRole).toLongLong();
    auto item = mNovel.findItem(id);
    Json5Object obj = item.toObject();
    for (auto idx = 0; idx < branch->childCount(); ++idx) {
        auto sprout = branch->child(idx);
        auto id = sprout->data(0, Qt::UserRole).toLongLong();
        auto item = mNovel.findItem(id);
        obj[QString::number(id)] = treeOfItems(sprout);
    }
    return obj;
}

QList<Item*> Main::vectorOfItems(QTreeWidgetItem* branch) {
    QList<Item*> items;
    auto id = branch->data(0, Qt::UserRole).toLongLong();
    Item& item = mNovel.findItem(id);
    items.append(&item);
    for (auto childNum = 0; childNum < branch->childCount(); ++childNum) {
        auto childItems = vectorOfItems(branch->child(childNum));
        items.append(childItems);
    }
    return items;
}

void Main::wordCounts() {
    auto oldCount = mWordCount.currentItem();
    auto newCount = mNovel.countAll();
    auto difference = newCount - oldCount;
    mWordCount.setCurrentItem(newCount);
    mWordCount.setTotal(mWordCount.total() + difference);
    mWordCount.setSinceOpened(mWordCount.sinceOpened() + difference);
    mWordCount.setSinceLastCounted(mWordCount.sinceLastCounted() + difference);
}

void Main::setupActions() {
    mActions.clear();
    mActions
        << mUi->actionSave
        << mUi->actionSave_As
        << mUi->actionAbout_TextSmith
        << mUi->actionas_Plain_text
        << mUi->actionas_EBook
        << mUi->actionas_PDF
        << mUi->actionas_RTF
        << mUi->actionAdd_Item
        << mUi->actionEdit_Item
        << mUi->actionMove_Current_Item_Down
        << mUi->actionMove_Current_Item_Out
        << mUi->actionMove_an_Item_Up
        << mUi->actionOpen_Current_Item
        << mUi->actionRemove_Item
        << mUi->actionOpen
        << mUi->actionOpen_All
        << mUi->actionBold
        << mUi->actionCenter
        << mUi->actionClose_All
        << mUi->actionCopy
        << mUi->actionCut
        << mUi->actionDistraction_Free
        << mUi->actionExit
        << mUi->actionFind_Next
        << mUi->actionFind_Replace
        << mUi->actionFull_Justification
        << mUi->actionHelp
        << mUi->actionIndent
        << mUi->actionItalic
        << mUi->actionLeft_Justify
        << mUi->actionLowercase
        << mUi->actionRight_Justify
        << mUi->actionNew
        << mUi->actionOpen_Current_Item
        << mUi->actionPaste
        << mUi->actionPrefereces
        << mUi->actionPrint
        << mUi->actionRead_To_Me
        << mUi->actionRedo
        << mUi->actionPrint
        << mUi->actionScripting
        << mUi->actionUndent
        << mUi->actionUnderline
        << mUi->actionUndo
        << mUi->actionUppercase
        << mUi->actionWeb_Site
        << mUi->actionWord_Count;
}

void Main::setupConnections() {
    mUi->actionExit->setShortcut(QKeySequence("Alt+F4"));
    connect(mUi->actionExit,    &QAction::triggered, this, &Main::exitAction);
    connect(mUi->actionNew,     &QAction::triggered, this, &Main::newAction);
    connect(mUi->actionOpen,    &QAction::triggered, this, &Main::openAction);
    connect(mUi->actionSave,    &QAction::triggered, this, &Main::saveAction);
    connect(mUi->actionSave_As, &QAction::triggered, this, &Main::saveAsAction);

    connect(mUi->actionBold,               &QAction::triggered, this, &Main::boldAction);
    connect(mUi->actionCenter,             &QAction::triggered, this, &Main::centerJustifyAction);
    connect(mUi->actionCopy,               &QAction::triggered, this, &Main::copyAction);
    connect(mUi->actionCut,                &QAction::triggered, this, &Main::cutAction);
    connect(mUi->actionFind_Replace,       &QAction::triggered, this, &Main::findReplace);
    connect(mUi->actionFull_Justification, &QAction::triggered, this, &Main::fullJustifyAction);
    connect(mUi->actionIndent,             &QAction::triggered, this, &Main::indentAction);
    connect(mUi->actionItalic,             &QAction::triggered, this, &Main::italicAction);
    connect(mUi->actionLeft_Justify,       &QAction::triggered, this, &Main::leftJustifyAction);
    connect(mUi->actionLowercase,          &QAction::triggered, this, &Main::lowerCaseAction);
    connect(mUi->actionUndent,             &QAction::triggered, this, &Main::outdentAction);
    connect(mUi->actionPaste,              &QAction::triggered, this, &Main::pasteAction);
    connect(mUi->actionPrefereces,         &QAction::triggered, this, &Main::preferencesAction);
    connect(mUi->actionRedo,               &QAction::triggered, this, &Main::redoAction);
    connect(mUi->actionRight_Justify,      &QAction::triggered, this, &Main::rightJustifyAction);
    connect(mUi->actionUnderline,          &QAction::triggered, this, &Main::underlineAction);
    connect(mUi->actionUndo,               &QAction::triggered, this, &Main::undoAction);
    connect(mUi->actionUppercase,          &QAction::triggered, this, &Main::uppercaseAction);

    connect(mUi->actionAdd_Item,               &QAction::triggered, this, &Main::addItemAction);
    connect(mUi->actionClose_All,              &QAction::triggered, this, &Main::closeAllAction);
    connect(mUi->actionEdit_Item,              &QAction::triggered, this, &Main::editItemAction);
    connect(mUi->actionMove_Current_Item_Down, &QAction::triggered, this, &Main::actionMoveDown);
    connect(mUi->actionMove_Current_Item_Out,  &QAction::triggered, this, &Main::actionMoveOut);
    connect(mUi->actionMove_an_Item_Up,        &QAction::triggered, this, &Main::actionMoveUp);
    connect(mUi->actionOpen_All,               &QAction::triggered, this, &Main::openAllAction);
    connect(mUi->actionRemove_Item,            &QAction::triggered, this, &Main::removeItemAction);

    connect(mUi->actionDistraction_Free, &QAction::triggered, this, &Main::fullScreenAction);
    connect(mUi->actionRead_To_Me,       &QAction::triggered, this, &Main::readToMeAction);
    connect(mUi->actionWord_Count,       &QAction::triggered, this, &Main::doWordCount);

    connect(mUi->newItemToolButton,    &QToolButton::clicked, this, &Main::addItemAction);
    connect(mUi->deleteItemToolButton, &QToolButton::clicked, this, &Main::removeItemAction);
    connect(mUi->downToolButton,       &QToolButton::clicked, this, &Main::actionMoveDown);
    connect(mUi->outToolButton,        &QToolButton::clicked, this, &Main::actionMoveOut);
    connect(mUi->upToolButton,         &QToolButton::clicked, this, &Main::actionMoveUp);

    connect(mUi->boldToolButton,           &QToolButton::clicked, this, &Main::boldAction);
    connect(mUi->centerToolButton,         &QToolButton::clicked, this, &Main::centerJustifyAction);
    connect(mUi->fullJustifyToolButton,    &QToolButton::clicked, this, &Main::fullJustifyAction);
    connect(mUi->leftJustifyToolButton,    &QToolButton::clicked, this, &Main::leftJustifyAction);
    connect(mUi->increaseIndentToolButton, &QToolButton::clicked, this, &Main::indentAction);
    connect(mUi->italicToolButton,         &QToolButton::clicked, this, &Main::italicAction);
    connect(mUi->leftJustifyToolButton,    &QToolButton::clicked, this, &Main::leftJustifyAction);
    connect(mUi->decreaseIndentToolButton, &QToolButton::clicked, this, &Main::outdentAction);
    connect(mUi->rightJustifyToolButton,   &QToolButton::clicked, this, &Main::rightJustifyAction);
    connect(mUi->underlineToolButton,      &QToolButton::clicked, this, &Main::underlineAction);

    connect(mUi->menuFile,  &QMenu::aboutToShow, this, &Main::aboutToShowFileMenuAction);
    connect(mUi->menuEdit,  &QMenu::aboutToShow, this, &Main::aboutToShowEditMenuAction);
    connect(mUi->menuNovel, &QMenu::aboutToShow, this, &Main::aboutToShowNovelMenuAction);
    connect(mUi->menuView,  &QMenu::aboutToShow, this, &Main::aboutToShowViewMenuAction);

    connect(mUi->treeWidget, &QTreeWidget::itemDoubleClicked,  this, &Main::doubleClickAction);
    connect(mUi->treeWidget, &QTreeWidget::currentItemChanged, this, &Main::currentItemChangedAction);

    connect(mUi->textEdit, &TextEdit::cursorPositionChanged, this, &Main::cursorPositionChanged);
    connect(mUi->textEdit, &TextEdit::textChanged,           this, &Main::textChangedAction);

    mUi->treeWidget->setAcceptDrops(true);
    mUi->treeWidget->setDragDropMode(QAbstractItemView::DragDrop);
    mUi->treeWidget->setDefaultDropAction(Qt::MoveAction);
    mUi->treeWidget->setMimeDataBuilder(std::bind(&Main::buildTreeMimeData,    this, std::placeholders::_1, std::placeholders::_2));
    mUi->treeWidget->setMimeDataReceiver(std::bind(&Main::receiveTreeMimeData, this, std::placeholders::_1, std::placeholders::_2));

    mUi->textEdit->setAcceptDrops(true);
    mUi->textEdit->setAcceptRichText(true);
    // create the custom drag event and the drop event handlers in TextEdit
    // set the mime data builder and receivers

    connect(mStopButton, &QToolButton::clicked,     this, &Main::stop);
    connect(&mSpeech,    &Speech::speaking,         this, &Main::highlight);
    connect(&mSpeech,    &Speech::speakingFinished, this, &Main::stopped);

    QLineEdit *commandLine = new QLineEdit;
    commandLine->setPlaceholderText("Type a command...");
    commandLine->hide();
    statusBar()->addPermanentWidget(commandLine);

    QShortcut* paletteShortcut = new QShortcut(QKeySequence("Ctrl+Shift+P"), this);
    connect(paletteShortcut, &QShortcut::activated, [=]() {
        commandLine->setVisible(true);
        commandLine->setFocus();
    });

    StringList commands;
    for (QAction* action: mActions) commands << action->text();
    mCompleter = new QCompleter(commands.toQStringList(), this);
    commandLine->setCompleter(mCompleter);

    connect(commandLine, &QLineEdit::returnPressed, [=,this]() {
                QString cmd = commandLine->text();
                for (QAction* action: mActions) {
                    if (action->text() == cmd) {
                        action->trigger();
                        break;
                    }
                }
                commandLine->clear();
                commandLine->hide();
            });

    connect(&mTimer, &QTimer::timeout, this, [this]() {
                if (mSaving.exchange(true)) return;
                mNovel.setHtml(mCurrentNode, mUi->textEdit->toHtml());
                mapTree(mById, mUi->treeWidget->topLevelItem(0));
                mCopy = mNovel;
                mGeom = geometry();
                QThread* thread = QThread::create([this]() {
                    save(mCopy, mById, mUi->textEdit->textCursor().position(), mGeom, NoUi);
                    mSaving = false;
                });
                connect(thread, &QThread::finished, thread, &QObject::deleteLater);
                thread->start();
            });
}

void Main::setupIcons() {
    mIcons["newItemToolButton"] =        ":/icon/newItem.ico";
    mIcons["deleteItemToolButton"] =     ":/icon/deleteItem.ico";
    mIcons["upToolButton"] =             ":/icon/moveItemUp.ico";
    mIcons["downToolButton"] =           ":/icon/moveItemDown.ico";
    mIcons["outToolButton"] =            ":/icon/moveItemOut.ico";
    mIcons["boldToolButton"] =           ":/icon/bold.ico";
    mIcons["italicToolButton"] =         ":/icon/italic.ico";
    mIcons["underlineToolButton"] =      ":/icon/underline.ico";
    mIcons["leftJustifyToolButton"] =    ":/icon/leftjustify.ico";
    mIcons["centerToolButton"] =         ":/icon/Center.ico";
    mIcons["rightJustifyToolButton"] =   ":/icon/rightJustify.ico";
    mIcons["fullJustifyToolButton"] =    ":/icon/fullJustify.ico";
    mIcons["increaseIndentToolButton"] = ":/icon/increseIndent.ico";
    mIcons["decreaseIndentToolButton"] = ":/icon/decreaseIndent.ico";
}

void Main::setupTabOrder() {
    QWidget::setTabOrder(mUi->newItemToolButton,        mUi->deleteItemToolButton);
    QWidget::setTabOrder(mUi->deleteItemToolButton,     mUi->upToolButton);
    QWidget::setTabOrder(mUi->upToolButton,             mUi->downToolButton);
    QWidget::setTabOrder(mUi->downToolButton,           mUi->outToolButton);
    QWidget::setTabOrder(mUi->outToolButton,            mUi->boldToolButton);
    QWidget::setTabOrder(mUi->boldToolButton,           mUi->italicToolButton);
    QWidget::setTabOrder(mUi->italicToolButton,         mUi->underlineToolButton);
    QWidget::setTabOrder(mUi->underlineToolButton,      mUi->leftJustifyToolButton);
    QWidget::setTabOrder(mUi->leftJustifyToolButton,    mUi->centerToolButton);
    QWidget::setTabOrder(mUi->centerToolButton,         mUi->rightJustifyToolButton);
    QWidget::setTabOrder(mUi->rightJustifyToolButton,   mUi->fullJustifyToolButton);
    QWidget::setTabOrder(mUi->fullJustifyToolButton,    mUi->increaseIndentToolButton);
    QWidget::setTabOrder(mUi->increaseIndentToolButton, mUi->decreaseIndentToolButton);
    QWidget::setTabOrder(mUi->decreaseIndentToolButton, mUi->treeWidget);
    QWidget::setTabOrder(mUi->treeWidget,               mUi->textEdit);
    QWidget::setTabOrder(mUi->textEdit,                 mUi->newItemToolButton);
}

void Main::changeDocumentFont(QTextDocument* doc, const QFont& font) {
    QTextCursor cursor(doc);

    struct Range {
        int pos;
        int len;
    };
    QVector<Range> ranges;

    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 2 * lineHeight;
    QTextBlockFormat toIndent;
    toIndent.setTextIndent(indent);
    toIndent.setBottomMargin(lineHeight);

    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); it != block.end(); ++it) {
            if (!it.fragment().isValid()) continue;
            const QTextFragment frag = it.fragment();
            ranges.append({ frag.position(), frag.length() });
        }

        cursor.setPosition(block.position());
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.mergeBlockFormat(toIndent);
    }

    for (const auto& range: ranges) {
        cursor.setPosition(range.pos);
        cursor.setPosition(range.pos + range.len, QTextCursor::KeepAnchor);

        QTextCharFormat fmt;
        fmt.setFontFamilies({ font.family() });
        fmt.setFontPointSize(font.pointSize());
        cursor.mergeCharFormat(fmt);
    }
}

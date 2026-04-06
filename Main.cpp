#include "Main.h"
#include "ui_Main.h"

#define QDialog QWidget
#define Ui_Dialog Ui_Widget
#include "ui_dialog.h"
#undef Ui_Dialog
#undef QDialog

#define QDialog QWidget
#define Ui_spellDialog Ui_SpellWidget
#include "ui_SpellCheckDialog.h"
#undef Ui_spellDialog
#undef QDialog

#define QDialog QWidget
#define Ui_speechDialog Ui_SpeechWidget
#include "ui_SpeechDialog.h"
#undef Ui_speechDialog
#undef QDialog

#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QCompleter>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QPdfWriter>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QScreen>
#include <QScrollBar>
#include <QShortcut>
#include <QShowEvent>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextDocumentFragment>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextToSpeech>
#include <QThread>
#include <QTreeWidget>
#include <QUrl>
#include <QWindow>

#ifdef Q_OS_LINUX
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#endif

Q_DECLARE_METATYPE(QTextDocument*)

#include "AboutDialog.h"
#include "ExportDialog.h"
#include "Exporter.h"
#include "FullScreenDialog.h"
#include "HelpDialog.h"
#include "ItemDescriptionDialog.h"
#include "PageSetupDialog.h"
#include "PreferencesDialog.h"
#include "PrintDialog.h"
#include "ScriptDialog.h"

//using namespace Words;

#include "HtmlExporter.h"
template class Exporter<HtmlExporter>;
#include "MarkdownExporter.h"
template class Exporter<MarkdownExporter>;
#include "PdfExporter.h"
template class Exporter<PdfExporter>;
#include "EBookExporter.h"
template class Exporter<EBookExporter>;
#include "RtfExporter.h"
template class Exporter<RtfExporter>;
#include "TextExporter.h"
template class Exporter<TextExporter>;

constexpr qlonglong Second = 1000;
Main* Main::sMain = nullptr;

#ifdef QT_DEBUG
static constexpr auto saveCompression = Json5Object::HumanReadable;
#else
static constexpr auto saveCompression = Json5Object::Compress;
#endif


class EditItemCommand
    : public QUndoCommand {
public:
    EditItemCommand(QTreeWidgetItem* branch,
                    Item& newItem,
                    Item& oldItem)
        : mBranch(branch)
        , mNewItem(newItem)
        , mOldItem(oldItem) {
        setText("Edit Tree Item");
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
    if (isChanged()) {
        mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave novel before exiting?",
                         [this]() { doSave(); },
                         [this]() { clearChanged(); },
                         [this]() { doNothing(); },
                         "Warning!");
        if (isChanged()) {
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

void Main::doAboutDialog() {
    AboutDialog dlg;
    dlg.exec();
}

void Main::doAboutToShowEditMenu() {
    if (focusWidget() == mUi->textEdit) {
        mUi->actionUndo->setEnabled(mUi->textEdit->document()->isUndoAvailable());
        mUi->actionRedo->setEnabled(mUi->textEdit->document()->isRedoAvailable());
        mUi->actionCopy->setEnabled(mUi->textEdit->textCursor().hasSelection());
        mUi->actionCut->setEnabled(mUi->textEdit->textCursor().hasSelection());
        mUi->actionPaste->setEnabled(mUi->textEdit->canPaste());
        mUi->actionFind_Next->setEnabled(mSearch && !mSearch->text().isEmpty());
        mUi->actionUppercase->setEnabled(mUi->textEdit->textCursor().hasSelection());
        mUi->actionLowercase->setEnabled(mUi->textEdit->textCursor().hasSelection());
    } else {
        mUi->actionUndo->setEnabled(mUi->treeWidget->canUndo());
        mUi->actionRedo->setEnabled(mUi->treeWidget->canRedo());
        mUi->actionCopy->setEnabled(true);
        mUi->actionCut->setEnabled(mUi->treeWidget->currentItem() && mUi->treeWidget->currentItem()->parent());
        mUi->actionPaste->setEnabled(mUi->treeWidget->canPaste());
        mUi->actionFind_Next->setEnabled(false);
        mUi->actionUppercase->setEnabled(false);
        mUi->actionLowercase->setEnabled(false);
    }
}

void Main::doAboutToShowFileMenu() {
    mUi->actionSave->setEnabled(mNovel.isChanged());
}

void Main::doAboutToShowNovelMenu() {
    mUi->actionOpen_Current_Item->setEnabled(mUi->treeWidget->currentItem() && mUi->treeWidget->currentItem()->childCount() != 0);
    mUi->actionOpen_Current_Item->setText((mUi->treeWidget->currentItem() && mUi->treeWidget->currentItem()->isExpanded()) ? "Close Current Item" : "Open Current Item");
    mUi->actionRemove_Item->setEnabled(!(mUi->treeWidget->currentItem() == mUi->treeWidget->topLevelItem(0)));
    mUi->actionMove_an_Item_Up->setEnabled(!nothingAbove());
    mUi->actionMove_Current_Item_Down->setEnabled(!nothingBelow());
    mUi->actionMove_Current_Item_Out->setEnabled(!nothingAbove());
}

void Main::doAboutToShowViewMenu() {
    mUi->actionRead_To_Me->setEnabled(mSpeechAvailable);
    mUi->actionSpell_checking->setEnabled(mSpelling.ready());
}

void Main::doAboutToShowHelpMenu() {
}

void Main::doAddItem() {
    Item item;
    item.setDocumentFont(mUi->textEdit->document()->defaultFont());
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
        mUi->boldToolButton->setChecked(false);
        setMenu(mUi->actionBold, false);
    } else {
        format.setFontWeight(QFont::Bold);
        mUi->boldToolButton->setChecked(true);
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

void Main::doClearRecentNovels() {
    auto recents = mPrefs.recentNovels();
    recents.clear();
    mPrefs.setRecentNovels(recents);
    QMenu* recentMenu = mUi->menu_Recent_Novels;
    auto actions = recentMenu->actions();
    QAction* clearAction = mUi->action_Clear_Recent_Novels;
    for (auto&& action: actions) {
        if (action != clearAction) recentMenu->removeAction(action);
    }
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

#ifdef Q_OS_LINUX
void Main::doDbusChanged(QString ns, QString key, QDBusVariant value) {
    if (ns == "org.freedesktop.appearance" && key == "color-scheme") {
        uint v = value.variant().toUInt();
        mPrefs.setIsDark(v == 1);
        updateFromPrefs();
    }
}
#endif

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
    mUi->treeWidget->saveUndo(new EditItemCommand(branch, current, save ));
    changed();
}

void Main::doExit() {
    close();
}

void Main::doExport(const QString& name) {
    const auto& factory = ExporterBase::registry()[name];
    const auto ids = vectorOfIds(mUi->treeWidget->currentItem(),
                                 { mPrefs.chapterTag(), mPrefs.sceneTag(), mPrefs.coverTag() });
    ExporterBase* exporter = factory(mNovel, ids);
    ExportDialog dlg(exporter, this);
    if (dlg.exec() == QDialog::Rejected) return;

    mPrefs.setChapterTag(dlg.chapterTag());
    mPrefs.setCoverTag(dlg.coverTag());
    mPrefs.setSceneTag(dlg.sceneTag());

    exporter->setChapterTag(dlg.chapterTag());
    exporter->setCoverTag(dlg.coverTag());
    exporter->setSceneTag(dlg.sceneTag());
    exporter->setInternalImages(mUi->textEdit->internalImages());
    auto* rootItem = mUi->treeWidget->topLevelItem(0);
    const auto finalIds = vectorOfIds(rootItem, { mPrefs.chapterTag(), mPrefs.sceneTag(), mPrefs.coverTag() });
    exporter->setIds(finalIds);

    QString filename = dlg.filename();
    if (filename.isEmpty()) {
        mMsg.Statement("Export of " + exporter->name() + " file\n requires a file name!");
        return;
    } else {
        QFileInfo file(filename);
        if (file.isRelative()) {
            filename = QDir(mDocDir).filePath(filename);
        }
    }
    exporter->setFilename(filename);
    QFileInfo info(filename);
    filename = info.fileName();
    mDocDir = info.absolutePath();
    if (!exporter->convert()) mMsg.Statement("Export of " + exporter->name() + " file\n'" + filename + "'\nFailed");
}

void Main::doFindChanged() {
    if (mSearch && mFindWidget) {
        bool sensitivity = mFindWidget->caseInsensitiveCheckBox->isChecked()
                               ? SearchCore::NotCaseInsensitive
                               : SearchCore::CaseInsensitive;
        auto sense = sensitivity ? Qt::CaseInsensitive : Qt::CaseSensitive;
        mSearch->setSensitivity(sense);
        QString text = mFindWidget->findLineEdit->text();
        mSearch->setText(text);
        QString replace = mFindWidget->replaceLineEdit->text();
        mFindWidget->replacePushButton->setDisabled(text.isEmpty() || text.compare(replace, sense) == 0);
        mFindWidget->replaceAllPushButton->setDisabled(text.isEmpty() || text.compare(replace, sense) == 0);
        doFindNext();
    }
}

void Main::doFindDone() {
    mFindLine->hide();
    mUi->textEdit->setFocus();
}

void Main::doFindNext() {
    if (mSpellcheck->isVisible()) {
        doSpellcheckNext();
    } else if (mSearch && !mSearch->text().isEmpty() && !mUi->treeWidget->hasFocus()) {
        NovelPosition pos = mSearch->findNext();
        if (pos.isValid()) {
            QTreeWidgetItem* branch = findItem(mUi->treeWidget->topLevelItem(0), pos.id());
            mUi->treeWidget->setCurrentItem(branch);
            QString text = mFindWidget->findLineEdit->text();
            auto cursor = mUi->textEdit->textCursor();
            cursor.setPosition(pos.textPosition());
            cursor.setPosition(pos.textPosition() + text.length(), QTextCursor::KeepAnchor);
            mUi->textEdit->setTextCursor(cursor);
            cursorPositionChanged();
            QString replace = mFindWidget->replaceLineEdit->text();
            bool sensitivity = mFindWidget->caseInsensitiveCheckBox->isChecked()
                                   ? SearchCore::CaseInsensitive
                                   : SearchCore::NotCaseInsensitive;
            auto sense = sensitivity ? Qt::CaseInsensitive : Qt::CaseSensitive;
            mFindWidget->replacePushButton->setDisabled(!text.compare(replace, sense));
            mFindWidget->replaceAllPushButton->setDisabled(!text.compare(replace, sense));
        } else {
            mFindWidget->replacePushButton->setDisabled(true);
            mFindWidget->replacePushButton->setDisabled(true);
        }
    }
}

void Main::doFindReplace() {
    mSpellcheck->hide();
    mFindLine->show();
    delete mSearch;
    mSearch = nullptr;

    if (mUi->treeWidget->hasFocus()) return;

    QString find = mFindWidget->findLineEdit->text();
    QString replace = mFindWidget->replaceLineEdit->text();

    if (!find.isEmpty()) mFindWidget->findLineEdit->selectAll();

    mFindLine->setFocus();
    mFindWidget->findLineEdit->setFocus();

    bool sensitivity = !mFindWidget->caseInsensitiveCheckBox->isChecked()
                           ? SearchCore::NotCaseInsensitive
                           : SearchCore::CaseInsensitive;
    auto cursor = mUi->textEdit->textCursor();
    if (cursor.hasSelection()) {
        mFindWidget->thisItemRadioButton->setDisabled(true);
        mFindWidget->thisItemAndChildItemsRadioButton->setDisabled(true);
        auto begin = cursor.selectionStart();
        auto end   = cursor.selectionEnd();
        mSearch = new SearchSelection(mNovel, mCurrentNode, find, begin, end, sensitivity);
    } else {
        mFindWidget->thisItemRadioButton->setEnabled(true);
        mFindWidget->thisItemAndChildItemsRadioButton->setEnabled(true);
        auto start = cursor.position();
        if (mFindWidget->thisItemRadioButton) mSearch = new SearchItem(mNovel, mCurrentNode, start, find, sensitivity);
        else mSearch = new SearchTree(mNovel, mUi->treeWidget->currentItem(), mCurrentNode, start, find, sensitivity);
    }

    auto sense = sensitivity ? Qt::CaseSensitive : Qt::CaseInsensitive;
    bool replaceAble = find.isEmpty() || !find.compare(replace, sense);
    mFindWidget->replacePushButton->setDisabled(replaceAble);
    mFindWidget->replaceAllPushButton->setDisabled(replaceAble);

    connect(mFindWidget->findLineEdit,            &QLineEdit::textEdited,        this, &Main::findChanged);
    connect(mFindWidget->caseInsensitiveCheckBox, &QCheckBox::checkStateChanged, this, &Main::findChanged);

    if (find.isEmpty()) return;

    doFindChanged();
}

void Main::doFocusChanged(QWidget*, QWidget* now) {
    if (now == mUi->treeWidget) mFindLine->hide();
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

    TextEdit* edit = mUi->textEdit;
    QTextDocument* doc = edit->document();

    dlg.setDocument(doc);
    dlg.setPosition(edit->textCursor().position());

    if (mPrefs.typingSounds()) dlg.setSoundPool(&mSoundPool);

    auto arr = mPrefs.otherSplitter();
    if (arr.size() >= 2) dlg.setOther(arr);

    dlg.showFullScreen();
    dlg.exec();

    auto other = dlg.other();
    mPrefs.setOtherSplitter(other);

    edit->setDocument(doc);

    setPosition(dlg.position());
    doCursorPositionChanged();
}

void Main::doHelp() {
    HelpDialog dlg;
    dlg.exec();
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
        mUi->italicToolButton->setChecked(true);
        setMenu(mUi->actionItalic, true);
    } else {
        format.setFontItalic(false);
        mUi->italicToolButton->setChecked(false);
        setMenu(mUi->actionItalic, false);
    }
    cursor.mergeCharFormat(format);
    mUi->textEdit->setTextCursor(cursor);
    changed();
}

QMimeData* Main::copyMimeData(const QMimeData* src) {
    QMimeData* copy = new QMimeData();
    StringList formats { src->formats() };
    for (const auto& format: formats) {
        QByteArray data = src->data(format);
        copy->setData(format, data);
    }
    return copy;
}

void Main::doItemChanged(QTreeWidgetItem* current) {
    if (current == nullptr) return;

    auto& oldItem = mNovel.findItem(mCurrentNode);
    oldItem.count();
    oldItem.setPosition(mUi->textEdit->textCursor().position());

    mCurrentNode = current->data(0, Qt::UserRole).toLongLong();
    Item& item = mNovel.findItem(mCurrentNode);
    QTextDocument* doc = item.doc();
    mUi->textEdit->setDocument(doc);

    /* this is a hack! */
    QApplication::processEvents();
    mUi->textEdit->setUpdatesEnabled(false);

    mUi->textEdit->selectAll();
    if (mUi->textEdit->textCursor().selectedText().length()) {
        QClipboard* clipboard = QApplication::clipboard();
        QMimeData* save = copyMimeData(clipboard->mimeData());
        QApplication::processEvents();
        mUi->textEdit->cut();
        QApplication::processEvents();
        mUi->textEdit->paste();
        clipboard->setMimeData(save);
    }

    mUi->textEdit->setUpdatesEnabled(true);
    /* end of hack */

    mPosition = item.position();
    setPosition(mPosition);

    mUi->actionOpen_Current_Item->setText(current->isExpanded() ? "Close Current Item" : "Open Current Item");

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

void Main::doManageScripts() {
    ScriptDialog dlg(this);

    QDir source(mScriptsPath);
    StringList sources { source.entryList({ "*.5th" }, QDir::Filter::Files, QDir::SortFlag::Name) };
    Map<QString, QString> hoverText;
    StringList deleteAfter;
    for (const auto& name: sources) {
        QFile file(mScriptsPath + "/" + name);
        if (!file.open(QIODevice::ReadOnly)) deleteAfter.append(name);
        else {
            QString tagLine = file.readLine();
            if (tagLine.startsWith("//")) {
                QString text = tagLine.mid(2).trimmed();
                if (text.isEmpty()) deleteAfter.append(name);
                else hoverText[name] = text;
            } else deleteAfter.append(name);
        }
    }
    for (const auto& failed: deleteAfter) sources.removeAll(failed);
    dlg.setAvailableScripts(sources, hoverText);
    dlg.setActiveScripts(mPrefs.actingScripts());
    dlg.setButtonStates();

    if (dlg.exec() == QDialog::Rejected) return;

    setupScripting();
    loadScripts();
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
    if (isChanged()) {
        if (mNovel.isChanged()) {
            mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave this novel before starting a new Novel?",
                             [this]() { doSave(); },
                             [this]() { clearChanged(); },
                             [this]() { doNothing(); },
                             "Warning!");
            if (isChanged()) return;
        }
    }

    mNovel.clear();
    mSpelling.clearWords();
    mPosition = 0;
    mState.clear();
    mCurrentNode = mNovel.root();
    auto* tree = mUi->treeWidget;
    tree->clearUndo();
    Item& item = mNovel.findItem(mCurrentNode);
    auto* doc = item.doc();
    auto* edit = mUi->textEdit;
    edit->setDocument(doc);
    edit->clearInternalImages();
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
    if (isChanged()) {
        mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave this novel before working a different Novel?",
                         [this]() { doSave(); },
                         [this]() { clearChanged(); },
                         [this]() { doNothing(); },
                         "Warning!");
        if (isChanged()) return;
    }

    QString filename = QFileDialog::getOpenFileName(this, "Open Which Novel?", mDocDir,
                                                    "Novels (*.novel);;All Files (*.*)");
    if (filename.isEmpty()) return;

    loadFile(filename);
}

//  open needs to preserve the recent file list (save, then restore after file loaded, add the opened novel to it).
void Main::loadFile(const QString& filename) {
    QFileInfo info(filename);
    mDocDir = info.absolutePath();

    busy();
    mSpelling.clearWords();
    mNovel.clear();
    mPosition = 0;
    mState.clear();
    mNovel.setFilename(filename);
    mNovel.open();
    mUi->treeWidget->clearUndo();
    Json5Object obj = mNovel.extra();
    auto savedRecents = mPrefs.recentNovels();
    if (obj.contains(Novel::Prefs)) {
        Json5Object prefs = Item::hasObj(obj, Novel::Prefs, {} );
        mPrefs.read(prefs);
        mCurrentNode =      Item::hasNum(obj, Novel::Current, qlonglong(-1));
        mPosition =         Item::hasNum(obj, Novel::Position, qlonglong(0));
        Json5Array array =  Item::hasArr(obj, Novel::State);
        mState.clear();
        for (auto& state: array) {
            if (!state.isArray()) continue;
            auto& item = state.toArray();
            int id = Item::hasNum(item, 0, qlonglong(-1));
            bool expanded = Item::hasBool(item, 1);
            mState[id] = expanded;
        }
        Json5Array images = Item::hasArr(obj, Novel::Images, {});
        mImageStore.clear();
        for (auto& obj: images) {
            if (!obj.isObject()) continue;
            Json5Object image = obj.toObject();
            QString url = Item::hasStr(image, { Novel::V1Url, Novel::Url }, {});
            if (url.isEmpty()) continue;
            QString str = Item::hasStr(image, { Novel::V1Data, Novel::Data }, {});
            if (str.isEmpty()) continue;
            QByteArray data = QByteArray::fromBase64(str.toUtf8());
            if (data.isEmpty()) continue;
            QImage img;
            img.loadFromData(data);
            if (img.isNull()) continue;
            mImageStore[url] = img;
            mUi->textEdit->addInternalImage(url, img, false);
        }
        Json5Array words = Item::hasArr(obj, Novel::Dictionary);
        for (auto i = 0; i < words.count(); ++i) {
            QString word = Item::hasStr(words, i, "");
            if (!word.isEmpty()) mSpelling.addWord(word);
        }
    } else if (obj.contains(Novel::V1)) {
        Json5Object prefs = Item::hasObj(obj, Novel::Prefs, {} );
        mPrefs.read(prefs);
        mCurrentNode = mNovel.root();
        mPosition = 0;
        mState.clear();
        bool state = Item::hasBool(obj, Novel::State, false);
        Json5Array ids = Item::hasArr(obj, Novel::Ids, {});
        for (auto& id: ids) mState[id.toInt()] = state;
    }

    mPrefs.setRecentNovels(savedRecents);
    mPrefs.addNovel(info.baseName() + ".novel", filename);

    setPosition(mPosition);
    update(true);
    doCursorPositionChanged();
    auto item = mNovel.findItem(mCurrentNode);
    auto count = item.count();
    auto total = mNovel.countAll();
    mWordCount.setSinceLastCounted(0);
    mWordCount.setSinceOpened(0);
    mWordCount.setCurrentItem(count);
    mWordCount.setTotal(total);
    clearChanged();
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

void Main::doPageSetup() {
    PageSetupDialog dlg(this);
    if (dlg.exec() == QDialog::Rejected) return;
    mPrefs = *dlg.preferences();
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

void Main::doPause() {
    if (!mSpeechAvailable) return;
    if (mSpeech.paused()) {
        mSpeech.play();
        mSpeechWidget->pausePlayPushButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        mSpeech.pause();
        mSpeechWidget->pausePlayPushButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void Main::doPreferences() {
    Preferences prefs(mPrefs);
    PreferencesDialog dlg(mPrefs, this);
    if (dlg.exec() == QDialog::Rejected) mPrefs = prefs;
    updateFromPrefs();
}

void Main::doPrint() {
    PrintDialog dlg(this);
    dlg.exec();
}

void Main::doReadToMe() {
    if (!mSpeechAvailable) return;

    QTextCursor cursor = mUi->textEdit->textCursor();
    if (cursor.hasSelection()) mTextToSpeak = cursor.selectedText();
    else mTextToSpeak = mUi->textEdit->toPlainText();

    if (mTextToSpeak.size() == 0) return;

    mSpeaking->setVisible(true);
    mSpeechWidget->pausePlayPushButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
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
    auto* tree = mUi->treeWidget;
    tree->saveUndo(new RemoveItemCommand(mUi->treeWidget, parent, idx, item, nvlItem));
    auto* doc = tree->textDocument(node);
    tree->removeTextDocument(node);
    delete doc;
    parent->removeChild(item);

    changed();
}

void Main::doReplace() {
    auto sensitive = mFindWidget->caseInsensitiveCheckBox->isChecked()
                         ? Qt::CaseInsensitive
                         : Qt::CaseSensitive;
    QString find = mFindWidget->findLineEdit->text();
    QString replace = mFindWidget->replaceLineEdit->text();
    if (replace.compare(find, sensitive) == 0 || find.isEmpty()) return;

    auto cursor = mUi->textEdit->textCursor();
    QString text = mUi->textEdit->toPlainText();
    QString textAtPosition = text.mid(cursor.position());
    QString left = textAtPosition.left(find.length());
    if (left.compare(find, sensitive)) return;

    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, replace.length());
    cursor.removeSelectedText();
}

void Main::doReplaceAll() {
    auto sensitive = mFindWidget->caseInsensitiveCheckBox->isChecked()
                         ? Qt::CaseInsensitive
                         : Qt::CaseSensitive;
    QString find = mFindWidget->findLineEdit->text();
    QString replace = mFindWidget->replaceLineEdit->text();
    if (replace.compare(find, sensitive) == 0 || find.isEmpty()) return;

    auto cursor = mUi->textEdit->textCursor();
    QString text = mUi->textEdit->toPlainText();
    QString textAtPosition = text.mid(cursor.position());
    QString left = textAtPosition.left(find.length());
    if (left.compare(find, sensitive)) return;

    do {
        doReplace();
        doFindNext();
        auto cursor = mUi->textEdit->textCursor();
        QString text = mUi->textEdit->toPlainText();
        QString textAtPosition = text.mid(cursor.position());
        QString left = textAtPosition.left(find.length());
    } while (left.compare(find, sensitive) == 0);
}

void Main::doReplaceChanged() {
    QString find = mFindWidget->findLineEdit->text();
    QString replace = mFindWidget->replaceLineEdit->text();
    mFindWidget->replacePushButton->setDisabled(find.isEmpty() || find == replace);
    mFindWidget->replaceAllPushButton->setDisabled(find.isEmpty() ||find == replace);
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
    if (!mNovel.isChanged()) return;

    if (mSaving.exchange(true)) return;

//    mNovel.setHtml(mCurrentNode, mUi->textEdit->toHtml());
    Map<qlonglong, bool> byId;
    mapTree(byId, mUi->treeWidget->topLevelItem(0));
    save(mNovel, byId, mUi->textEdit->textCursor().position(), geometry());
    mSaving = false;
}

bool Main::doSaveAs() {
    QString filename = QFileDialog::getSaveFileName(this, "Save the Novel as?", mDocDir, "Novels (*.novel);;All Files (*.*)");
    if (filename.isEmpty()) return false;
    if (!filename.endsWith(".novel", Qt::CaseInsensitive)) filename.append(".novel");
    mNovel.setFilename(filename);
    changed();
    doSave();
    return true;
}

void Main::doSelectionChanged() {
    selectionItems();
}

void Main::doSpellcheck() {
    if (!mSpelling.ready()) return;
    if (mUi->treeWidget->hasFocus()) return;
    mFindLine->hide();
    mSpellcheck->show();

    mSpellcheckIds = vectorOfIds(mUi->treeWidget->currentItem(), { });
    mSpellcheckPosition = 0;
    doSpellcheckNext();
}

void Main::doSpellcheckAddWord(const QString& word) {
    mSpelling.addWord(word);
    changed();
    doSpellcheckNext();
}

void Main::doSpellcheckDone() {
    mSpellcheck->hide();
    mUi->textEdit->setFocus();
    mSpellcheckIds.clear();
}

void Main::doSpellcheckLeft() {
    auto* scrollArea = mSpellWidget->scrollArea;
    auto* scrollBar = scrollArea->horizontalScrollBar();
    auto* leftMost = dynamic_cast<QPushButton*>(mSpellWidget->scrollAreaWidgetContents->children().first());
    int newPos = scrollBar->value() - leftMost->geometry().width() + 4;
    if (newPos < 0) newPos = 0;
    scrollBar->setValue(newPos);
    mSpellWidget->leftPushButton->setEnabled(scrollBar->value() > 0);
    mSpellWidget->rightPushButton->setEnabled(true);
}

void Main::doSpellcheckNext() {
    while (!mSpellcheckIds.isEmpty()) {
        auto id = mSpellcheckIds.first();
        Item& item = mNovel.findItem(id);
        TextEdit work;
        work.setHtml(item.html());
        QString text = work.toPlainText();
        for (mSpellcheckWord = nextWord(text, mSpellcheckPosition); mSpellcheckWord.mNext != mSpellcheckWord.mAt; mSpellcheckWord = nextWord(text, mSpellcheckWord.mNext)) {
            auto suggestions = mSpelling.check(mSpellcheckWord.mWord);
            if (suggestions.isEmpty()) continue;
            auto* branch = findItem(mUi->treeWidget->topLevelItem(0), id);
            mUi->treeWidget->setCurrentItem(branch);
            setPosition(mSpellcheckWord.mAt);
            mUi->textEdit->ensureCursorVisible();
            cursorPositionChanged();
            auto* area = mSpellWidget->scrollAreaWidgetContents;
            auto* layout = dynamic_cast<QGridLayout*>(area->layout());
            if (layout == nullptr) {
                layout = new QGridLayout(area);
                area->setLayout(layout);
            }
            while (QLayoutItem* btn = layout->takeAt(0)) {
                if (QWidget* w = btn->widget()) w->deleteLater();
                delete btn;
            }
            QPushButton* button { nullptr };
            for (auto&& [i, suggestion]: enumerate(suggestions)) {
                auto* contents = mSpellWidget->scrollAreaWidgetContents;
                auto* button = new QPushButton(suggestion, contents);
                button->setObjectName(mSpellcheckWord.mWord);
                QRect geom = button->geometry();
                int x = (geom.width() + 4) * i;
                geom.setX(x);
                geom.setY(0);
                geom.setHeight(area->height());
                button->setGeometry(geom);
                connect(button, &QPushButton::clicked, this, &Main::suggestionAction);
                button->setEnabled(true);
                button->setVisible(true);
                layout->addWidget(button);
                contents->adjustSize();
            }
            if (!button) break;
            repaint();
            mSpellWidget->leftPushButton->setDisabled(true);
            auto* scrollBar = dynamic_cast<QScrollBar*>(mSpellWidget->scrollArea->horizontalScrollBar());
            mSpellWidget->rightPushButton->setEnabled(scrollBar->value() < scrollBar->maximum());
            return;
        }
        mSpellcheckIds.takeFirst();
        mSpellcheckPosition = 0;
    }
    doSpellcheckDone();
}

void Main::doSpellcheckRight() {
    auto* scrollArea = mSpellWidget->scrollArea;
    auto* scrollBar = scrollArea->horizontalScrollBar();
    auto* rightMost = dynamic_cast<QPushButton*>(mSpellWidget->scrollAreaWidgetContents->children().last());
    int newPos = scrollBar->value() + rightMost->geometry().width() + 4;
    if (newPos >= scrollBar->maximum()) newPos = scrollBar->maximum();
    scrollBar->setValue(newPos);
    mSpellWidget->rightPushButton->setEnabled(scrollBar->value() < scrollBar->maximum());
    mSpellWidget->leftPushButton->setEnabled(true);
}

void Main::doStop(bool stopped) {
    QTextCursor cursor(mUi->textEdit->document());
    if (mSavedCursor.hasSelection()) {
        auto cursor = mUi->textEdit->textCursor();
        cursor.setPosition(mSavedCursor.selectionStart());
        cursor.setPosition(mSavedCursor.selectionEnd());
        mUi->textEdit->setTextCursor(cursor);
    } else setPosition(mSavedCursor.position());
    mSpeaking->setVisible(false);
    if (!stopped) mSpeech.stop();
}

void Main::doSuggestion(const QString& from, const QString& to) {
    QString toStr = to.toHtmlEscaped();
    QString fromStr = from.toHtmlEscaped();
    Item& item = mNovel.findItem(mCurrentNode);
    QString html = item.html().replace(fromStr, toStr);
    item.setHtml(html);
    mSpellcheckPosition = mSpellcheckWord.mAt;
    changed();
    doSpellcheckNext();
}

void Main::doTextChanged() {
    changed();
}

void Main::doToolbars() {
    bool isVisible = mUi->editToolbarFrame->isVisible();
    mUi->editToolbarFrame->setVisible(!isVisible);
    mUi->novelToolbarFrame->setVisible(!isVisible);
    mUi->actionHide_Show_Toolbar->setText(!isVisible ? "Hide Toolbars" : "Show Toolbars");
    mPrefs.setToolbarVisible(!isVisible);
}

void Main::doUnderline() {
    auto cursor = mUi->textEdit->textCursor();
    QTextCharFormat format;
    if (!cursor.charFormat().fontUnderline()) {
        format.setFontUnderline(true);
        mUi->underlineToolButton->setChecked(true);
        setMenu(mUi->actionUnderline, true);
    } else {
        format.setFontUnderline(false);
        mUi->underlineToolButton->setChecked(false);
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

void Main::doWeb() {
    QDesktopServices::openUrl(QUrl("https://my-domain/index.html"));
}

void Main::doWordCount() {
    wordCounts();
    auto report = QString("Total: %1 | Current Item %2 | Since Opening %3 | Since Last Count %4")
                      .arg(mWordCount.total())
                      .arg(mWordCount.currentItem())
                      .arg(mWordCount.sinceOpened())
                      .arg(mWordCount.sinceLastCounted());
    mUi->statusbar->showMessage(report, 30 * 1000);
}

void Main::addActionToMenuPath(QMenuBar* bar, QAction* action, const QString& path) {
    StringList parts { path.split('\t', Qt::SkipEmptyParts) };
    if (parts.isEmpty()) return;

    QMenu* currentMenu = nullptr;

    // Step 1: Find or create the top-level menu
    for (QAction* act: bar->actions()) {
        if (act->menu() && act->text() == parts[0]) {
            currentMenu = act->menu();
            break;
        }
    }

    if (!currentMenu) {
        currentMenu = new QMenu(parts[0], bar);
        bar->addMenu(currentMenu);
    }

    // Step 2: Walk down the submenu chain
    for (int i = 1; i < parts.size(); ++i) {
        const QString& name = parts[i];

        QMenu* nextMenu = nullptr;
        for (QAction* act: currentMenu->actions()) {
            if (act->menu() && act->text() == name) {
                nextMenu = act->menu();
                break;
            }
        }

        if (!nextMenu) {
            nextMenu = new QMenu(name, currentMenu);
            currentMenu->addMenu(nextMenu);
        }

        currentMenu = nextMenu;
    }

    // Step 3: Insert the final action
    currentMenu->addAction(action);
}

void Main::applyNovelFormatting() {
    TextEdit* editor = mUi->textEdit;
    QFont targetFont(mPrefs.fontFamily(), mPrefs.fontSize());
    auto* doc = editor->document();
    editor->document()->setDefaultFont(targetFont);

    QTextCursor cursor = editor->textCursor();
    cursor.select(QTextCursor::Document);
    QTextCharFormat charFmt;
    charFmt.setFontFamilies({ targetFont.family() });
    if (targetFont.pointSize() > 0) charFmt.setFontPointSize(targetFont.pointSizeF());
    else if (targetFont.pixelSize() > 0) charFmt.setFontPointSize(targetFont.pixelSize() * 0.75);
    cursor.mergeCharFormat(charFmt);
    cursor.clearSelection();

    QFontMetrics fm(targetFont);
    int em = fm.horizontalAdvance('M'); // The width of a capital M
    int lineHeight = fm.height();

    cursor.movePosition(QTextCursor::Start);

    editor->blockSignals(true); // Disable updates for speed
    cursor.beginEditBlock();    // Group into one undo step

    do {
        QTextBlockFormat fmt = cursor.blockFormat(); // Get EXISTING format

        Qt::Alignment align = fmt.alignment();
        if (align == Qt::AlignLeft || align == Qt::AlignJustify || align == Qt::AlignLeading) fmt.setTextIndent(4 * em);

        fmt.setBottomMargin(1 * lineHeight);

        cursor.setBlockFormat(fmt);
    } while (cursor.movePosition(QTextCursor::NextBlock));

    cursor.endEditBlock();
    editor->blockSignals(false);

    QTextCharFormat penFmt;
    penFmt.setFontFamilies({targetFont.family()});

    if (targetFont.pointSizeF() > 0) penFmt.setFontPointSize(targetFont.pointSizeF());
    else if (targetFont.pixelSize() > 0) penFmt.setFontPointSize(targetFont.pixelSize() * 0.75);

    editor->setCurrentCharFormat(penFmt);

    if (doc->isEmpty()) {
        QTextCursor startCursor = editor->textCursor();
        startCursor.movePosition(QTextCursor::Start);
        editor->setTextCursor(startCursor);
        editor->setCurrentCharFormat(penFmt); // Set it again to be sure
    }
}

void Main::buildTree(const TreeNode& branch, QTreeWidgetItem* itemBranch, Map<qlonglong, bool>& byId) {
    auto twItem = new QTreeWidgetItem();
    auto& item = mNovel.findItem(branch.id());
    auto* doc = item.doc();
    // TBDEL QTextDocument* doc = new QTextDocument();
    // TBDEL registerImages(item.html(), doc);
    // TBDEL doc->setParent(nullptr);
    auto tree = mUi->treeWidget;
    twItem->setText(0, item.name());
    twItem->setData(0, Qt::UserRole, item.id());
    // TBDEL tree->setTextDocument(branch.id(), doc);

    if (itemBranch == nullptr) {
        // TBDEL doc->setParent(nullptr);
        mUi->textEdit->setDocument(doc);
        tree->addTopLevelItem(twItem);
    } else itemBranch->addChild(twItem);

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

void Main::loadImageBytes(const QImage& img, const QString& type, std::function<void(QByteArray)> callback) {
    QByteArray data;
    if (!img.isNull()) {
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, type.toStdString().c_str());
    }
    callback(data);
}

void Main::loadImageBytesFromUrl(const QUrl& url, std::function<void(QByteArray)> callback) {
    loadImageBytesFromUrl(url, "PNG", callback);
}

void Main::loadImageBytesFromUrl(const QUrl& url, const QString& type, std::function<void(QByteArray)> callback) {
    if (url.isLocalFile()
        || url.scheme() == "qrc"
        || url.scheme().isEmpty()) {
        // Local file or resource
        QImage img(url.toLocalFile());
        loadImageBytes(img, type, callback);
        return;
    }

    if (url.scheme() == "internal") {
        auto& imgs = mUi->textEdit->internalImages();
        if (imgs.contains(url)) {
            loadImageBytes(imgs[url], type, callback);
            return;
        }

        QByteArray data;
        callback(data);
        return;
    }

    // Remote URL
    auto* nam = new QNetworkAccessManager();
    QNetworkReply* reply = nam->get(QNetworkRequest(url));

    QObject::connect(reply, &QNetworkReply::finished, [this, type, reply, callback]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        QImage img(data);
        loadImageBytes(img, type, callback);
    });
}

void Main::justifyButtons() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    auto align = block.alignment();

    bool isCenter = align.testFlag(Qt::AlignHCenter);
    bool isRight = align.testFlag(Qt::AlignRight) || align.testFlag(Qt::AlignTrailing);
    bool isJustify = align.testFlag(Qt::AlignJustify);
    bool isLeft = !(isCenter || isRight || isJustify) || align.testFlag(Qt::AlignLeft) || align.testFlag(Qt::AlignLeading);

    mUi->leftJustifyToolButton->setChecked(isLeft);
    setMenu(mUi->actionLeft_Justify,       isLeft);
    mUi->centerToolButton->setChecked(isCenter);
    setMenu(mUi->actionCenter,        isCenter);
    mUi->rightJustifyToolButton->setChecked(isRight);
    setMenu(mUi->actionRight_Justify,       isRight);
    mUi->fullJustifyToolButton->setChecked(isJustify);
    setMenu(mUi->actionFull_Justification, isJustify);
}

void Main::loadScripts() {
    mScriptsPath = mDocDir + "/TextSmith/Scripts";
    QDir scriptDir(mScriptsPath);
    scriptDir.mkpath(mScriptsPath);
    StringList scripts { scriptDir.entryList({ "*.5th" }, QDir::Filter::Files, QDir::SortFlag::Name) };
    std::filesystem::path toPath(mScriptsPath.toStdString());

    auto acting = mPrefs.actingScripts();
    QString sourcePath = mAppDir + "/Scripts";
    QDir sourceDir(sourcePath);
    StringList sources { sourceDir.entryList({ "*.5th" }, QDir::Filter::Files, QDir::SortFlag::Name) };
    if (sources.empty()) {
        sourcePath = mAppDir + "/../../Scripts";
        QDir sourceDir(sourcePath);
        sources = sourceDir.entryList({ "*.5th" }, QDir::Filter::Files, QDir::SortFlag::Name);
    }

    for (const auto& source: sources) {
        if (!scripts.contains(source)) {
            QFile::copy(sourcePath + "/" + source, mScriptsPath + "/" + source);
            scripts.append(source);
        }
    }

    for (const auto& script: acting) {
        QFile file(script);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QByteArray data(file.readAll());
        file.close();
        QString code(data);
        mVm->evaluate(code);
    }
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
    tree->saveUndo(new RemoveItemCommand(mUi->treeWidget, branch->parent(), index, branch, item));
    tree->removeTextDocument(id);

    delete branch;
}

QString Main::findKey(QString& used, const QString& name, bool secondPass) {
    for (const auto& ch: name) {
        auto uc = ch.toUpper();
        if (!used.contains(uc)) {
            used += uc;
            return uc;
        }
        auto lc = ch.toLower();
        if (!used.contains(lc)) {
            used += lc;
            return QString("Shift+%1").arg(uc);
        }
    }
    if (secondPass) return "";
    return findKey(used, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890", true);
}

void Main::mapTree(Map<qlonglong, bool>& byId, QTreeWidgetItem* item) {
    auto id = item->data(0, Qt::UserRole).toLongLong();
    bool open = item->isExpanded();
    byId[id] = open;
    for (int i = 0; i < item->childCount(); ++i) mapTree(byId, item->child(i));
}

Main::WordPos Main::nextWord(const QString& str, qlonglong pos) {
    pos = skipSpaces(str, pos);
    qlonglong start = pos;
    while (pos < str.size() && !str[pos].isSpace()) ++pos;
    bool uppercase = false;
    if (start != pos) uppercase = str.mid(pos, pos - start)[0].isUpper();
    return { str.mid(pos, pos - start), start, pos, uppercase };
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
            QTextDocument* doc = new QTextDocument();
            doc->setParent(nullptr);
            newBranch->setText(0, item.name());
            newBranch->setData(0, Qt::UserRole, id);
            tree->setTextDocument(id, doc);
            break;
        } else if (mimeData->hasUrls()) {
            auto urls = mimeData->urls();
            for (int i = 0; i < urls.count(); ++i) {
                auto& url = urls[i];
                if (url.isLocalFile()) {
                    QString filename = url.fileName();
                    if (filename.endsWith(".txt", Qt::CaseInsensitive)) {
                        QFile file(url.path());
                        if (file.open(QIODevice::ReadOnly)) {
                            QString text = file.readAll();
                            edit.setText(text);
                            file.close();
                            invalidText = false;
                            break;
                        }
                    } else if (filename.endsWith(".html", Qt::CaseInsensitive)) {
                        QFile file(url.path());
                        if (file.open(QIODevice::ReadOnly)) {
                            QString html = file.readAll();
                            edit.setHtml(html);
                            file.close();
                            invalidText = false;
                            break;
                        }
                    } else if (filename.endsWith(".bmp",  Qt::CaseInsensitive) ||
                               filename.endsWith(".gif",  Qt::CaseInsensitive) ||
                               filename.endsWith(".jpg",  Qt::CaseInsensitive) ||
                               filename.endsWith(".jpeg", Qt::CaseInsensitive) ||
                               filename.endsWith(".png",  Qt::CaseInsensitive) ||
                               filename.endsWith(".pbm",  Qt::CaseInsensitive) ||
                               filename.endsWith(".pgm",  Qt::CaseInsensitive) ||
                               filename.endsWith(".ppm",  Qt::CaseInsensitive) ||
                               filename.endsWith(".xbm",  Qt::CaseInsensitive) ||
                               filename.endsWith(".xpm",  Qt::CaseInsensitive)) {
                        QImage img(url.path());
                        edit.addInternalImage(url, img);
                        auto cursor = edit.textCursor();
                        QTextImageFormat format;
                        format.setName(url.path());
                        cursor.insertImage(format);
                        invalidText = false;
                        break;
                    }
                }
            }
            if (!invalidText) {
                setupHtml(edit);
                Item item;
                item.setHtml(edit.toHtml());
                auto id = item.id();
                mNovel.addItem(item);
                QTreeWidgetItem* newBranch = new QTreeWidgetItem();
                QTextDocument* doc = new QTextDocument();
                doc->setParent(nullptr);
                newBranch->setText(0, item.name());
                newBranch->setData(0, Qt::UserRole, id);
                tree->setTextDocument(id, doc);
            }
        } else break;
    }
    if (invalidJson && invalidText) return false;

    if (parent != nullptr) {
        parent->insertChild(row, newBranch);
        tree->saveUndo(new InsertItemCommand(mUi->treeWidget, parent, row, newBranch, item));
        changed();
    }

    return true;
}

void Main::registerImages(const QString& html, QTextDocument* doc) {
    for (const auto& entry: mImageStore) {
        if (html.contains("<img src=\"" + entry.first)) {
            doc->addResource(QTextDocument::ImageResource, entry.first, entry.second);
        }
    }
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

// save needs to add the novel to the recent novel list (in case it's new and just saved, if it exists, this is a no-op).
void Main::save(Novel& novel, Map<qlonglong, bool>& byId, qlonglong pos, const QRect& geom, bool noUi) {
    if ((novel.filename().isEmpty() && noUi) || !novel.isChanged()) return;
    if (mNovel.filename().isEmpty() && !doSaveAs()) return;
    QFileInfo info(mNovel.filename());
    QString name = info.fileName();
    mDocDir = info.absolutePath();
    mPrefs.addNovel(name, mNovel.filename());
    Json5Object extra;
    extra[Novel::Current] = mCurrentNode;
    extra[Novel::Position] = qlonglong(pos);
    mPrefs.setWindowLocation(geom);
    extra[Novel::Prefs] = mPrefs.write();
    mState = byId;
    Json5Array state;
    for (const auto& idState: byId) {
        Json5Array item;
        item.append(idState.first);
        item.append(idState.second);
        state.append(item);
    }
    extra["State"] = state;
    Json5Array arr;
    auto& images = mUi->textEdit->internalImages();
    auto keys = images.keys();
    for (auto i = 0; i < keys.count(); ++i) {
        auto& key = keys[i];
        Json5Object obj;
        obj[Novel::Url] = key.toString();
        QImage& img = images[key];
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");
        buffer.close();
        obj[Novel::Data] = QString(bytes.toBase64());
        arr.append(obj);
    }
    extra[Novel::Images] = arr;
    StringList dictionary = mSpelling.words();
    Json5Array words;
    for (const auto& word: dictionary) words.append(word);
    extra[Novel::Dictionary] = words;
    mNovel.setExtra(extra);
    TreeNode tree = saveTree(mUi->treeWidget->topLevelItem(0));
    mNovel.setBranches(tree);
    if (!mNovel.save(saveCompression) && !noUi) {
        mMsg.OK("Unable to save the file.\n\nTry and save it under a different name\nor save it to a different directory.",
                [this]() { doNothing(); },
                "Something unexpected has happened");
    }
    else setTitle();
}

TreeNode Main::saveTree(QTreeWidgetItem* node) {
    TreeNode part(node->data(0, Qt::UserRole).toLongLong());
    for (auto i = 0; i < node->childCount(); ++i) part.addBranch(saveTree(node->child(i)));
    return part;
}

QSize Main::scrollBarSize(QScrollBar* bar) {
    QStyleOptionSlider opt;
    opt.initFrom(bar);
    opt.orientation = bar->orientation();
    opt.minimum = bar->minimum();
    opt.maximum = bar->maximum();
    opt.sliderPosition = bar->sliderPosition();
    opt.sliderValue = bar->value();
    opt.pageStep = bar->pageStep();
    opt.singleStep = bar->singleStep();
    opt.upsideDown = false;
    if (bar->orientation() == Qt::Horizontal) opt.state |= QStyle::State_Horizontal;

    QRect handleRect = bar->style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, bar);

    return { handleRect.width(), handleRect.height() };
}

void Main::selectionItems() {
    bool selectionMade = mUi->textEdit->textCursor().selectedText().length() != 0;
    mUi->actionCopy->setEnabled(selectionMade);
    mUi->actionCut->setEnabled(selectionMade);
    mUi->actionLowercase->setEnabled(selectionMade);
    mUi->actionUppercase->setEnabled(selectionMade);
}

void Main::setDoc(qlonglong id) {
    Item& item = mNovel.findItem(id);
    mUi->textEdit->setDocument(item.doc());

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
    applyNovelFormatting();
    doCursorPositionChanged();
}

void Main::setIcon(QToolButton* button, bool isChecked) {
    button->setChecked(isChecked);
}

void Main::setMenu(QAction* menuItem, bool isChecked) {
    menuItem->setChecked(isChecked);
}

void Main::setPosition(qlonglong pos) {
    auto cursor = mUi->textEdit->textCursor();
    cursor.setPosition(pos);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::setTitle() {
    QString title = "TextSmith";
    if (!mNovel.filename().isEmpty()) {
        QFileInfo info(mNovel.filename());
        title += " - " + info.fileName();
    }
    if (!mNovel.isChanged()) title += " [Saved]";
    mUi->actionSave->setEnabled(mNovel.isChanged());
    setWindowTitle(title);
}

void Main::update(bool unchanged) { // new, open
    QTreeWidget* tree = mUi->treeWidget;
    tree->clear();
    buildTree(mNovel.branches(), nullptr, mState);
    mImageStore.clear();
    updateDocument();
    updateFromPrefs();

    mUi->textEdit->setFocus();
    if (unchanged) clearChanged();
}

void Main::updateFromPrefs() {
    if (mPrefs.autoSave()) mTimer.start(mPrefs.autoSaveIntyerval() * Second);
    else mTimer.stop();

    mUi->actionRead_To_Me->setEnabled(mPrefs.voice() >= 0);
    QFont uiFont(mPrefs.uiFontFamily(), mPrefs.uiFontSize());
    QApplication::setFont(uiFont);
    mUi->menubar->setFont(uiFont);
    updateGeometry();
    repaint();

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
        for (auto& m: mPrefs.mainSplitter()) arr.append(m);
        mUi->splitter->setSizes(arr.toQList());
    }
}

void Main::updateDocument() { // update, and post-fullscreen
    QTreeWidget* tree = mUi->treeWidget;
    QTreeWidgetItem* branch = findItem(tree->topLevelItem(0), mCurrentNode);
    if (branch != nullptr) {
        Item& item = mNovel.findItem(mCurrentNode);
        mUi->textEdit->setDocument(item.doc());
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

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
        updateFromPrefs();
    });

#if defined(Q_OS_LINUX) && QT_CONFIG(dbus)
    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Settings",
        "SettingChanged", this, SLOT(dbusChanged(QString, QString, QDBusVariant)));
#endif

    mSoundPool.load(SoundPool::Sound::KeyWhack,    QUrl("qrc:/Sound/KeyWhack.wav"),    8);
    mSoundPool.load(SoundPool::Sound::SpaceThunk,  QUrl("qrc:/Sound/SpaceThunk.mp3"),  8);
    mSoundPool.load(SoundPool::Sound::ReturnRoll,  QUrl("qrc:/Sound/ReturnRoll.mp3"),  8);
    mSoundPool.load(SoundPool::Sound::ReturnClunk, QUrl("qrc:/Sound/ReturnClunk.mp3"), 8);
    mSoundPool.load(SoundPool::Sound::MarginDing,  QUrl("qrc:/Sound/MarginDing.mp3"),  8);

    mSpeechAvailable = Speech::speechAvailable(this);

    mCurrentNode = mNovel.root();

    QDir().mkpath(mAppDir);
    QDir().mkpath(mDocDir);
    QDir().mkpath(mDocDir + "/TextSmith");
    QDir().mkpath(mDocDir + "/TextSmith/scripts");
    QDir().mkpath(mLocalDir);

    setupActions();
    setupConnections();
    setupScripting();
    setupTabOrder();

    mUi->actionRead_To_Me->setEnabled(mSpeechAvailable);

    mPrefsLoaded = mPrefs.load();
    loadScripts();
    setupIcons();
    bool isVisible = mPrefs.toolbarVisible();
    mUi->actionHide_Show_Toolbar->setText(isVisible ? "Hide Toolbars" : "Show Toolbars");
    QFont font(mPrefs.uiFontFamily(), mPrefs.uiFontSize());
    QApplication::setFont(font);
    mUi->menubar->setFont(font);
    updateGeometry();
    repaint();

    mUi->treeWidget->setColumnCount(1);
    mUi->textEdit->setTabChangesFocus(true);

    StringList arguments { app->arguments() };
    if (arguments.size() > 1) loadFile(arguments[1]);
    else {
        doNew();
        Item::resetLastID(1);
    }
    updateFromPrefs();
    clearChanged();
}

Main::~Main() {
    QList<int> array;
    for (auto size: mUi->splitter->sizes()) array.append(int(size));
    mPrefs.setMainSplitter(array);
    delete mUi;
}

QTreeWidgetItem* Main::buildTreeFromJson(Json5Object& obj) {
    Item item;
    item.setHtml(Item::hasStr(obj, "Html"));
    item.setName(Item::hasStr(obj, "Name"));
    item.setPosition(Item::hasNum(obj, "Position", qlonglong(0)));
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
    QTextDocument* doc = new QTextDocument();
    doc->setParent(nullptr);
    branch->setText(0, item.name());
    branch->setData(0, Qt::UserRole, item.id());
    mUi->treeWidget->setTextDocument(item.id(), doc);
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
    QString html = Item::setupDocument(font);
    text.document()->setDefaultFont(font);
    text.document()->setHtml(html);
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

QList<qlonglong> Main::vectorOfIds(QTreeWidgetItem* branch, const StringList& tags, StartingFlag starting) {
    StringList usingTags;
    for (const auto& tag: tags) usingTags += Main::splitToList(tag);

    QList<qlonglong> ids;
    auto id = branch->data(0, Qt::UserRole).toLongLong();
    Item& item = mNovel.findItem(id);

    if (usingTags.isEmpty() || item.hasTag(usingTags)) ids.append(id);
    else if (starting != StartingFlag::Starting) return ids;

    for (auto childNum = 0; childNum < branch->childCount(); ++childNum) {
        auto childItems = vectorOfIds(branch->child(childNum), usingTags, StartingFlag::Continuing);
        ids.append(childItems);
    }
    return ids;
}

void Main::wordCounts() {
    auto oldCount = mWordCount.total();
    auto newCount = mNovel.countAll();
    auto difference = newCount - oldCount;
    auto current = mNovel.count(mCurrentNode);
    mWordCount.setCurrentItem(current);
    mWordCount.setTotal(newCount);
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
    mDocMetaType = qRegisterMetaType<QTextDocument*>("QTextDocument*");

    mUi->actionExit->setShortcut(QKeySequence("Alt+F4"));
    connect(mUi->actionExit,       &QAction::triggered, this, &Main::exitAction);
    connect(mUi->actionNew,        &QAction::triggered, this, &Main::newAction);
    connect(mUi->actionOpen,       &QAction::triggered, this, &Main::openAction);
    connect(mUi->actionPage_Setup, &QAction::triggered, this, &Main::pageSetup);
    connect(mUi->actionPrint,      &QAction::triggered, this, &Main::printAction);
    connect(mUi->actionSave,       &QAction::triggered, this, &Main::saveAction);
    connect(mUi->actionSave_As,    &QAction::triggered, this, &Main::saveAsAction);

    // Note: we remeber /all/ of the novels worked on, we just delete the ones that have been delete and suddenly you can see older novels!
    mUi->action_Clear_Recent_Novels->setShortcut(QKeySequence("Alt+C,Alt+R"));
    connect(mUi->action_Clear_Recent_Novels, &QAction::triggered, this, &Main::clearRecentNovels);
    auto* recentMenu = mUi->menu_Recent_Novels;
    auto recent = mPrefs.recentNovels();
    List<Preferences::Recent> gone;
    int count = 0;
    for (const auto& novel: recent) {
        QFileInfo info(novel.sPath);
        if (!info.exists()) {
            gone.append(novel);
            continue;
        }
        if (count++ >= 10) continue;
        QAction* act = recentMenu->addAction(novel.sTitle);
        act->setShortcut(QKeySequence("Ctrl+" + QString::number((count == 10) ? 0 : count)));
        act->setShortcutContext(Qt::ApplicationShortcut);

        connect(act, &QAction::triggered, this, [this, novel]() {
            loadFile(novel.sPath);
        });
    }
    for (const auto& file: gone) recent.remove(file);
    mPrefs.setRecentNovels(recent);

    QMenu* exportMenu = new QMenu("Export", this);
    mUi->actionExport->setMenu(exportMenu);

    const auto& map = ExporterBase::registry();

    QString keysUsed;
    for (const QString& info: map.keys()) {
        QAction* act = exportMenu->addAction(info);
        QString key = findKey(keysUsed, info);
        if (!key.isEmpty()) act->setShortcut(QKeySequence("Ctrl+E, Ctrl+" + key));
        act->setShortcutContext(Qt::ApplicationShortcut);

        connect(act, &QAction::triggered, this, [this, info]() {
            doExport(info);
        });
    }

    connect(mUi->actionBold,               &QAction::triggered, this, &Main::boldAction);
    connect(mUi->actionCenter,             &QAction::triggered, this, &Main::centerJustifyAction);
    connect(mUi->actionCopy,               &QAction::triggered, this, &Main::copyAction);
    connect(mUi->actionCut,                &QAction::triggered, this, &Main::cutAction);
    connect(mUi->actionFind_Next,          &QAction::triggered, this, &Main::findNext);
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

    mUi->actionRead_To_Me->setShortcut(QKeySequence("Ctrl+Alt+R"));
    connect(mUi->actionDistraction_Free,  &QAction::triggered, this, &Main::fullScreenAction);
    connect(mUi->actionRead_To_Me,        &QAction::triggered, this, &Main::readToMeAction);
    connect(mUi->actionWord_Count,        &QAction::triggered, this, &Main::doWordCount);
    connect(mUi->actionSpell_checking,    &QAction::triggered, this, &Main::spellCheck);
    connect(mUi->actionHide_Show_Toolbar, &QAction::triggered, this, &Main::toolBars);

    connect(mUi->actionAbout_TextSmith, &QAction::triggered, this, &Main::aboutDialog);
    connect(mUi->actionHelp,            &QAction::triggered, this, &Main::helpDIalog);
    connect(mUi->actionWeb_Site,        &QAction::triggered, this, &Main::web);
    connect(mUi->actionScripting,       &QAction::triggered, this, &Main::manageScripts);

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

    mUi->treeWidget->setDragEnabled(true);
    mUi->treeWidget->setDropIndicatorShown(true);
    mUi->treeWidget->setAcceptDrops(true);
    mUi->treeWidget->setDragDropMode(QAbstractItemView::DragDrop);
    mUi->treeWidget->setDefaultDropAction(Qt::MoveAction);
    mUi->treeWidget->setMimeDataBuilder(std::bind(&Main::buildTreeMimeData,    this, std::placeholders::_1, std::placeholders::_2));
    mUi->treeWidget->setMimeDataReceiver(std::bind(&Main::receiveTreeMimeData, this, std::placeholders::_1, std::placeholders::_2));
    mUi->treeWidget->setMimeCanPaste(std::bind(&Main::canPasteMimeData,        this, std::placeholders::_1));

    mUi->textEdit->setAcceptDrops(true);
    mUi->textEdit->setAcceptRichText(true);

    connect(qApp, &QApplication::focusChanged, this, &Main::focusChanged);

    mSpeaking = new QWidget();
    mSpeechWidget = new Ui_SpeechWidget();
    mSpeechWidget->setupUi(mSpeaking);
    mSpeaking->hide();
    statusBar()->addPermanentWidget(mSpeaking);

    connect(mSpeechWidget->stopPushButton,      &QPushButton::clicked,     this, &Main::stop);
    connect(mSpeechWidget->pausePlayPushButton, &QPushButton::clicked,     this, &Main::pause);
    connect(&mSpeech,                           &Speech::speaking,         this, &Main::highlight);
    connect(&mSpeech,                           &Speech::speakingFinished, this, &Main::stopped);

    QShortcut* editItemShortcut = new QShortcut(QKeySequence("F2"), this);
    connect(editItemShortcut, &QShortcut::activated, this, &Main::doEditItem);

    mFindLine = new QWidget();
    mFindWidget = new Ui_Widget();
    mFindWidget->setupUi(mFindLine);
    mFindLine->hide();
    statusBar()->addPermanentWidget(mFindLine);

    connect(mFindWidget->findLineEdit,            &QLineEdit::textEdited,        this, &Main::findChanged);
    connect(mFindWidget->caseInsensitiveCheckBox, &QCheckBox::checkStateChanged, this, &Main::findReplace);
    auto buttonGroup = new QButtonGroup();
    mFindWidget->thisItemRadioButton->setChecked(true);
    buttonGroup->addButton(mFindWidget->thisItemRadioButton);
    buttonGroup->addButton(mFindWidget->thisItemAndChildItemsRadioButton);
    connect(buttonGroup, &QButtonGroup::buttonClicked, this, &Main::findReplace);

    QShortcut* replaceShortcut =    new QShortcut(QKeySequence("Ctrl+R"), this);
    QShortcut* replaceAllShortcut = new QShortcut(QKeySequence("Ctrl+Shift+R"), this);
    QShortcut* findDoneShortcut =   new QShortcut(QKeySequence("Esc"), this);
    connect(mFindWidget->replaceLineEdit,      &QLineEdit::textEdited, this, &Main::replaceChanged);
    connect(replaceShortcut,                   &QShortcut::activated,  this, &Main::replace);
    connect(mFindWidget->replacePushButton,    &QPushButton::toggled,  this, &Main::replace);
    connect(replaceAllShortcut,                &QShortcut::activated,  this, &Main::replaceAll);
    connect(mFindWidget->replaceAllPushButton, &QPushButton::toggled,  this, &Main::replaceAll);
    connect(findDoneShortcut,                  &QShortcut::activated,  this, &Main::barDone);

    mSpellcheck = new QWidget();
    mSpellWidget = new Ui_SpellWidget();
    mSpellWidget->setupUi(mSpellcheck);
    auto* area = mSpellWidget->scrollAreaWidgetContents;
    auto *layout = new QGridLayout(area);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    mSpellcheck->hide();
    statusBar()->addPermanentWidget(mSpellcheck);

    connect(mSpellWidget->stopPushButton,  &QPushButton::clicked, this, &Main::spellcheckDone);
    connect(mSpellWidget->nextPushButton,  &QPushButton::clicked, this, &Main::spellcheckNext);
    connect(mSpellWidget->leftPushButton,  &QPushButton::clicked, this, &Main::scrollSpellcheckLeft);
    connect(mSpellWidget->rightPushButton, &QPushButton::clicked, this, &Main::scrollSpellcheckRight);

    QLineEdit *commandLine = new QLineEdit;
    commandLine->setPlaceholderText("Type a command...");
    commandLine->hide();
    statusBar()->addPermanentWidget(commandLine);

    QShortcut* paletteShortcut = new QShortcut(QKeySequence("Ctrl+Shift+C"), this);
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

void Main::setupScripting() {
    delete mVm;
    mVm = new fifth::vm;
    if (!mVm) return;

    // Menu words
    mVm->addBuiltin("exit",   [this](fifth::vm*) { doExit(); });
    mVm->addBuiltin("new",    [this](fifth::vm*) { doNew(); });
    mVm->addBuiltin("open",   [this](fifth::vm*) { doOpen(); });
    mVm->addBuiltin("print",  [this](fifth::vm*) { doPrint(); });
    mVm->addBuiltin("save",   [this](fifth::vm*) { doSave(); });
    mVm->addBuiltin("saveAs", [this](fifth::vm*) { doSaveAs(); });

    mVm->addBuiltin("bold",         [this](fifth::vm*) { doBold(); });
    mVm->addBuiltin("center",       [this](fifth::vm*) { doCenterJustify(); });
    mVm->addBuiltin("copy",         [this](fifth::vm*) { doCopy(); });
    mVm->addBuiltin("cut",          [this](fifth::vm*) { doCut(); });
    mVm->addBuiltin("findnext",     [this](fifth::vm*) { doFindNext(); });
    mVm->addBuiltin("findreplace",  [this](fifth::vm*) { doFindReplace(); });
    mVm->addBuiltin("fulljustify",  [this](fifth::vm*) { doFullJustify(); });
    mVm->addBuiltin("indent",       [this](fifth::vm*) { doIndent(); });
    mVm->addBuiltin("italic",       [this](fifth::vm*) { doItalic(); });
    mVm->addBuiltin("leftjustify",  [this](fifth::vm*) { doLeftJustify(); });
    mVm->addBuiltin("lowercase",    [this](fifth::vm*) { doLowercase(); });
    mVm->addBuiltin("outdent",      [this](fifth::vm*) { doOutdent(); });
    mVm->addBuiltin("paste",        [this](fifth::vm*) { doPaste(); });
    mVm->addBuiltin("preferences",  [this](fifth::vm*) { doPreferences(); });
    mVm->addBuiltin("redo",         [this](fifth::vm*) { doRedo(); });
    mVm->addBuiltin("rightjustify", [this](fifth::vm*) { doRightJustify(); });
    mVm->addBuiltin("underline",    [this](fifth::vm*) { doUnderline(); });
    mVm->addBuiltin("undo",         [this](fifth::vm*) { doUndo(); });
    mVm->addBuiltin("uppercase",    [this](fifth::vm*) { doUppercase(); });

    mVm->addBuiltin("addbranch", [this](fifth::vm*) { doAddItem(); });
    mVm->addBuiltin("closeall",  [this](fifth::vm*) { doCloseAll(); });
    mVm->addBuiltin("edititem",  [this](fifth::vm*) { doEditItem(); });
    mVm->addBuiltin("movedown",  [this](fifth::vm*) { doMoveDown(); });
    mVm->addBuiltin("moveout",   [this](fifth::vm*) { doMoveOut(); });
    mVm->addBuiltin("moveup",    [this](fifth::vm*) { doMoveUp(); });
    mVm->addBuiltin("openall",   [this](fifth::vm*) { doOpenAll(); });
    mVm->addBuiltin("removeall", [this](fifth::vm*) { doRemoveItem(); });

    mVm->addBuiltin("fullscreen", [this](fifth::vm*) { doFullScreen(); });
    mVm->addBuiltin("readtome",   [this](fifth::vm*) { doReadToMe(); });
    mVm->addBuiltin("spellcheck", [this](fifth::vm*) { doSpellcheck(); });
    mVm->addBuiltin("wordcount",  [this](fifth::vm*) { doWordCount(); });

    mVm->addBuiltin("about",        [this](fifth::vm*) { doAboutDialog(); });
    mVm->addBuiltin("help",         [this](fifth::vm*) { doHelp(); });
    mVm->addBuiltin("web",          [this](fifth::vm*) { doWeb(); });
    mVm->addBuiltin("manageScripts",[this](fifth::vm*) { doManageScripts(); });

    // TextSmith2 words
    mVm->addBuiltin("shortcut", [this](fifth::vm* vm) { //    shortcut -u-> // short cut is expected to be followed by code block { }
        auto& user = vm->user();
        QString key = user.pop().asString().str();
        if (key.isEmpty()) {
            user.push(0);
            return;
        }
        QShortcut* shortcut = new QShortcut(QKeySequence(key), this);
        mShortcut[shortcut] = nullptr;
        user.push(qlonglong(shortcut));
        connect(shortcut, &QShortcut::activated, this, [this, shortcut]() {
            auto& user = mVm->user();
            auto& body = mShortcut[shortcut];
            if (body.isExe()) {
                auto call = body.asCallable();
                if (call) {
                    auto comp = dynamic_cast<fifth::compiled*>(call);
                    comp->eval(mVm);
                }
            } else if (body.isStr() || body.isNum()) user.push(body);
            else user.push(0);
        });
    });
    mVm->addBuiltin("action", [this](fifth::vm* vm) {  //  name [shortcut] -u-> action /* compile code between { } if not shortcut */
        auto& user = vm->user();
        auto s = user.pop();
        QString name;
        fifth::exe body = s.asCallable();
        s = user.pop();
        if (s.isNum()) {
            QShortcut* ptr = (QShortcut*)(s.asNumber());
            if (!mShortcut.contains(ptr)) return;
            mShortcut[ptr] = body;
            s = user.pop();
        } else return;
        name = s.asString().str();
        if (name.isEmpty()) return;
        QAction* action = new QAction(name, this);
        connect(action, &QAction::activate, this, [this, body]() {
            auto comp = dynamic_cast<fifth::compiled*>(body);
            comp->eval(mVm);
        });
        user.push(qlonglong(action));
    });
    mVm->addBuiltin("menu", [this](fifth::vm* vm) { // menu-path action -u->
        auto& user = vm->user();
        auto a = user.pop();
        auto m = user.pop();
        QAction* action = (QAction*)(a.asNumber());
        if (action == nullptr) return;
        QString menuPath = m.asString().str();
        if (!m.isStr() || menuPath.isEmpty()) return;
        addActionToMenuPath(mUi->menubar, action, menuPath);
    });
    mVm->addBuiltin("getfilename", [this](fifth::vm* vm) { //   -u-> filename
        QString filename = QFileDialog::getSaveFileName(this, "Filename to write to?", mDocDir, "All Files (*.*)");
        if (!filename.isEmpty()) {
            QFileInfo info(filename);
            if (info.exists()) filename = "";
            else mDocDir = info.absolutePath();
        }
        vm->user().push(filename);
    });
    mVm->addBuiltin("writefile", [](fifth::vm* vm) {
        auto& user = vm->user();
        QString filename = user.pop().asString().str();
        int result = 0;
        if (!filename.isEmpty()) {
            QFileInfo info(filename);
            if (!info.exists()) {
                QString doc;
                auto num = user.pop().asNumber();
                for (auto i = 0; i < num; ++i) doc += user.pop().asString().str() + "\n";
                QFile file(filename);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                    QTextStream out(&file);
                    out << doc;
                    file.close();
                    result = 1;
                }
            }
        }
        user.push(result);
    });
    mVm->addBuiltin("message", [this](fifth::vm* vm) { mMsg.Statement(vm->user().pop().asString().str()); }); // msg -u->
    mVm->addBuiltin("yesno", [this](fifth::vm* vm) { // question -u-> 1|0
        auto& user = vm->user();
        mMsg.YesNo(user.pop().asString().str(),
                   [&user]() { user.push(1); },
                   [&user]() { user.push(0); },
                   "Yes, or No?");
    });
    mVm->addBuiltin("okcancel", [this](fifth::vm* vm) { // question -u-> 1|0
        auto& user = vm->user();
        mMsg.OKCancel(user.pop().asString().str(),
                     [&user]() { user.push(1); },
                     [&user]() { user.push(0); },
                     "Okay, or Cancel?");
    });
    mVm->addBuiltin("yesnocancel", [this](fifth::vm* vm) { // question -u-> 1|0|-1
        auto& user = vm->user();
        mMsg.YesNoCancel(user.pop().asString().str(),
                        [&user]() { user.push(1); },
                        [&user]() { user.push(0); },
                        [&user]() { user.push(-1); },
                        "Yes, No, or Cancel?");
    });

    // List words (more)
    mVm->addBuiltin("reverse", [](fifth::vm* vm) {
        auto& user = vm->user();
        QList<fifth::value> contents;
        auto num = user.pop().asNumber();
        for (auto i = 0; i < num; ++i) contents.append(user.pop());
        for (auto i = 0; i < num; ++i) user.push(contents[i]);
        user.push(num);
    });
    // String words (more)
    mVm->addBuiltin("html2para", [this](fifth::vm* vm) { // id -u-> para[n] ... para[1] n
        auto& user = vm->user();
        Item& item = fifthItem(user);
        QTextEdit edit;
        edit.setHtml(item.html());
        StringList paragraphs = edit.toPlainText().split("\n", Qt::SkipEmptyParts);
        for (auto i = paragraphs.count(); i != 0; --i) user.push(paragraphs[i - 1]);
        user.push(paragraphs.count());
    });
    mVm->addBuiltin("para2words", [](fifth::vm* vm) { // para -u-> word[n] ... word[1] n
        auto& user = vm->user();
        auto s = user.pop();
        auto paragraph = s.asString().str();
        auto words = paragraph.split(" ", Qt::SkipEmptyParts);
        for (auto i = words.count(); i != 0; --i) user.push(words[i - 1]);
        user.push(words.count());
    });
    mVm->addBuiltin("tolower", [](fifth::vm* vm){
        auto& user = vm->user();
        auto s = user.pop();
        auto str = s.asString().str();
        str = str.toLower();
        user.push(str);
    });
    mVm->addBuiltin("toupper", [](fifth::vm* vm){
        auto& user = vm->user();
        auto s = user.pop();
        auto str = s.asString().str();
        str = str.toUpper();
        user.push(str);
    });
    mVm->addBuiltin("unpunct", [](fifth::vm* vm){
        static QRegularExpression punctuation(R"([.,!?;:"'()\[\]{}\-_/\\@#%&*+=<>~`^|])");
        auto& user = vm->user();
        auto s = user.pop();
        auto str = s.asString().str();
        str.remove(punctuation);
        user.push(str);
    });
    mVm->addBuiltin("split", [](fifth::vm* vm) { // string splitBy -u-> part[n] ... part[1] n
        auto& user = vm->user();
        auto s2 = user.pop();
        auto s1 = user.pop();
        QString string = s1.asString().str();
        QString splitBy = s2.asString().str();
        StringList parts;
        if (splitBy.isEmpty()) user.push(string);
        else {
            parts = { string.split(splitBy) };
            for (auto i = parts.count(); i != 0; i--) user.push(parts[i - 1]);
        }
        user.push(parts.count());
    });
    mVm->addBuiltin("find", [](fifth::vm* vm) { //  string target -u-> position of target in  string
        auto& user = vm->user();
        auto t = user.pop();
        auto s = user.pop();
        QString string = s.asString().str();
        QString target = t.asString().str();
        user.push(string.indexOf(target));
    });
    mVm->addBuiltin("replace", [](fifth::vm* vm) { //  string target with -u-> new string
        auto& user = vm->user();
        auto w = user.pop();
        auto t = user.pop();
        auto s = user.pop();
        QString string = s.asString().str();
        QString target = t.asString().str();
        QString with = w.asString().str();
        user.push(string.replace(target, with));
    });

    // Novel words
    mNovel.setupScripting(mVm);

    // Edit words
    mVm->addBuiltin("selection", [this](fifth::vm* vm) { //   -u-> start end
        auto& user = vm->user();
        auto cursor = mUi->textEdit->textCursor();
        user.push(cursor.selectionStart());
        user.push(cursor.selectionEnd());
    });
    mVm->addBuiltin("setselection", [this](fifth::vm* vm) { // start end -u->
        auto& user = vm->user();
        auto e = user.pop();
        auto s = user.pop();
        auto cursor = mUi->textEdit->textCursor();
        cursor.setPosition(s.asNumber());
        cursor.setPosition(e.asNumber(), QTextCursor::KeepAnchor);
    });
    mVm->addBuiltin("replaceselection", [this](fifth::vm* vm) { //  text -u->
        auto& user = vm->user();
        QString text = user.pop().asString().str();
        auto cursor = mUi->textEdit->textCursor();
        cursor.removeSelectedText();
        cursor.insertText(text);
    });
    mVm->addBuiltin("textposition",    [this](fifth::vm* vm) { vm->user().push(mUi->textEdit->textCursor().position()); }); //   -u-> position
    mVm->addBuiltin("selectedtext",    [this](fifth::vm* vm) { vm->user().push(mUi->textEdit->textCursor().selectedText()); }); //  -u-> text
    mVm->addBuiltin("settextposition", [this](fifth::vm* vm) { mUi->textEdit->textCursor().setPosition(vm->user().pop().asNumber()); }); //  position -u->
    mVm->addBuiltin("plaintext",       [this](fifth::vm* vm) { vm->user().push(mUi->textEdit->toPlainText()); }); //   -u-> text

    // TreeWidget words
    mVm->addBuiltin("rootid",        [this](fifth::vm* vm) { vm->user().push(mUi->treeWidget->topLevelItem(0)->data(0, Qt::UserRole).toLongLong()); }); //    -u-> id
    mVm->addBuiltin("currentbranch", [this](fifth::vm* vm) { vm->user().push(mCurrentNode); }); //    -u-> id
    mVm->addBuiltin("setbranch",     [this](fifth::vm* vm) { mUi->treeWidget->setCurrentItem(findItem(mUi->treeWidget->topLevelItem(0), vm->user().pop().asNumber())); }); // id -u->
    mVm->addBuiltin("getchildren",   [this](fifth::vm* vm) {  // id -u-> id[n] ... id[1] n
        auto& user = vm->user();
        auto i = user.pop();
        QList<qlonglong> children;
        auto* branch = findItem(mUi->treeWidget->topLevelItem(0), i.asNumber());
        if (branch != nullptr) {
            auto num = branch->childCount();
            for (auto i = 0; i < num; ++i) children.append(branch->child(i)->data(0, Qt::UserRole).toLongLong());
        }
        for (auto i = children.count(); i != 0; --i) user.push(children[i - 1]);
        user.push(children.count());
    });
}

QIcon Main::createResponsiveIcon(const QString &path, int targetSize) {
    QPixmap source(path);

    QPixmap brightPix = source.scaled(targetSize, targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap dimPix(brightPix.size());
    if (path.contains("Ck")) {
        dimPix.fill(Qt::transparent);

        QPainter p(&dimPix);
        p.setOpacity(0.3); // 40% Opacity (Adjust for your eyes)
        p.drawPixmap(0, 0, brightPix);
        p.end();
    } else dimPix = brightPix;

    QIcon icon;

    // State: Normal / Off -> Show DIM
    icon.addPixmap(dimPix, QIcon::Normal, QIcon::Off);

    // State: Hover / Active -> Show BRIGHT
    icon.addPixmap(brightPix, QIcon::Active,   QIcon::Off);
    icon.addPixmap(brightPix, QIcon::Selected, QIcon::Off);

    // State: Checked (e.g. Bold is ON) -> Show BRIGHT
    icon.addPixmap(brightPix, QIcon::Normal, QIcon::On);

    return icon;
}

void Main::setupIcons() {
    mIcons["newItemToolButton"] =        createResponsiveIcon(":/icon/newItem.ico", 24);
    mIcons["deleteItemToolButton"] =     createResponsiveIcon(":/icon/deleteItem.ico", 24);
    mIcons["upToolButton"] =             createResponsiveIcon(":/icon/moveItemUp.ico", 24);
    mIcons["downToolButton"] =           createResponsiveIcon(":/icon/moveItemDown.ico", 24);
    mIcons["outToolButton"] =            createResponsiveIcon(":/icon/moveItemOut.ico", 24);
    mIcons["boldToolButton"] =           createResponsiveIcon(":/icon/boldCk.ico", 24);
    mIcons["italicToolButton"] =         createResponsiveIcon(":/icon/italicCk.ico", 24);
    mIcons["underlineToolButton"] =      createResponsiveIcon(":/icon/underlineCk.ico", 24);
    mIcons["leftJustifyToolButton"] =    createResponsiveIcon(":/icon/leftjustifyCk.ico", 24);
    mIcons["centerToolButton"] =         createResponsiveIcon(":/icon/CenterCk.ico", 24);
    mIcons["rightJustifyToolButton"] =   createResponsiveIcon(":/icon/rightJustifyCk.ico", 24);
    mIcons["fullJustifyToolButton"] =    createResponsiveIcon(":/icon/fullJustifyCk.ico", 24);
    mIcons["increaseIndentToolButton"] = createResponsiveIcon(":/icon/LtincreseIndent.ico", 24);
    mIcons["decreaseIndentToolButton"] = createResponsiveIcon(":/icon/LtdecreaseIndent.ico", 24);

    mIcons["LtnewItemToolButton"] =        createResponsiveIcon(":/icon/LtnewItem.ico", 24);
    mIcons["LtdeleteItemToolButton"] =     createResponsiveIcon(":/icon/LtdeleteItem.ico", 24);
    mIcons["LtupToolButton"] =             createResponsiveIcon(":/icon/LtmoveItemUp.ico", 24);
    mIcons["LtdownToolButton"] =           createResponsiveIcon(":/icon/LtmoveItemDown.ico", 24);
    mIcons["LtoutToolButton"] =            createResponsiveIcon(":/icon/LtmoveItemOut.ico", 24);
    mIcons["LtboldToolButton"] =           createResponsiveIcon(":/icon/LtboldCk.ico", 24);
    mIcons["LtitalicToolButton"] =         createResponsiveIcon(":/icon/LtitalicCk.ico", 24);
    mIcons["LtunderlineToolButton"] =      createResponsiveIcon(":/icon/LtunderlineCk.ico", 24);
    mIcons["LtleftJustifyToolButton"] =    createResponsiveIcon(":/icon/LtleftjustifyCk.ico", 24);
    mIcons["LtcenterToolButton"] =         createResponsiveIcon(":/icon/LtCenterCk.ico", 24);
    mIcons["LtrightJustifyToolButton"] =   createResponsiveIcon(":/icon/LtrightJustifyCk.ico", 24);
    mIcons["LtfullJustifyToolButton"] =    createResponsiveIcon(":/icon/LtfullJustifyCk.ico", 24);
    mIcons["LtincreaseIndentToolButton"] = createResponsiveIcon(":/icon/LtincreseIndent.ico", 24);
    mIcons["LtdecreaseIndentToolButton"] = createResponsiveIcon(":/icon/LtdecreaseIndent.ico", 24);
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

qlonglong Main::skipSpaces(const QString& str, qlonglong pos) {
    while (pos < str.size() && str[pos].isSpace()) ++pos;
    return pos;
}

void Main::changeDocumentFont(QTextDocument* doc, const QFont& font) {
    QString html = doc->toHtml();
    html = Item::changeFont(doc, font);
    doc->setHtml(html);
    Main::ref().ui()->textEdit->setWrapMargin();
}

StringList Main::splitToList(const QString& tag) {
    StringList tags { tag.split(",") };
    StringList resultTags;
    for (auto& tag: tags) resultTags += tag.trimmed();
    return resultTags;
}

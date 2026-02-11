#pragma once

#include <functional>

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QDropEvent>
#include <QHash>
#include <QMimeData>
#include <QPainter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUndoCommand>
#include <QUndoStack>

class InsertItemCommand;
class QTextDocument;

class TreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit TreeWidget(QWidget *parent = nullptr)
        : QTreeWidget(parent)
        , mUndoStack(new QUndoStack(this)) { }
    ~TreeWidget() { delete mUndoStack; }

    using QTreeWidget::QTreeWidget;

    std::function<void(const QList<QTreeWidgetItem*>&, QMimeData*)>  mimeDataBuilder()  { return mBuildMimeData; }
    std::function<bool(QDropEvent*, const QMimeData*)>               mimeDataReciever() { return mReceiveMimeData; }
    std::function<bool(const QMimeData*)>                            mimeCanPaste()     { return mCanPasteMimeData; }

    void setMimeDataBuilder(std::function<void(const QList<QTreeWidgetItem*>&, QMimeData*)> func) { mBuildMimeData = func; }
    void setMimeDataReceiver(std::function<bool(QDropEvent*, const QMimeData*)> func)             { mReceiveMimeData = func; }
    void setMimeCanPaste(std::function<bool(const QMimeData*)> func)                              { mCanPasteMimeData = func; }

    void           clearTextDocuments()                             { mDocs.clear(); }
    void           removeTextDocument(qlonglong id)                 { mDocs.remove(id); }
    void           setTextDocument(qlonglong id, QTextDocument* td) { mDocs[id] = td; }
    QTextDocument* textDocument(qlonglong id)                       { return mDocs[id]; }

protected:
    QUndoStack*      mUndoStack;
    QHash<qlonglong, QTextDocument*> mDocs;


    std::function<void(const QList<QTreeWidgetItem*>&, QMimeData*)>  mBuildMimeData =    [](const QList<QTreeWidgetItem*>&, QMimeData*) { };
    std::function<bool(QDropEvent*, const QMimeData*)>               mReceiveMimeData =  [](QDropEvent*, const QMimeData*)              { return false; };
    std::function<bool(const QMimeData*)>                            mCanPasteMimeData = [](const QMimeData*)                           { return false; };

    void dropEvent(QDropEvent* de) override {
        const QMimeData* mime = de->mimeData();

        QTreeWidgetItem *targetItem = itemAt(de->position().toPoint());

        if (!targetItem || targetItem == topLevelItem(0)) {
            if (dropIndicatorPosition() != QAbstractItemView::OnItem) return;
        }

        if (mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
            // Internal move: let QTreeWidget handle it
            QTreeWidget::dropEvent(de);
            if (topLevelItemCount() > 1) {
                QTreeWidgetItem *badItem = topLevelItem(1);
                takeTopLevelItem(1);
                topLevelItem(0)->addChild(badItem);
            }
            return;
        }

        if (mReceiveMimeData(de, mime)) {
            de->acceptProposedAction();
            if (topLevelItemCount() > 1) {
                QTreeWidgetItem *badItem = topLevelItem(1);
                takeTopLevelItem(1);
                topLevelItem(0)->addChild(badItem);
            }
            return;
        }

        de->ignore();
    }

    void startDrag(Qt::DropActions supportedActions) override {
        QTreeWidgetItem* item = currentItem();
        if (!item) return;

        auto mime = itemMimeData(selectedItems());

        QBrush fgBrush = item->foreground(0);
        QPen pen;
        if (fgBrush.style() != Qt::NoBrush) pen = fgBrush.color();
        else pen = this->palette().text().color();

        // Same logic for background (optional, usually you want the base color)
        QBrush bgBrush = item->background(0);
        QColor fill;
        if (bgBrush.style() != Qt::NoBrush) fill = bgBrush.color();
        else fill = this->palette().base().color();

        QFont font = this->font();
        QFontMetrics fm(font);
        QString text = item->text(0);
        QSize size(fm.horizontalAdvance(text) + 8, fm.height() + 4);

        QPixmap pixmap(size);
        pixmap.fill(QColor(fill.red(), fill.green(), fill.blue(), 220));

        QPainter painter(&pixmap);
        painter.setFont(font);
        painter.setPen(pen);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
        painter.end();

        // Start the drag
        QDrag* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
        drag->exec(supportedActions);
    }

    void dragMoveEvent(QDragMoveEvent *event) override {
        QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());

        if (!targetItem) {
            event->ignore(); // Show "No Entry" cursor
            return;
        }

        if (targetItem == topLevelItem(0)) {
            if (dropIndicatorPosition() != QAbstractItemView::OnItem) {
                event->ignore();
                return;
            }
        }

        QTreeWidget::dragMoveEvent(event);
    }

public:
    QMimeData* itemMimeData(const QList<QTreeWidgetItem*> items) const {
        if (items.isEmpty()) return nullptr;

        QMimeData* mime = mimeData(items);
        mBuildMimeData(items, mime);
        return mime;
    }

    void insertFromMimeData(const QMimeData* mime) {
        mReceiveMimeData(nullptr, mime);
    }

    void saveUndo(QUndoCommand* cmd) {
        mUndoStack->push(cmd);
    }

    void clearUndo() {
        mUndoStack->clear();
    }

    bool canPaste() { return mCanPasteMimeData(QApplication::clipboard()->mimeData()); }
    bool canRedo()  { return mUndoStack->canRedo(); }
    bool canUndo()  { return mUndoStack->canUndo(); }
    void redo()     { mUndoStack->redo(); }
    void undo()     { mUndoStack->undo(); }
};


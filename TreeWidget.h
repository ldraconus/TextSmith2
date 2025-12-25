#pragma once

#include <functional>

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QTreeWidget>
#include <QUndoCommand>
#include <QUndoStack>

class InsertItemCommand;

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

protected:
    QUndoStack* mUndoStack;

    std::function<void(const QList<QTreeWidgetItem*>&, QMimeData*)>  mBuildMimeData =    [](const QList<QTreeWidgetItem*>&, QMimeData*) { };
    std::function<bool(QDropEvent*, const QMimeData*)>               mReceiveMimeData =  [](QDropEvent*, const QMimeData*)              { return false; };
    std::function<bool(const QMimeData*)>                            mCanPasteMimeData = [](const QMimeData*)                           { return false; };

    void dropEvent(QDropEvent* de) override {
        const QMimeData* mime = de->mimeData();

        if (mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
            // Internal move: let QTreeWidget handle it
            QTreeWidget::dropEvent(de);
            return;
        }

        if (mReceiveMimeData(de, mime)) {
            de->acceptProposedAction();
            return;
        }

        de->ignore();
    }

    void startDrag(Qt::DropActions supportedActions) override {
        QTreeWidgetItem* item = currentItem();
        if (!item) return;

        auto mime = itemMimeData(selectedItems());

        QColor pen = item->foreground(0).color();
        QFont font = this->font();
        QFontMetrics fm(font);
        QString text = item->text(0);
        QSize size(fm.horizontalAdvance(text) + 8, fm.height() + 4);

        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

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
    bool canRedo() { return mUndoStack->canRedo(); }
    bool canUndo() { return mUndoStack->canUndo(); }
    void redo() { mUndoStack->redo(); }
    void undo() { mUndoStack->undo(); }
};


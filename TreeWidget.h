#pragma once

#include <functional>

#include <QDrag>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QTreeWidget>
#include <QTreeWidgetItem>

class TreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    using QTreeWidget::QTreeWidget;

    std::function<void(QTreeWidgetItem*, QMimeData*)> mimeDataBuilder()  { return mBuildMimeData; }
    std::function<bool(QMimeData*)>                   mimeDataReciever() { return mReceiveMimeData; }

    void setMimeDataBuilder(std::function<void(QTreeWidgetItem*, QMimeData*)> func)  { mBuildMimeData = func; }
    void setMimeDataReceiver(std::function<bool(const QMimeData*)> func)             { mReceiveMimeData = func; }

protected:
    std::function<void(QTreeWidgetItem*, QMimeData*)> mBuildMimeData =   [](QTreeWidgetItem*, QMimeData*) { };
    std::function<bool(const QMimeData*)>             mReceiveMimeData = [](const QMimeData*)             { return false; };

    void dropEvent(QDropEvent* event) override {
        const QMimeData* mime = event->mimeData();

        if (mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
            // Internal move: let QTreeWidget handle it
            QTreeWidget::dropEvent(event);
            return;
        }

        if (mReceiveMimeData(mime)) {
            event->acceptProposedAction();
            return;
        }
/*
        if (mime->hasFormat("application/json5")) {
            QByteArray data = mime->data("application/json5");
            NovelItem *newItem = NovelItem::fromJson5(data);
            addTopLevelItem(newItem);
            event->acceptProposedAction();
            return;
        }

        if (mime->hasText()) {
            // External text: create a new item
            QString text = mime->text();
            QTreeWidgetItem *newItem = new QTreeWidgetItem();
            newItem->setText(0, text);
            // don't do this!
            addTopLevelItem(newItem);

            event->acceptProposedAction();
            return;
        }
*/
    // 8003801186
        event->ignore();
    }

    void startDrag(Qt::DropActions supportedActions) override {
        QTreeWidgetItem* item = currentItem();
        if (!item) return;

        QMimeData* mimeData = model()->mimeData(selectedIndexes());
        mBuildMimeData(item, mimeData);

        QFont font = this->font();
        QFontMetrics fm(font);
        QString text = item->text(0);
        QSize size(fm.horizontalAdvance(text) + 8, fm.height() + 4);

        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setFont(font);
        painter.setPen(Qt::black);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
        painter.end();

        // Start the drag
        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
        drag->exec(supportedActions);
    }
};

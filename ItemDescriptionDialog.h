#pragma once

#include <QDialog>

#include "StringList.h"

namespace Ui {
class ItemDescriptionDialog;
}

class Item;
class QKeyEvent;

class ItemDescriptionDialog: public QDialog {
    Q_OBJECT

public:
    ItemDescriptionDialog(Item* item, QWidget *parent = nullptr);
    ~ItemDescriptionDialog();

public slots:
    void buttonPressedAction()          { doButtonPressed(); }
    void editedAction(const QString& t) { doEdited(t); }
    void returnPressedAction()          { doReturnPressed(); }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QList<QPushButton*>        mButtons;
    Item*                      mItem;
    Ui::ItemDescriptionDialog* mUi;

    void doButtonPressed();
    void doEdited(const QString& t);
    void doReturnPressed();

    void addButton(int num, const QString& tag);
    void addTags(const StringList& tags);
    void noDefault(QPushButton* btn);
    void removeButton(const QPushButton* button);
    void setupConnections();
};

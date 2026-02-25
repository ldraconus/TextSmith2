#pragma once

#include <QDialog>

#include <StringList.h>

namespace Ui {
class ScriptDialog;
}

class QListWidgetItem;
class ScriptDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScriptDialog(QWidget *parent = nullptr);
    ~ScriptDialog();

    void setActiveScripts(const StringList& list);
    void setAvailableScripts(const StringList& list);

    StringList activeScripts() const;
    void setButtonStates();

public slots:
    void activeItemClicked(QListWidgetItem*);
    void activeItemDoubleClicked(QListWidgetItem*);
    void allActive();
    void allInactive();
    void availableItemClicked(QListWidgetItem*);
    void availableItemDoubleClicked(QListWidgetItem*);
    void makeActive();
    void makeInactive();

private:
    Ui::ScriptDialog *mUi;

    void removeItem(const QString& text);
};

#pragma once

#include <QDialog>

#include <Map.h>
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
    void setAvailableScripts(const StringList& list, const Map<QString, QString>& hoveText = { });

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
    Map<QString, QString> mHoverText;
    Ui::ScriptDialog*     mUi;

    void removeItem(const QString& text);
};

#pragma once

#include <QMainWindow>

class QApplication;

QT_BEGIN_NAMESPACE
namespace Ui {
    class Main;
}
QT_END_NAMESPACE

class Main : public QMainWindow {
    Q_OBJECT

private:
    QApplication* mApp;
    Ui::Main*     mUi;

public:
    Main(QApplication* app, QWidget* parent = nullptr);
    ~Main();

public slots:
};
